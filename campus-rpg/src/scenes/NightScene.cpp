#include "NightScene.h"
#include "GameManager.h"
#include "TileMap.h"
#include "Entity.h"

namespace
{
    bool canMoveTo(engine::Vec2 pos, const TileMap &map)
    {
        int tileX = static_cast<int>(pos.x / 32.0f);
        int tileY = static_cast<int>(pos.y / 32.0f);
        return map.isWalkable(tileX, tileY);
    }

    PlayerEntity *findPlayer(TileMap &map)
    {
        for (const auto &e : map.entities())
        {
            if (e && e->type() == "player")
                return static_cast<PlayerEntity *>(e.get());
        }
        return nullptr;
    }
} // namespace

void NightScene::handleInput(engine::IInput &input)
{
    moveX_ = 0.0f;
    moveY_ = 0.0f;

    if (input.isKeyPressed(engine::Key::Left) || input.isKeyPressed(engine::Key::A))
        moveX_ -= 1.0f;
    if (input.isKeyPressed(engine::Key::Right) || input.isKeyPressed(engine::Key::D))
        moveX_ += 1.0f;
    if (input.isKeyPressed(engine::Key::Up) || input.isKeyPressed(engine::Key::W))
        moveY_ -= 1.0f;
    if (input.isKeyPressed(engine::Key::Down) || input.isKeyPressed(engine::Key::S))
        moveY_ += 1.0f;

    if (input.wasKeyJustPressed(engine::Key::Escape))
    {
        GameManager::instance().setNight(false);
        GameManager::instance().enterScene(SceneType::Town);
    }
}

void NightScene::update(float deltaTime)
{
    TileMap &map = GameManager::instance().currentMap();
    PlayerEntity *player = findPlayer(map);
    if (!player)
        return;

    engine::Vec2 newPos = player->position();
    newPos.x += moveX_ * moveSpeed_ * deltaTime;
    if (canMoveTo(newPos, map))
        player->setPosition({newPos.x, player->position().y});

    newPos = player->position();
    newPos.y += moveY_ * moveSpeed_ * deltaTime;
    if (canMoveTo(newPos, map))
        player->setPosition({player->position().x, newPos.y});

    // Check collision with enemy entities.
    for (const auto &entity : map.entities())
    {
        if (entity && entity->type() == "enemy" && player->intersects(*entity))
        {
            auto enemyEntity = static_cast<EnemyEntity *>(entity.get());
            auto enemy = GameManager::instance().createEnemyFromTemplate(enemyEntity->enemyTemplateId());
            if (enemy)
            {
                GameManager::instance().battleSystem().startBattle(GameManager::instance().character(), *enemy);
                GameManager::instance().enterScene(SceneType::Battle);
            }
            break;
        }
    }
}

void NightScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    TileMap &map = GameManager::instance().currentMap();
    for (int y = 0; y < map.height(); ++y)
    {
        for (int x = 0; x < map.width(); ++x)
        {
            engine::Color color = map.tileAt(x, y).isWalkable() ? engine::Color(40, 40, 60) : engine::Color(20, 20, 30);
            renderer.drawRect({x * 32.0f, y * 32.0f, 32.0f, 32.0f}, color);
        }
    }

    for (const auto &entity : map.entities())
    {
        if (!entity)
            continue;
        engine::Color color;
        if (entity->type() == "player")
            color = engine::Color::blue();
        else if (entity->type() == "enemy")
            color = engine::Color::red();
        else
            color = engine::Color::gray();
        renderer.drawRect(entity->worldBounds(), color);
    }

    renderer.drawText("Town (Night) - WASD/Arrows: move, touch shadows to fight, Esc: sleep", {10, 10}, 16, engine::Color::white());
}
