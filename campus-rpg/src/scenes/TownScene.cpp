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

    bool canMoveTo(engine::Vec2 pos, const TileMap & /*map*/)
    {
        // No tile collision: allow movement anywhere within the 800x600 window.
        return pos.x >= 0.0f && pos.x <= 800.0f && pos.y >= 0.0f && pos.y <= 600.0f;
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
        GameManager::instance().enterScene(SceneType::Status);
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
    if (input.wasKeyJustPressed(engine::Key::Space))
    {
        GameManager::instance().enterScene(SceneType::Armory);
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

    engine::Vec2 newPos = player->position();
    newPos.x += moveX_ * moveSpeed_ * deltaTime;
    if (canMoveTo(newPos, map))
        player->setPosition({newPos.x, player->position().y});

    newPos = player->position();
    newPos.y += moveY_ * moveSpeed_ * deltaTime;
    if (canMoveTo(newPos, map))
        player->setPosition({player->position().x, newPos.y});

    // Map switching: entering school area (town, bottom-right crossroad) or
    // exit area (school, left-center road start).
    engine::Vec2 pp = player->position();
    bool onSecond = GameManager::instance().onSecondMap();
    if (!onSecond && pp.x >= 700.0f && pp.y >= 480.0f)
    {
        // Travel to school map.
        GameManager::instance().setOnSecondMap(true);
        TileMap &school = GameManager::instance().schoolMap();
        // Spawn at left-center of school map (the exit road start).
        engine::Vec2 spawnPos = {80.0f, 220.0f};
        for (const auto &e : school.entities())
        {
            if (e && e->type() == "player")
            {
                static_cast<PlayerEntity *>(e.get())->setPosition(spawnPos);
                break;
            }
        }
    }
    else if (onSecond && pp.x <= 50.0f)
    {
        // Travel back to town map.
        GameManager::instance().setOnSecondMap(false);
        TileMap &town = GameManager::instance().townMap();
        // Spawn near bottom-right crossroad so we don't bounce back.
        engine::Vec2 spawnPos = {640.0f, 430.0f};
        for (const auto &e : town.entities())
        {
            if (e && e->type() == "player")
            {
                static_cast<PlayerEntity *>(e.get())->setPosition(spawnPos);
                break;
            }
        }
    }
}

void TownScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    bool onSecond = GameManager::instance().onSecondMap();

    // Background scaled to fill window
    if (onSecond)
        renderer.drawTexture("town_bg2", {0, 0, 800, 600});
    else
        renderer.drawTexture("town_bg", {0, 0, 800, 600});

    TileMap &map = GameManager::instance().currentMap();

    if (!onSecond)
    {
        // Shop & home: no overlay, just labels to hint location.
        renderer.drawText("shop", {45, 55}, 20, engine::Color(255, 255, 255, 200));
        renderer.drawText("home", {578, 35}, 18, engine::Color(255, 255, 255, 200));

        // School arrow (right-pointing) at bottom-right crossroad.
        renderer.drawText(">", {740, 520}, 48, engine::Color(255, 255, 255, 230));
    }
    else
    {
        // Exit arrow (left-pointing) on school map left-center road start.
        renderer.drawText("<", {30, 215}, 48, engine::Color(255, 255, 255, 230));
    }

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

    // Day indicator - top right corner (replaces the old time clock).
    renderer.drawRect({620, 10, 170, 50}, engine::Color(0, 0, 0, 200));
    renderer.drawRect({622, 12, 166, 46}, engine::Color(30, 30, 50, 200));
    renderer.drawText("Day " + std::to_string(GameManager::instance().day()),
                      {650, 22}, 20, engine::Color::cyan());

    // Interaction hint
    if (onSecond)
    {
        renderer.drawText("E:Talk/Exit  I:Items  C:Status  L:Social  Space:Armory  N:Night  F5:Save  [School]",
                          {40, 570}, 14, engine::Color::white());
    }
    else
    {
        renderer.drawText("E:Talk/Shop/Home  I:Items  C:Status  L:Social  Space:Armory  N:Night  F5:Save",
                          {50, 570}, 14, engine::Color::white());
    }

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

    if (!GameManager::instance().onSecondMap())
    {
        // Home area: rest until night.
        if (player->position().x > 550.0f && player->position().x < 650.0f &&
            player->position().y <= 90.0f)
        {
            GameManager::instance().enterScene(SceneType::RestConfirm);
            return;
        }

        // Shop area: enter shop (requires E/Enter, not auto).
        if (player->position().x < 150.0f && player->position().y < 150.0f)
        {
            GameManager::instance().enterScene(SceneType::Shop);
            return;
        }
    }
}
