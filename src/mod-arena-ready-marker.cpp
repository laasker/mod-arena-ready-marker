/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2+ license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE
 */

#include "mod-arena-ready-marker.h"
#include "ArenaTeam.h"
#include "Battleground.h"
#include "Chat.h"
#include "Config.h"
#include "GameObject.h"
#include "Log.h"
#include "Map.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace
{
    // Cached config values, refreshed on (re)load; atomics because BGs update on map worker threads
    std::atomic<bool> moduleEnabled = true;
    std::atomic<bool> ratedOnly = false;
    std::atomic<bool> announceAll = false;
    std::atomic<int32> countdownTime = BG_START_DELAY_15S;

    struct ReadyMarkerState
    {
        std::vector<ObjectGuid> markers;
        GuidSet clicked;
    };

    // Battlegrounds update on per-map worker threads, so guard the container
    std::unordered_map<Battleground const*, ReadyMarkerState> stateStore;
    std::mutex stateMutex;

    void SpawnReadyMarkers(Battleground* bg)
    {
        Map* map = bg->FindBgMap();
        if (!map)
            return;

        for (auto const& marker : ReadyMarkers)
        {
            if (marker.bgTypeId != bg->GetBgTypeID())
                continue;

            GameObject* go = new GameObject();
            if (!go->Create(map->GenerateLowGuid<HighGuid::GameObject>(), ARENA_READY_MARKER_ENTRY, map, PHASEMASK_NORMAL, marker.posX, marker.posY, marker.posZ, marker.orientation, G3D::Quat(0.0f, 0.0f, 0.0f, 0.0f), 100, GO_STATE_READY))
            {
                LOG_ERROR("module.arenareadymarker", "Cannot create ready marker (entry: {}) for arena (map: {}, instance id: {})!", ARENA_READY_MARKER_ENTRY, map->GetId(), bg->GetInstanceID());
                delete go;
                continue;
            }

            if (!map->AddToMap(go))
            {
                delete go;
                continue;
            }

            // Ring of Valor spawns all of its objects with phasemask 3 (reason unknown, probably unnecessary)
            if (bg->GetBgTypeID() == BATTLEGROUND_RV)
                go->SetPhaseMask(3, true);

            std::lock_guard<std::mutex> lock(stateMutex);
            stateStore[bg].markers.push_back(go->GetGUID());
        }
    }

    void DespawnReadyMarkers(Battleground* bg)
    {
        std::vector<ObjectGuid> markers;
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            auto itr = stateStore.find(bg);
            if (itr == stateStore.end() || itr->second.markers.empty())
                return;

            markers.swap(itr->second.markers);
        }

        Map* map = bg->FindBgMap();
        if (!map)
            return;

        for (ObjectGuid const& guid : markers)
        {
            if (GameObject* go = map->GetGameObject(guid))
            {
                go->SetRespawnTime(0);
                go->Delete();
            }
        }
    }

    void HandleReadyMarkerClick(Battleground* bg, Player* player)
    {
        int32 targetCountdown = countdownTime;

        if (!bg->isArena() || bg->GetStatus() >= STATUS_IN_PROGRESS || bg->GetStartDelayTime() <= targetCountdown
            || (bg->GetStartingEventFlags() & BG_STARTING_EVENT_3) || player->IsSpectator())
            return;

        uint32 count = 0;
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            GuidSet& clicked = stateStore[bg].clicked;
            clicked.insert(player->GetGUID());
            count = clicked.size();
        }

        uint32 req = ArenaTeam::GetReqPlayersForType(bg->GetArenaType());

        ChatHandler handler(player->GetSession());
        handler.SendNotification(handler.PGetParseModuleString(module_string, MARKED_READY_SELF, count, req));

        if (announceAll)
        {
            for (auto const& [guid, member] : bg->GetPlayers())
            {
                if (member == player || member->IsSpectator() || !member->GetSession())
                    continue;

                ChatHandler memberHandler(member->GetSession());
                if (member->GetBgTeamId() == player->GetBgTeamId())
                    memberHandler.SendNotification(memberHandler.PGetParseModuleString(module_string, MARKED_READY_TEAMMATE, player->GetName(), count, req));
                else
                    memberHandler.SendNotification(memberHandler.PGetParseModuleString(module_string, MARKED_READY_OPPONENT, count, req));
            }
        }

        if (count == req)
        {
            // Suppress the announcements whose thresholds (60s/30s/15s, see Arena.cpp) the countdown jumps past,
            // so stale "One minute" / "Thirty seconds" texts don't fire right after the jump
            if (targetCountdown < BG_START_DELAY_1M)
                bg->AddStartingEventFlag(BG_STARTING_EVENT_1);
            if (targetCountdown < BG_START_DELAY_30S)
                bg->AddStartingEventFlag(BG_STARTING_EVENT_2);
            if (targetCountdown < BG_START_DELAY_15S)
                bg->AddStartingEventFlag(BG_STARTING_EVENT_3);

            bg->SetStartTime(bg->GetStartTime() + bg->GetStartDelayTime() - targetCountdown);
            bg->SetStartDelayTime(targetCountdown);
        }
    }
}

class ArenaReadyMarkerBGScript : public AllBattlegroundScript
{
public:
    ArenaReadyMarkerBGScript() : AllBattlegroundScript("ArenaReadyMarkerBGScript", {
        ALLBATTLEGROUNDHOOK_ON_BATTLEGROUND_SETUP,
        ALLBATTLEGROUNDHOOK_ON_BATTLEGROUND_UPDATE,
        ALLBATTLEGROUNDHOOK_ON_BATTLEGROUND_DESTROY
    }) { }

    void OnBattlegroundSetup(Battleground* bg) override
    {
        if (!moduleEnabled || !bg->isArena())
            return;

        if (ratedOnly && !bg->isRated())
            return;

        SpawnReadyMarkers(bg);
    }

    void OnBattlegroundUpdate(Battleground* bg, uint32 /*diff*/) override
    {
        if (!bg->isArena() || bg->GetStatus() != STATUS_WAIT_JOIN)
            return;

        // Remove the markers once clicking them can no longer expedite the countdown:
        // the 15 seconds announcement was reached, or the countdown is at/below the configured target
        if ((bg->GetStartingEventFlags() & BG_STARTING_EVENT_3) || bg->GetStartDelayTime() <= countdownTime)
            DespawnReadyMarkers(bg);
    }

    void OnBattlegroundDestroy(Battleground* bg) override
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        stateStore.erase(bg);
    }
};

class ArenaReadyMarkerWorldScript : public WorldScript
{
public:
    ArenaReadyMarkerWorldScript() : WorldScript("ArenaReadyMarkerWorldScript", {
        WORLDHOOK_ON_AFTER_CONFIG_LOAD
    }) { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        moduleEnabled = sConfigMgr->GetOption<bool>("ModArenaReadyMarker.Enable", true);
        announceAll = sConfigMgr->GetOption<bool>("ModArenaReadyMarker.AnnounceAll", true);
        countdownTime = std::clamp<int32>(sConfigMgr->GetOption<int32>("ModArenaReadyMarker.CountdownTime", 15), 0, sWorld->getIntConfig(CONFIG_ARENA_PREP_TIME)) * IN_MILLISECONDS;
        ratedOnly = sConfigMgr->GetOption<bool>("ModArenaReadyMarker.RatedOnly", false);
    }
};

class go_arena_ready_marker : public GameObjectScript
{
public:
    go_arena_ready_marker() : GameObjectScript("go_arena_ready_marker") { }

    bool OnGossipHello(Player* player, GameObject* /*go*/) override
    {
        if (Battleground* bg = player->GetBattleground())
            HandleReadyMarkerClick(bg, player);

        return false;
    }
};

void ModArenaReadyMarkerScripts()
{
    new ArenaReadyMarkerWorldScript();
    new ArenaReadyMarkerBGScript();
    new go_arena_ready_marker();
}

void Addmod_arena_ready_markerScripts()
{
    ModArenaReadyMarkerScripts();
}
