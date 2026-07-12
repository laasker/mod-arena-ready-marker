/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2+ license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE
 */

#ifndef MOD_ARENA_READY_MARKER_H
#define MOD_ARENA_READY_MARKER_H

#include "SharedDefines.h"

constexpr uint32 ARENA_READY_MARKER_ENTRY = 301337;

#define module_string "mod-arena-ready-marker"

enum ArenaReadyMarkerStrings
{
    MARKED_READY_SELF     = 0,
    MARKED_READY_TEAMMATE = 1,
    MARKED_READY_OPPONENT = 2
};

struct ArenaReadyMarkerData
{
    BattlegroundTypeId bgTypeId;
    float posX, posY, posZ, orientation;
};

constexpr ArenaReadyMarkerData ReadyMarkers[] =
{
    { BATTLEGROUND_BE, 6287.19f, 288.25f,  5.33f,  0.0f }, // Gold
    { BATTLEGROUND_BE, 6189.47f, 235.54f,  5.52f,  0.0f }, // Green
    { BATTLEGROUND_DS, 1229.44f, 759.35f,  17.89f, 0.0f }, // Gold
    { BATTLEGROUND_DS, 1352.90f, 822.77f,  17.96f, 0.0f }, // Green
    { BATTLEGROUND_NA, 4022.82f, 2966.61f, 12.17f, 0.0f }, // Gold
    { BATTLEGROUND_NA, 4090.46f, 2875.43f, 12.16f, 0.0f }, // Green
    { BATTLEGROUND_RL, 1273.71f, 1734.05f, 31.61f, 0.0f }, // Gold
    { BATTLEGROUND_RL, 1298.61f, 1598.59f, 31.62f, 0.0f }, // Green
    { BATTLEGROUND_RV, 758.16f, -279.86f,  2.80f,  0.0f }, // Gold
    { BATTLEGROUND_RV, 768.72f, -288.48f,  2.80f,  0.0f }, // Green
};

#endif // MOD_ARENA_READY_MARKER_H
