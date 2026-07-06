#include "Entity.h"

Entity::Entity(engine::Vec2 position, engine::Rect bounds)
    : position_(position), bounds_(bounds) {}

void Entity::setPosition(engine::Vec2 pos)
{
    position_ = pos;
}

void Entity::move(engine::Vec2 delta)
{
    position_ += delta;
}

engine::Rect Entity::worldBounds() const
{
    return {position_.x + bounds_.x, position_.y + bounds_.y,
            bounds_.width, bounds_.height};
}

bool Entity::intersects(const Entity &other) const
{
    return worldBounds().intersects(other.worldBounds());
}

PlayerEntity::PlayerEntity(engine::Vec2 position)
    : Entity(position, {-16.0f, -16.0f, 32.0f, 32.0f}) {}

NpcEntity::NpcEntity(engine::Vec2 position, std::string socialLinkId)
    : Entity(position, {-16.0f, -16.0f, 32.0f, 32.0f}),
      socialLinkId_(std::move(socialLinkId)) {}

EnemyEntity::EnemyEntity(engine::Vec2 position, std::string enemyTemplateId)
    : Entity(position, {-16.0f, -16.0f, 32.0f, 32.0f}),
      enemyTemplateId_(std::move(enemyTemplateId)) {}
