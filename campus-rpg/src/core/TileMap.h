#pragma once

#include "EngineTypes.h"
#include "Entity.h"
#include "Tile.h"

#include <memory>
#include <vector>

class TileMap
{
public:
    TileMap(int width, int height);

    int width() const { return width_; }
    int height() const { return height_; }

    Tile &tileAt(int x, int y);
    const Tile &tileAt(int x, int y) const;
    bool isWalkable(int x, int y) const;

    void addEntity(std::unique_ptr<Entity> entity);
    bool removeEntity(Entity *entity);
    const std::vector<std::unique_ptr<Entity>> &entities() const { return entities_; }

    std::vector<Entity *> entitiesAt(const engine::Rect &area) const;
    Entity *firstEntityAt(const engine::Rect &area, const std::string &type) const;

    void clearEntities();

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<std::vector<Tile>> tiles_;
    std::vector<std::unique_ptr<Entity>> entities_;
};
