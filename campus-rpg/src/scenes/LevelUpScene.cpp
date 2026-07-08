#include "LevelUpScene.h"
#include "GameManager.h"
#include "Character.h"
#include "Persona.h"

LevelUpScene::LevelUpScene(std::string returnTextureId)
    : returnTextureId_(std::move(returnTextureId))
{
}

void LevelUpScene::handleInput(engine::IInput &input)
{
    if (input.wasKeyJustPressed(engine::Key::Enter) ||
        input.wasKeyJustPressed(engine::Key::Space) ||
        input.wasKeyJustPressed(engine::Key::Escape))
    {
        GameManager::instance().character().clearLevelUpSnapshot();
        GameManager::instance().enterScene(SceneType::Town);
    }
}

void LevelUpScene::update(float /*deltaTime*/)
{
}

void LevelUpScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    // Darkened background
    renderer.drawTexture(returnTextureId_, {0, 0, 800, 600});
    renderer.drawRect({0, 0, 800, 600}, engine::Color(10, 10, 25, 220));

    auto &character = GameManager::instance().character();
    const auto &snap = character.levelUpSnapshot();

    // ---- Outer golden frame (panel) ----
    const float panelX = 40.0f, panelY = 40.0f;
    const float panelW = 720.0f, panelH = 520.0f;
    // Shadow
    renderer.drawRect({panelX + 6, panelY + 6, panelW, panelH}, engine::Color(0, 0, 0, 120));
    // Panel bg
    renderer.drawRect({panelX, panelY, panelW, panelH}, engine::Color(25, 25, 45, 240));
    // Border lines (gold)
    engine::Color gold(220, 190, 60, 255);
    renderer.drawRect({panelX, panelY, panelW, 4}, gold);
    renderer.drawRect({panelX, panelY + panelH - 4, panelW, 4}, gold);
    renderer.drawRect({panelX, panelY, 4, panelH}, gold);
    renderer.drawRect({panelX + panelW - 4, panelY, 4, panelH}, gold);

    // ---- Title ----
    renderer.drawRect({panelX + 210, panelY + 14, 300, 44}, engine::Color(0, 0, 0, 180));
    renderer.drawText("LEVEL UP!", {panelX + 290, panelY + 24}, 30, engine::Color(255, 215, 0, 255));
    // Title underline
    renderer.drawRect({panelX + 250, panelY + 56, 200, 2}, gold);

    // ---- Left panel: portrait area ----
    float leftX = panelX + 30, leftY = panelY + 80;
    float leftW = 280, leftH = 400;
    // Portrait bg
    renderer.drawRect({leftX, leftY, leftW, leftH}, engine::Color(35, 35, 55, 220));
    // Portrait border (double line for glow effect)
    renderer.drawRect({leftX - 2, leftY - 2, leftW + 4, 3}, gold);
    renderer.drawRect({leftX - 2, leftY + leftH - 1, leftW + 4, 3}, gold);
    renderer.drawRect({leftX - 2, leftY - 2, 3, leftH + 4}, gold);
    renderer.drawRect({leftX + leftW - 1, leftY - 2, 3, leftH + 4}, gold);

    // Hero portrait
    std::string heroTex = "hero_" + std::to_string(GameManager::instance().selectedHeroIndex());
    renderer.drawTexture(heroTex, {leftX + 20, leftY + 20, leftW - 40, leftH - 100});

    // Character name
    renderer.drawText(character.name(), {leftX + 80, leftY + leftH - 60}, 22, engine::Color(200, 220, 255, 255));
    // Level badge
    renderer.drawRect({leftX + 90, leftY + leftH - 34, 100, 26}, engine::Color(60, 50, 20, 220));
    renderer.drawText("Lv." + std::to_string(snap.newLevel), {leftX + 115, leftY + leftH - 30}, 18, gold);

    // ---- Right panel: stats area ----
    float rightX = panelX + 340, rightY = panelY + 80;
    float rightW = 350, rightH = 400;
    // Stats bg
    renderer.drawRect({rightX, rightY, rightW, rightH}, engine::Color(30, 30, 50, 220));
    // Stats border
    renderer.drawRect({rightX, rightY, rightW, 2}, engine::Color(180, 160, 50, 200));
    renderer.drawRect({rightX, rightY + rightH - 2, rightW, 2}, engine::Color(180, 160, 50, 200));

    // Stats section title
    renderer.drawText("Attribute Growth", {rightX + 95, rightY + 14}, 22, engine::Color(255, 215, 0, 255));
    renderer.drawRect({rightX + 80, rightY + 42, 190, 1}, engine::Color(180, 160, 50, 150));

    // Draw stat rows
    float rowY = rightY + 56;
    const int rowSize = 18;
    const float rowStep = 36;

    auto drawRow = [&](const std::string &label, int oldVal, int newVal)
    {
        int delta = newVal - oldVal;
        // Label
        renderer.drawText(label, {rightX + 20, rowY}, rowSize, engine::Color(200, 200, 220, 255));
        // Old value (dim)
        renderer.drawText(std::to_string(oldVal), {rightX + 170, rowY}, rowSize, engine::Color(150, 150, 150, 255));
        // Arrow
        renderer.drawText(">", {rightX + 210, rowY}, rowSize, engine::Color(255, 215, 0, 255));
        // New value (bright)
        renderer.drawText(std::to_string(newVal), {rightX + 230, rowY}, rowSize, engine::Color::white());
        // Delta (green)
        if (delta > 0)
        {
            renderer.drawText("(+" + std::to_string(delta) + ")",
                               {rightX + 275, rowY}, rowSize, engine::Color(0, 255, 100, 255));
        }
        rowY += rowStep;
    };

    drawRow("Max HP", snap.oldMaxHp, snap.newMaxHp);
    drawRow("Max SP", snap.oldMaxSp, snap.newMaxSp);

    // ---- Bottom hint bar ----
    float hintY = panelY + panelH - 36;
    renderer.drawRect({panelX + 200, hintY, 320, 28}, engine::Color(0, 0, 0, 180));
    renderer.drawText("Press Enter or Space to continue",
                       {panelX + 235, hintY + 6}, 16, engine::Color(200, 200, 200, 255));
}
