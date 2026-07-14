#include "RestConfirmScene.h"
#include "GameManager.h"
#include "TileMap.h"
#include "Entity.h"
#include "Character.h"

#include <algorithm>

RestConfirmScene::~RestConfirmScene() = default;

void RestConfirmScene::drawShadow(engine::IRenderer &r, float x, float y, float w, float h, float offset)
{
    r.drawRect({x + offset, y + offset, w, h}, engine::Color(0, 0, 0, 60));
}

void RestConfirmScene::drawBorder(engine::IRenderer &r, float x, float y, float w, float h,
                                  engine::Color bright, engine::Color dark)
{
    r.drawRect({x, y, w, 2}, bright);
    r.drawRect({x, y + h - 2, w, 2}, bright);
    r.drawRect({x, y, 2, h}, bright);
    r.drawRect({x + w - 2, y, 2, h}, bright);
    r.drawRect({x + 3, y + 3, w - 6, 1}, dark);
    r.drawRect({x + 3, y + h - 4, w - 6, 1}, dark);
    r.drawRect({x + 3, y + 3, 1, h - 6}, dark);
    r.drawRect({x + w - 4, y + 3, 1, h - 6}, dark);
}

void RestConfirmScene::drawPanel(engine::IRenderer &r, float x, float y, float w, float h,
                                 engine::Color fill, engine::Color bright, engine::Color dark)
{
    drawShadow(r, x, y, w, h);
    r.drawRect({x, y, w, h}, fill);
    drawBorder(r, x, y, w, h, bright, dark);
}

void RestConfirmScene::handleInput(engine::IInput &input)
{
    if (input.wasKeyJustPressed(engine::Key::Left) || input.wasKeyJustPressed(engine::Key::A))
        selected_ = 0;
    if (input.wasKeyJustPressed(engine::Key::Right) || input.wasKeyJustPressed(engine::Key::D))
        selected_ = 1;

    if (selected_ == 0 && (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E)))
    {
        confirm();
        return;
    }

    if (input.wasKeyJustPressed(engine::Key::Escape) || (selected_ == 1 && (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))))
    {
        GameManager::instance().enterScene(nightMode_ ? SceneType::Night : SceneType::Town);
    }
}

void RestConfirmScene::confirm()
{
    if (nightMode_)
    {
        // Night -> next day: advance day, restore 30% max HP, go to Town.
        Character &c = GameManager::instance().character();
        int healed = std::min(c.maxHp(), c.hp() + c.maxHp() * 30 / 100);
        c.setHp(healed);
        GameManager::instance().setNight(false);
        GameManager::instance().advanceDay();
        GameManager::instance().enterScene(SceneType::Town);
    }
    else
    {
        // Day -> night: spawn enemies and switch scene.
        GameManager::instance().setNight(true);
        TileMap &map = GameManager::instance().currentMap();
        engine::Vec2 playerPos{100, 100};
        for (const auto &e : map.entities())
        {
            if (e && e->type() == "player")
                playerPos = e->position();
        }
        map.clearEntities();
        map.addEntity(std::make_unique<PlayerEntity>(playerPos));
        map.addEntity(std::make_unique<EnemyEntity>(engine::Vec2{225, 400}, "enemy_slime", "monsters/bunny"));
        map.addEntity(std::make_unique<EnemyEntity>(engine::Vec2{400, 250}, "enemy_goblin", "monsters/duck"));
        map.addEntity(std::make_unique<EnemyEntity>(engine::Vec2{500, 350}, "enemy_boss", "monsters/treant"));
        GameManager::instance().enterScene(SceneType::Night);
    }
}

void RestConfirmScene::update(float /*deltaTime*/)
{
}

void RestConfirmScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    bool onSecond = GameManager::instance().onSecondMap();
    if (onSecond)
        renderer.drawTexture("town_bg2", {0, 0, 800, 600});
    else
        renderer.drawTexture("town_bg", {0, 0, 800, 600});

    renderer.drawRect({0, 0, 800, 600}, engine::Color(0, 0, 0, 160));

    engine::Color panelFill(18, 18, 32, 235);
    engine::Color panelBright(120, 130, 180, 255);
    engine::Color panelDark(8, 8, 16, 200);
    engine::Color accent(180, 160, 50, 255);
    engine::Color accentDark(60, 50, 20, 200);
    engine::Color btnFillConfirm(40, 80, 50, 220);
    engine::Color btnFillCancel(80, 40, 40, 220);

    float boxX = 150.0f, boxY = 110.0f, boxW = 500.0f, boxH = 160.0f;
    drawPanel(renderer, boxX, boxY, boxW, boxH, panelFill, panelBright, panelDark);

    float tagW = 130.0f;
    float tagX = boxX + 20.0f, tagY = boxY - 16.0f;
    drawShadow(renderer, tagX, tagY, tagW, 30.0f, 3.0f);
    renderer.drawRect({tagX, tagY, tagW, 30.0f}, engine::Color(40, 35, 60, 235));
    drawBorder(renderer, tagX, tagY, tagW, 30.0f, accent, accentDark);
    renderer.drawText("Home", {tagX + 35.0f, tagY + 6.0f}, 17, engine::Color::white());

    std::string prompt = nightMode_
                             ? "Do you want to rest? (advance to next day, restore 30% HP)"
                             : "Do you want to rest? (go directly to night)";
    renderer.drawText(prompt,
                      {boxX + 30.0f, boxY + 50.0f}, 18, engine::Color::white());

    float btnW = 150.0f, btnH = 50.0f, btnY = boxY + boxH - 70.0f;
    float btn1X = boxX + 60.0f, btn2X = boxX + boxW - 60.0f - btnW;

    engine::Color c1Fill = (selected_ == 0) ? engine::Color(60, 140, 80, 240) : btnFillConfirm;
    engine::Color c1Bright = (selected_ == 0) ? engine::Color(160, 255, 180, 255) : panelBright;
    drawPanel(renderer, btn1X, btnY, btnW, btnH, c1Fill, c1Bright, panelDark);
    renderer.drawText("E / Enter", {btn1X + 30.0f, btnY + 16.0f}, 18,
                      (selected_ == 0) ? engine::Color::yellow() : engine::Color::white());

    engine::Color c2Fill = (selected_ == 1) ? engine::Color(140, 60, 60, 240) : btnFillCancel;
    engine::Color c2Bright = (selected_ == 1) ? engine::Color(255, 160, 160, 255) : panelBright;
    drawPanel(renderer, btn2X, btnY, btnW, btnH, c2Fill, c2Bright, panelDark);
    renderer.drawText("Esc", {btn2X + 55.0f, btnY + 16.0f}, 18,
                      (selected_ == 1) ? engine::Color::yellow() : engine::Color::white());

    renderer.drawText("Left/Right: select", {boxX + 30.0f, boxY + boxH - 18.0f}, 12,
                      engine::Color(120, 120, 140, 255));
}
