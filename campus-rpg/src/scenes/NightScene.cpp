#include "NightScene.h"
#include "GameManager.h"
#include "TileMap.h"
#include "Entity.h"

namespace
{
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

    if (input.wasKeyJustPressed(engine::Key::C))
    {
        GameManager::instance().enterScene(SceneType::Status);
        return;
    }

    // Home area interaction: only on the town map (not the school/second map).
    if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
    {
        PlayerEntity *p = findPlayer(GameManager::instance().currentMap());
        if (p && !GameManager::instance().onSecondMap() &&
            p->position().x > 550.0f && p->position().x < 650.0f && p->position().y <= 90.0f)
        {
            GameManager::instance().enterScene(SceneType::RestConfirm);
            return;
        }
    }

    if (input.wasKeyJustPressed(engine::Key::Escape))
    {
        // Returning from night to day = next day: refresh NPCs on the map.
        GameManager::instance().setNight(false);
        GameManager::instance().advanceDay();
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

    // Map switching at night (same triggers as day: school area / exit area).
    engine::Vec2 pp = player->position();
    bool onSecond = GameManager::instance().onSecondMap();
    if (!onSecond && pp.x >= 700.0f && pp.y >= 480.0f)
    {
        // Travel to school map at night.
        GameManager::instance().setOnSecondMap(true);
        TileMap &school = GameManager::instance().schoolMap();
        engine::Vec2 spawnPos = {80.0f, 220.0f};
        school.clearEntities();
        school.addEntity(std::make_unique<PlayerEntity>(spawnPos));
        school.addEntity(std::make_unique<EnemyEntity>(engine::Vec2{250, 150}, "enemy_slime"));
        school.addEntity(std::make_unique<EnemyEntity>(engine::Vec2{400, 250}, "enemy_goblin"));
        school.addEntity(std::make_unique<EnemyEntity>(engine::Vec2{500, 350}, "enemy_boss"));
    }
    else if (onSecond && pp.x <= 50.0f)
    {
        // Travel back to town map at night.
        GameManager::instance().setOnSecondMap(false);
        TileMap &town = GameManager::instance().townMap();
        engine::Vec2 spawnPos = {640.0f, 430.0f};
        town.clearEntities();
        town.addEntity(std::make_unique<PlayerEntity>(spawnPos));
        town.addEntity(std::make_unique<EnemyEntity>(engine::Vec2{250, 150}, "enemy_slime"));
        town.addEntity(std::make_unique<EnemyEntity>(engine::Vec2{400, 250}, "enemy_goblin"));
        town.addEntity(std::make_unique<EnemyEntity>(engine::Vec2{500, 350}, "enemy_boss"));
    }
}

void NightScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    bool onSecond = GameManager::instance().onSecondMap();

    // Reuse background scaled to fill window; second map uses town_bg2.
    if (onSecond)
        renderer.drawTexture("town_bg2", {0, 0, 800, 600});
    else
        renderer.drawTexture("town_bg", {0, 0, 800, 600});

    // Dark overlay for night atmosphere; second map is even darker/more menacing.
    if (onSecond)
        renderer.drawRect({0, 0, 800, 600}, engine::Color(20, 0, 10, 200));
    else
        renderer.drawRect({0, 0, 800, 600}, engine::Color(10, 10, 40, 180));

    TileMap &map = GameManager::instance().currentMap();

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
        else if (entity->type() == "enemy")
        {
            auto b = entity->worldBounds();
            renderer.drawTexture("shadow", {b.x, b.y, 48, 48});
        }
    }

    // HUD
    renderer.drawRect({620, 10, 170, 70}, engine::Color(0, 0, 0, 180));
    if (onSecond)
    {
        renderer.drawText("Night [School]", {622, 14}, 16, engine::Color(255, 150, 150));
        renderer.drawText("lv." + std::to_string(GameManager::instance().character().level()),
                          {720, 38}, 18, engine::Color::yellow());
        renderer.drawText("Danger!", {635, 56}, 14, engine::Color(255, 100, 100));
    }
    else
    {
        renderer.drawText("Night", {635, 20}, 18, engine::Color::white());
        renderer.drawText("lv." + std::to_string(GameManager::instance().character().level()),
                          {720, 20}, 18, engine::Color::yellow());
    }

    // Map-switch arrows (same positions as day).
    if (!onSecond)
        renderer.drawText(">", {740, 520}, 48, engine::Color(255, 255, 255, 180));
    else
        renderer.drawText("<", {30, 215}, 48, engine::Color(255, 255, 255, 180));

    if (onSecond)
    {
        renderer.drawText("Night [School] - touch shadows to fight   C: status   <: exit   Esc: next day",
                          {120, 570}, 14, engine::Color::white());
    }
    else
    {
        renderer.drawText("Night - touch shadows to fight   C: status   Home: E   >: school   Esc: next day",
                          {60, 570}, 14, engine::Color::white());
    }
}
