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
    auraTimer_ += deltaTime;

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
                GameManager::instance().setCurrentEnemyTextureId(enemyEntity->textureId());
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
        school.addEntity(std::make_unique<EnemyEntity>(engine::Vec2{250, 150}, "enemy_slime", "monsters/iceball"));
        school.addEntity(std::make_unique<EnemyEntity>(engine::Vec2{400, 250}, "enemy_goblin", "monsters/beetle"));
        school.addEntity(std::make_unique<EnemyEntity>(engine::Vec2{500, 350}, "enemy_boss", "monsters/rhino"));
    }
    else if (onSecond && pp.x <= 50.0f)
    {
        // Travel back to town map at night.
        GameManager::instance().setOnSecondMap(false);
        TileMap &town = GameManager::instance().townMap();
        engine::Vec2 spawnPos = {640.0f, 430.0f};
        town.clearEntities();
        town.addEntity(std::make_unique<PlayerEntity>(spawnPos));
        town.addEntity(std::make_unique<EnemyEntity>(engine::Vec2{250, 150}, "enemy_slime", "monsters/bunny"));
        town.addEntity(std::make_unique<EnemyEntity>(engine::Vec2{400, 250}, "enemy_goblin", "monsters/duck"));
        town.addEntity(std::make_unique<EnemyEntity>(engine::Vec2{500, 350}, "enemy_boss", "monsters/treant"));
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
            std::string playerTex = "player_" + std::to_string(GameManager::instance().selectedHeroIndex());
            float size = 96.0f;
            float cx = b.x + b.width / 2.0f;
            float cy = b.y + b.height / 2.0f;
            renderer.drawTexture(playerTex, {cx - size / 2.0f, cy - size / 2.0f, size, size});
        }
        else if (entity->type() == "enemy")
        {
            auto b = entity->worldBounds();
            auto enemyEntity = static_cast<EnemyEntity *>(entity.get());
            std::string tex = enemyEntity->textureId();
            if (tex.empty()) tex = "shadow";
            bool isBoss = (enemyEntity->enemyTemplateId() == "enemy_boss");
            float size = isBoss ? 72.0f : 48.0f;
            float offset = (48.0f - size) / 2.0f;
            float bx = b.x + offset;
            float by = b.y + offset;

            if (isBoss)
            {
                // 危险气场：暗紫色底光
                engine::Color darkAura(130, 0, 220, 65);
                float pad = 10.0f;
                renderer.drawRect({bx - pad, by - pad, size + pad * 2, size + pad * 2}, darkAura);
            }

            renderer.drawTexture(tex, {bx, by, size, size});

            if (isBoss)
            {
                // 动态上升的邪气直线（直接画在身上）
                engine::Color evilPurple(220, 0, 255, 190);
                const int numLines = 10;
                float lineH = 18.0f;
                float speed = 50.0f;
                float cycle = size + lineH;
                for (int i = 0; i < numLines; ++i)
                {
                    float lx = bx + (size * (i + 0.5f) / numLines);
                    float phase = auraTimer_ * speed + i * 16.0f;
                    phase = phase - static_cast<int>(phase / cycle) * cycle;
                    float ly = by + size - phase;
                    float drawY = std::max(ly, by);
                    float drawH = std::min(ly + lineH, by + size) - drawY;
                    if (drawH > 0)
                        renderer.drawRect({lx - 1.5f, drawY, 3.0f, drawH}, evilPurple);
                }

                // 更快的细线增加紧张感
                engine::Color fastPurple(255, 0, 200, 150);
                for (int i = 0; i < 6; ++i)
                {
                    float lx = bx + 8.0f + i * (size - 16.0f) / 5.0f;
                    float phase = auraTimer_ * 75.0f + i * 24.0f;
                    phase = phase - static_cast<int>(phase / cycle) * cycle;
                    float ly = by + size - phase;
                    float drawY = std::max(ly, by);
                    float drawH = std::min(ly + lineH * 0.8f, by + size) - drawY;
                    if (drawH > 0)
                        renderer.drawRect({lx - 1.0f, drawY, 2.0f, drawH}, fastPurple);
                }
            }
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
