#include "TownScene.h"
#include "GameManager.h"
#include "TileMap.h"
#include "Entity.h"

#include <cmath>

namespace
{
    engine::Rect interactionArea(const Entity &player)
    {
        engine::Vec2 pos = player.position();
        return {pos.x - 28.0f, pos.y - 28.0f, 56.0f, 56.0f};
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
    if (input.wasKeyJustPressed(engine::Key::F5))
    {
        if (GameManager::instance().saveCurrentSlot())
        {
            saveMessage_ = "Saved to slot " + std::to_string(GameManager::instance().currentSlotId()) + "!";
            saveMessageTimer_ = 3.0f;
        }
        else
        {
            saveMessage_ = "Save failed.";
            saveMessageTimer_ = 3.0f;
        }
    }
    if (input.wasKeyJustPressed(engine::Key::L))
    {
        GameManager::instance().enterScene(SceneType::SocialLink);
    }
}

void TownScene::update(float deltaTime)
{
    if (saveMessageTimer_ > 0.0f)
    {
        saveMessageTimer_ -= deltaTime;
        if (saveMessageTimer_ <= 0.0f)
        {
            saveMessageTimer_ = 0.0f;
            saveMessage_.clear();
        }
    }

    TileMap &map = GameManager::instance().currentMap();
    PlayerEntity *player = findPlayer(map);
    if (!player)
        return;

    engine::Vec2 oldPos = player->position();

    engine::Vec2 newPos = player->position();
    newPos.x += moveX_ * moveSpeed_ * deltaTime;
    if (canMoveTo(newPos, map))
        player->setPosition({newPos.x, player->position().y});

    newPos = player->position();
    newPos.y += moveY_ * moveSpeed_ * deltaTime;
    if (canMoveTo(newPos, map))
        player->setPosition({player->position().x, newPos.y});

    // Track movement distance: every 4 tiles (128px) = 1 in-game hour.
    engine::Vec2 afterPos = player->position();
    float dist = std::hypot(afterPos.x - oldPos.x, afterPos.y - oldPos.y);
    if (dist > 0.1f)
    {
        accumulatedMoveDist_ += dist;
        const float kPixelsPerHour = GameManager::kTilesPerHour * 32.0f; // 4 tiles * 32px
        while (accumulatedMoveDist_ >= kPixelsPerHour)
        {
            accumulatedMoveDist_ -= kPixelsPerHour;
            GameManager::instance().advanceTime(1);
        }
    }
}

void TownScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    // Background scaled to fill window
    renderer.drawTexture("town_bg", {0, 0, 800, 600});

    TileMap &map = GameManager::instance().currentMap();

    // Draw shop area label
    renderer.drawRect({0, 0, 140, 140}, engine::Color(80, 80, 120, 120));
    renderer.drawText("shop", {45, 55}, 20, engine::Color::white());

    // Draw school area label
    renderer.drawRect({620, 100, 180, 450}, engine::Color(80, 120, 80, 120));
    renderer.drawText("school", {670, 300}, 20, engine::Color::white());

    // Draw entities
    for (const auto &entity : map.entities())
    {
        if (!entity)
            continue;
        if (entity->type() == "player")
        {
            auto b = entity->worldBounds();
            renderer.drawTexture("player", {b.x, b.y, 48, 48});
        }
        else if (entity->type() == "npc")
        {
            auto b = entity->worldBounds();
            renderer.drawTexture("npc", {b.x, b.y, 48, 48});
        }
    }

    // HUD - top left corner: level info
    renderer.drawRect({10, 10, 160, 50}, engine::Color(0, 0, 0, 180));
    renderer.drawText("lv." + std::to_string(GameManager::instance().character().level()),
                      {20, 20}, 18, engine::Color::yellow());

    // Time display - top right corner
    renderer.drawRect({620, 10, 170, 50}, engine::Color(0, 0, 0, 200));
    renderer.drawRect({622, 12, 166, 46}, engine::Color(30, 30, 50, 200));
    renderer.drawText(GameManager::instance().timeString(), {635, 22}, 20, engine::Color::cyan());

    // Interaction hint
    renderer.drawText("E: talk  I: inventory  C: char  N: night  F5: save  L: social",
                      {140, 570}, 16, engine::Color::white());

    // Save feedback overlay.
    if (!saveMessage_.empty())
    {
        renderer.drawRect({260, 510, 280, 36}, engine::Color(0, 0, 0, 200));
        renderer.drawText(saveMessage_, {280, 519}, 18, engine::Color::green());
    }
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
        return;
    }

    // Check if near shop area
    if (player->position().x < 140 && player->position().y < 140)
    {
        GameManager::instance().enterScene(SceneType::Shop);
    }
}
