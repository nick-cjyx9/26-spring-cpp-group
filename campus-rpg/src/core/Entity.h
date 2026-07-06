#pragma once

#include "EngineTypes.h"

#include <string>

class Entity
{
public:
    Entity(engine::Vec2 position, engine::Rect bounds);
    virtual ~Entity() = default;

    engine::Vec2 position() const { return position_; }
    engine::Rect bounds() const { return bounds_; }

    void setPosition(engine::Vec2 pos);
    void move(engine::Vec2 delta);

    engine::Rect worldBounds() const;
    bool intersects(const Entity &other) const;

    virtual std::string type() const = 0;

protected:
    engine::Vec2 position_;
    engine::Rect bounds_;
};

class PlayerEntity : public Entity
{
public:
    explicit PlayerEntity(engine::Vec2 position);
    std::string type() const override { return "player"; }
};

class NpcEntity : public Entity
{
public:
    NpcEntity(engine::Vec2 position, std::string socialLinkId);
    std::string type() const override { return "npc"; }
    const std::string &socialLinkId() const { return socialLinkId_; }

private:
    std::string socialLinkId_;
};

class EnemyEntity : public Entity
{
public:
    EnemyEntity(engine::Vec2 position, std::string enemyTemplateId);
    std::string type() const override { return "enemy"; }
    const std::string &enemyTemplateId() const { return enemyTemplateId_; }

private:
    std::string enemyTemplateId_;
};
