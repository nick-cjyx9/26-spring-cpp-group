#pragma once

#include "EngineTypes.h"

#include <vector>

// =====================================================================
// Town map data (scene 1)
// Texture 528x320 → screen 800x600. sx≈1.515, sy=1.875.
// =====================================================================

inline const std::vector<engine::Rect> &townCollisionZones()
{
    static const std::vector<engine::Rect> zones = {
        {0, 0, 409, 208},     // Houses
        {124, 180, 167, 152}, // Swimming pool
        {436, 124, 217, 86},  // Shop
        {436, 0, 191, 90},    // Home
        {344, 279, 115, 199}, // equipments store
    };
    return zones;
}

inline const std::vector<engine::Rect> &townNpcSpawnZones()
{
    static const std::vector<engine::Rect> zones = {
        {130, 340, 200, 60},
        {460, 340, 180, 60},
        {130, 460, 500, 50},
    };
    return zones;
}

inline engine::Vec2 townDefaultSpawn()
{
    // Just below the houses wall (houses zone ends at y=208). Verified walkable:
    // not inside any town collision zone.
    return {365.0f, 220.0f};
}

// =====================================================================
// School map data (scene 2) — roads are walkable, white tiles beside
// roads are NPC spawn areas.
// Texture 528x320 → screen 800x600. sx≈1.515, sy=1.875.
// =====================================================================

inline const std::vector<engine::Rect> &schoolCollisionZones()
{
    static const std::vector<engine::Rect> zones = {
        // Upper-left shop
        {0, 0, 140, 123},
        // Upper-right school building
        {0, 300, 460, 300},
        {380, 0, 420, 120},
        // Right structure
        {700, 120, 100, 480},
        {650, 120, 50, 50},
    };
    return zones;
}

inline const std::vector<engine::Rect> &schoolNpcSpawnZones()
{
    // White/light tile strips beside the walkable roads.
    static const std::vector<engine::Rect> zones = {
        {230, 180, 470, 120},
        {480, 210, 200, 30},
        {460, 300, 40, 220},
    };
    return zones;
}

inline engine::Vec2 schoolDefaultSpawn()
{
    // Walkable road intersection (bottom-left area).
    return {100.0f, 250.0f};
}

// =====================================================================
// Unified accessors (map-aware)
// =====================================================================

inline const std::vector<engine::Rect> &mapCollisionZones(bool onSecondMap)
{
    return onSecondMap ? schoolCollisionZones() : townCollisionZones();
}

inline const std::vector<engine::Rect> &mapNpcSpawnZones(bool onSecondMap)
{
    return onSecondMap ? schoolNpcSpawnZones() : townNpcSpawnZones();
}

inline engine::Vec2 mapDefaultSpawn(bool onSecondMap)
{
    return onSecondMap ? schoolDefaultSpawn() : townDefaultSpawn();
}

// =====================================================================
// Interaction trigger zones (town only)
// =====================================================================

enum class InteractionType
{
    Home,
    Shop,
    WeaponShop
};

struct InteractionZone
{
    engine::Rect area;
    InteractionType type;
};

inline const std::vector<InteractionZone> &townInteractionZones()
{
    static const std::vector<InteractionZone> zones = {
        {{436, 90, 191, 60}, InteractionType::Home},
        {{436, 210, 217, 60}, InteractionType::Shop},
        {{344, 478, 115, 60}, InteractionType::WeaponShop},
    };
    return zones;
}

// =====================================================================
// Collision check (1x1 hitbox, map-aware)
// =====================================================================

inline bool isInCollisionZone(engine::Vec2 pos, bool onSecondMap)
{
    engine::Rect playerBox{pos.x, pos.y, 1.0f, 1.0f};
    for (const auto &wall : mapCollisionZones(onSecondMap))
    {
        if (playerBox.intersects(wall))
            return true;
    }
    return false;
}

// =====================================================================
// Stuck rescue: find the nearest walkable position to a (possibly
// trapped) point by scanning outward in expanding square rings.
// Falls back to the map default spawn (guaranteed walkable).
// =====================================================================

inline engine::Vec2 findNearestWalkable(engine::Vec2 pos, bool onSecondMap)
{
    if (!isInCollisionZone(pos, onSecondMap))
        return pos;

    const float step = 10.0f;
    const int maxRadius = 60; // up to 600px search radius
    for (int r = 1; r <= maxRadius; ++r)
    {
        for (int dx = -r; dx <= r; ++dx)
        {
            for (int dy = -r; dy <= r; ++dy)
            {
                const int adx = dx < 0 ? -dx : dx;
                const int ady = dy < 0 ? -dy : dy;
                if (adx != r && ady != r)
                    continue; // only probe the ring edge (interior already checked)
                engine::Vec2 cand{pos.x + dx * step, pos.y + dy * step};
                if (cand.x < 0.0f || cand.y < 0.0f ||
                    cand.x > 800.0f || cand.y > 600.0f)
                    continue;
                if (!isInCollisionZone(cand, onSecondMap))
                    return cand;
            }
        }
    }
    return mapDefaultSpawn(onSecondMap);
}
