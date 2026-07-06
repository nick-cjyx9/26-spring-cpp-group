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

    // Reuse town background scaled to fill window.
    renderer.drawTexture("town_bg", {0, 0, 800, 600});

    // Dark overlay for night atmosphere.
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
    renderer.drawText("bkpk", {635, 20}, 18, engine::Color::white());
    renderer.drawText("lv." + std::to_string(GameManager::instance().character().level()),
                      {720, 20}, 18, engine::Color::yellow());

    renderer.drawText("Night - touch shadows to fight, Esc: sleep",
                      {180, 570}, 16, engine::Color::white());
}
