#include "TownScene.h"
#include "GameManager.h"
#include "TileMap.h"
#include "Entity.h"

namespace
{
    engine::Rect interactionArea(const Entity &player)
    {
        engine::Vec2 pos = player.position();
        return {pos.x - 24.0f, pos.y - 24.0f, 48.0f, 48.0f};
    }

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

void TownScene::handleInput(engine::IInput &input)
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

    if (input.wasKeyJustPressed(engine::Key::E) || input.wasKeyJustPressed(engine::Key::Enter))
    {
        tryInteract();
    }
    if (input.wasKeyJustPressed(engine::Key::I))
    {
        GameManager::instance().enterScene(SceneType::Inventory);
    }
    if (input.wasKeyJustPressed(engine::Key::C))
    {
        GameManager::instance().enterScene(SceneType::Character);
    }
    if (input.wasKeyJustPressed(engine::Key::N))
    {
        GameManager::instance().setNight(true);
        // Spawn enemy entities on the map for night combat.
        TileMap &map = GameManager::instance().currentMap();
        engine::Vec2 playerPos{100, 100};
        PlayerEntity *player = findPlayer(map);
        if (player)
            playerPos = player->position();
        map.clearEntities();
        map.addEntity(std::make_unique<PlayerEntity>(playerPos));
        map.addEntity(std::make_unique<EnemyEntity>(engine::Vec2{250, 150}, "enemy_slime"));
        map.addEntity(std::make_unique<EnemyEntity>(engine::Vec2{400, 250}, "enemy_goblin"));
        map.addEntity(std::make_unique<EnemyEntity>(engine::Vec2{500, 350}, "enemy_boss"));
        GameManager::instance().enterScene(SceneType::Night);
    }
}

void TownScene::update(float deltaTime)
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
}

void TownScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    TileMap &map = GameManager::instance().currentMap();
    for (int y = 0; y < map.height(); ++y)
    {
        for (int x = 0; x < map.width(); ++x)
        {
            engine::Color color = map.tileAt(x, y).isWalkable() ? engine::Color(60, 60, 60) : engine::Color(30, 30, 30);
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
        else if (entity->type() == "npc")
            color = engine::Color::green();
        else
            color = engine::Color::red();
        renderer.drawRect(entity->worldBounds(), color);
    }

    renderer.drawText("Town (Morning) - WASD/Arrows: move, E: talk, I: inventory, C: character", {10, 10}, 16, engine::Color::white());
}

void TownScene::tryInteract()
{
    TileMap &map = GameManager::instance().currentMap();
    PlayerEntity *player = findPlayer(map);
    if (!player)
        return;

    Entity *npc = map.firstEntityAt(interactionArea(*player), "npc");
    if (npc)
    {
        GameManager::instance().enterScene(SceneType::Dialogue);
    }
}
