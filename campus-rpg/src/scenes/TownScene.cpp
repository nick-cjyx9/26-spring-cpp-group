#include "TownScene.h"
#include "GameManager.h"
#include "TileMap.h"
#include "Entity.h"
#include "TownMapData.h"

#include <cmath>

namespace
{
    engine::Rect interactionArea(const Entity &player)
    {
        engine::Vec2 pos = player.position();
        return {pos.x - 5.0f, pos.y - 5.0f, 10.0f, 10.0f};
    }

    bool canMoveTo(engine::Vec2 pos, const TileMap & /*map*/)
    {
        if (pos.x < 0.0f || pos.y < 0.0f || pos.x > 800.0f || pos.y > 600.0f)
            return false;
        engine::Rect playerBox{pos.x, pos.y, 1.0f, 1.0f};
        bool isSecond = GameManager::instance().onSecondMap();
        for (const auto &wall : mapCollisionZones(isSecond))
        {
            if (playerBox.intersects(wall))
                return false;
        }
        return true;
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

    // Auto-rescue: if player is stuck inside a collision zone, teleport to default spawn.
    bool isSecond = GameManager::instance().onSecondMap();
    if (isInCollisionZone(player->position(), isSecond))
        player->setPosition(mapDefaultSpawn(isSecond));

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

    // Debug: draw red rectangles around collision zones (both maps).
    {
        for (const auto &wall : mapCollisionZones(onSecond))
            renderer.drawRect(wall, engine::Color(255, 0, 0, 100));
    }

    if (!onSecond)
    {
        // Labels at building centers.
        renderer.drawText("home", {500, 20}, 18, engine::Color(255, 255, 255, 200));
        renderer.drawText("shop", {500, 140}, 18, engine::Color(255, 255, 255, 200));

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

    // HUD: top boxes use the same size and padding.
    constexpr float hudX = 10.0f;
    constexpr float hudY = 10.0f;
    constexpr float hudW = 180.0f;
    constexpr float hudH = 56.0f;
    constexpr float hudPad = 10.0f;
    constexpr float rightHudX = 800.0f - hudX - hudW;
    renderer.drawRect({hudX, hudY, hudW, hudH}, engine::Color(0, 0, 0, 200));
    renderer.drawRect({hudX + 2.0f, hudY + 2.0f, hudW - 4.0f, hudH - 4.0f}, engine::Color(30, 30, 50, 200));
    renderer.drawText("lv." + std::to_string(GameManager::instance().character().level()) +
                          "  " + GameManager::instance().character().name(),
                      {hudX + hudPad, hudY + hudPad + 2.0f}, 18, engine::Color::yellow());

    // Day indicator - top right corner.
    renderer.drawRect({rightHudX, hudY, hudW, hudH}, engine::Color(0, 0, 0, 200));
    renderer.drawRect({rightHudX + 2.0f, hudY + 2.0f, hudW - 4.0f, hudH - 4.0f}, engine::Color(30, 30, 50, 200));
    renderer.drawText("Day " + std::to_string(GameManager::instance().day()),
                      {rightHudX + hudPad, hudY + hudPad + 2.0f}, 20, engine::Color::cyan());

    // Interaction hint
    if (onSecond)
    {
        renderer.drawText("E:Talk/Exit  I:Items  C:Status  L:Social  Space:Armory  F5:Save  [School]",
                          {40, 570}, 14, engine::Color::white());
    }
    else
    {
        renderer.drawText("E:Talk/Shop/Home  I:Items  C:Status  L:Social  Space:Armory  F5:Save",
                          {50, 570}, 14, engine::Color::white());
    }

    // Save feedback overlay.
    if (!saveMessage_.empty())
    {
        renderer.drawRect({260, 510, 280, 36}, engine::Color(0, 0, 0, 200));
        renderer.drawText(saveMessage_, {280, 519}, 18, engine::Color::green());
    }

    // Character coordinate debug.
    PlayerEntity *debugPlayer = findPlayer(map);
    if (debugPlayer)
    {
        engine::Vec2 pp = debugPlayer->position();
        renderer.drawText("(" + std::to_string(static_cast<int>(pp.x)) +
                              ", " + std::to_string(static_cast<int>(pp.y)) + ")",
                          {10, 555}, 14, engine::Color(255, 255, 0, 180));
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
        engine::Rect playerBox{player->position().x - 5.0f, player->position().y - 5.0f,
                               10.0f, 10.0f};
        for (const auto &zone : townInteractionZones())
        {
            if (playerBox.intersects(zone.area))
            {
                if (zone.type == InteractionType::Home)
                    GameManager::instance().enterScene(SceneType::RestConfirm);
                else if (zone.type == InteractionType::Shop)
                    GameManager::instance().enterScene(SceneType::Shop);
                return;
            }
        }
    }
}
