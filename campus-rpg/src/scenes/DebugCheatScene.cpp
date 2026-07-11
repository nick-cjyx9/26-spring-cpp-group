#include "DebugCheatScene.h"
#include "GameManager.h"
#include "Character.h"

void DebugCheatScene::handleInput(engine::IInput &input)
{
    if (input.wasKeyJustPressed(engine::Key::Escape))
    {
        GameManager::instance().enterScene(GameManager::instance().isNight() ? SceneType::Night : SceneType::Town);
        return;
    }

    if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
        GameManager::instance().toggleInfiniteGold();
}

void DebugCheatScene::update(float /*deltaTime*/)
{
}

void DebugCheatScene::render(engine::IRenderer &renderer)
{
    renderer.clear();
    renderer.drawTexture(GameManager::instance().onSecondMap() ? "town_bg2" : "town_bg", {0, 0, 800, 600});
    renderer.drawRect({0, 0, 800, 600}, engine::Color(0, 0, 0, 180));

    renderer.drawRect({180, 130, 440, 300}, engine::Color(25, 25, 50, 240));
    renderer.drawRect({180, 130, 440, 3}, engine::Color::yellow());
    renderer.drawRect({180, 427, 440, 3}, engine::Color::yellow());
    renderer.drawRect({180, 130, 3, 300}, engine::Color::yellow());
    renderer.drawRect({617, 130, 3, 300}, engine::Color::yellow());

    auto &gm = GameManager::instance();
    renderer.drawText("Debug Cheats", {305, 155}, 28, engine::Color::white());
    renderer.drawText("Temporary development tools", {285, 192}, 14, engine::Color::gray());

    bool enabled = gm.infiniteGoldEnabled();
    engine::Color stateColor = enabled ? engine::Color::green() : engine::Color(180, 180, 180);
    renderer.drawRect({220, 245, 360, 58}, enabled ? engine::Color(35, 85, 35, 230) : engine::Color(35, 35, 55, 230));
    renderer.drawText(std::string("> Infinite Gold: ") + (enabled ? "ON" : "OFF"),
                      {245, 264}, 22, stateColor);
    renderer.drawText("Current gold: " + std::to_string(gm.character().gold()),
                      {245, 326}, 18, engine::Color::yellow());

    renderer.drawText("Enter/E: toggle   Esc: back", {285, 390}, 14, engine::Color::gray());
}
