#include "TitleScene.h"
#include "GameManager.h"

void TitleScene::handleInput(engine::IInput &input)
{
    if (input.wasKeyJustPressed(engine::Key::Up))
        selectedIndex_ = (selectedIndex_ - 1 + 3) % 3;
    if (input.wasKeyJustPressed(engine::Key::Down))
        selectedIndex_ = (selectedIndex_ + 1) % 3;

    if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
    {
        switch (selectedIndex_)
        {
        case 0:
            GameManager::instance().enterScene(SceneType::Town);
            break;
        case 1:
            // Load game placeholder for demo.
            GameManager::instance().enterScene(SceneType::Town);
            break;
        case 2:
            GameManager::instance().quit();
            break;
        }
    }
}

void TitleScene::update(float /*deltaTime*/)
{
}

void TitleScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    // Dark background.
    renderer.drawRect({0, 0, 800, 600}, engine::Color(20, 20, 35));

    // Title
    renderer.drawText("Persona 67 Garbage", {220, 180}, 42, engine::Color::white());

    const char *items[] = {"start game", "load game", "exit"};
    for (int i = 0; i < 3; ++i)
    {
        engine::Color color = (i == selectedIndex_) ? engine::Color::yellow() : engine::Color::white();
        std::string prefix = (i == selectedIndex_) ? "> " : "  ";
        renderer.drawText(prefix + items[i], {330.0f, 300.0f + i * 45.0f}, 24, color);
    }

    renderer.drawText("Use Up/Down and Enter", {280, 520}, 16, engine::Color::gray());
}
