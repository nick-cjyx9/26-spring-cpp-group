#include "GameCompleteScene.h"
#include "GameManager.h"
#include "Character.h"

void GameCompleteScene::handleInput(engine::IInput &input)
{
    if (!showContinue_)
        return;

    if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
    {
        GameManager::instance().enterScene(SceneType::Title);
    }
}

void GameCompleteScene::update(float deltaTime)
{
    timer_ += deltaTime;
    if (timer_ >= 2.0f)
    {
        showContinue_ = true;
    }
}

void GameCompleteScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    // Celebratory background gradient
    renderer.drawRect({0, 0, 800, 600}, engine::Color(10, 5, 30, 255));

    // Title text
    renderer.drawText("Congratulations!", {250, 150}, 48, engine::Color(255, 215, 0, 255));
    renderer.drawText("You have defeated the Shadow Overlord!", {180, 230}, 24, engine::Color::white());

    // Player stats
    auto &character = GameManager::instance().character();
    renderer.drawText("Character: " + character.name(), {280, 310}, 22, engine::Color::cyan());
    renderer.drawText("Level: " + std::to_string(character.level()), {310, 350}, 22, engine::Color::cyan());
    renderer.drawText("Gold: " + std::to_string(character.gold()), {310, 390}, 22, engine::Color::yellow());

    // Completion message
    renderer.drawText("You have saved the town from the shadows!", {190, 450}, 20, engine::Color(200, 200, 255, 255));
    renderer.drawText("Thank you for playing!", {260, 490}, 20, engine::Color(200, 200, 255, 255));

    if (showContinue_)
    {
        float alpha = static_cast<int>((timer_ - 2.0f) * 2.0f) % 2 == 0 ? 255 : 180;
        renderer.drawText("Press Enter to return to title", {240, 540}, 20, engine::Color(255, 255, 255, static_cast<unsigned char>(alpha)));
    }
}