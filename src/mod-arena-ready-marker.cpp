/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2+ license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE
 */

#include "mod-arena-ready-marker.h"
#include "ArenaTeam.h"
#include "Battleground.h"
#include "Chat.h"
#include "GameObject.h"
#include "Log.h"
#include "Map.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"

#include <mutex>
#include <unordered_map>
#include <vector>

namespace
{
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
        if (!bg->isArena() || bg->GetStatus() >= STATUS_IN_PROGRESS || bg->GetStartDelayTime() <= BG_START_DELAY_15S
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
        ChatHandler(player->GetSession()).SendNotification("You are marked as ready {}/{}", count, req);

        if (count == req)
        {
            bg->AddStartingEventFlag(BG_STARTING_EVENT_2);
            bg->SetStartTime(bg->GetStartTime() + bg->GetStartDelayTime() - BG_START_DELAY_15S);
            bg->SetStartDelayTime(BG_START_DELAY_15S);
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
        if (bg->isArena() && sConfigMgr->GetOption<bool>("ModArenaReadyMarker.Enable", true))
            SpawnReadyMarkers(bg);
    }

    void OnBattlegroundUpdate(Battleground* bg, uint32 /*diff*/) override
    {
        // Remove the markers once the 15 seconds announcement was reached
        if (bg->isArena() && bg->GetStatus() == STATUS_WAIT_JOIN && (bg->GetStartingEventFlags() & BG_STARTING_EVENT_3))
            DespawnReadyMarkers(bg);
    }

    void OnBattlegroundDestroy(Battleground* bg) override
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        stateStore.erase(bg);
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
    new ArenaReadyMarkerBGScript();
    new go_arena_ready_marker();
}

void Addmod_arena_ready_markerScripts()
{
    ModArenaReadyMarkerScripts();
}
