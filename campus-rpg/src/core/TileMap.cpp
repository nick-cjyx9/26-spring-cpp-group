#include "TileMap.h"

#include <algorithm>

TileMap::TileMap(int width, int height)
    : width_(width), height_(height), tiles_(height, std::vector<Tile>(width)) {}

Tile &TileMap::tileAt(int x, int y)
{
    return tiles_[static_cast<size_t>(y)][static_cast<size_t>(x)];
}

const Tile &TileMap::tileAt(int x, int y) const
{
    return tiles_[static_cast<size_t>(y)][static_cast<size_t>(x)];
}

bool TileMap::isWalkable(int x, int y) const
{
    if (x < 0 || x >= width_ || y < 0 || y >= height_)
        return false;
    return tileAt(x, y).isWalkable();
}

void TileMap::addEntity(std::unique_ptr<Entity> entity)
{
    if (entity)
        entities_.push_back(std::move(entity));
}

bool TileMap::removeEntity(Entity *entity)
{
    auto it = std::find_if(entities_.begin(), entities_.end(),
                           [entity](const auto &e)
                           { return e.get() == entity; });
    if (it == entities_.end())
        return false;
    entities_.erase(it);
    return true;
}

std::vector<Entity *> TileMap::entitiesAt(const engine::Rect &area) const
{
    std::vector<Entity *> result;
    for (const auto &entity : entities_)
    {
        if (entity && entity->worldBounds().intersects(area))
            result.push_back(entity.get());
    }
    return result;
}

Entity *TileMap::firstEntityAt(const engine::Rect &area, const std::string &type) const
{
    for (const auto &entity : entities_)
    {
        if (entity && entity->type() == type && entity->worldBounds().intersects(area))
            return entity.get();
    }
    return nullptr;
}

void TileMap::clearEntities()
{
    entities_.clear();
}
