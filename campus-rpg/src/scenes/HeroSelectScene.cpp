#include "HeroSelectScene.h"
#include "GameManager.h"

#include <cctype>

HeroSelectScene::HeroSelectScene()
    : selectedHero_(0), flashTimer_(0.0f), showCursor_(true)
{
}

void HeroSelectScene::handleInput(engine::IInput &input)
{
    // Typed text for name input
    std::string typed = input.consumeTypedText();
    for (char ch : typed)
    {
        if (ch == '\b')
        {
            if (!nameBuffer_.empty())
                nameBuffer_.pop_back();
        }
        else if (std::isprint(static_cast<unsigned char>(ch)) && nameBuffer_.size() < 16)
        {
            nameBuffer_ += ch;
        }
    }

    // Left / Right arrows to switch hero
    if (input.wasKeyJustPressed(engine::Key::Left) || input.wasKeyJustPressed(engine::Key::A))
    {
        selectedHero_ = (selectedHero_ - 1 + kHeroCount) % kHeroCount;
    }
    if (input.wasKeyJustPressed(engine::Key::Right) || input.wasKeyJustPressed(engine::Key::D))
    {
        selectedHero_ = (selectedHero_ + 1) % kHeroCount;
    }

    // Enter to confirm
    if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
    {
        tryConfirm();
    }

    // Escape to cancel
    if (input.wasKeyJustPressed(engine::Key::Escape))
    {
        GameManager::instance().enterScene(SceneType::Title);
    }
}

void HeroSelectScene::update(float deltaTime)
{
    flashTimer_ += deltaTime;
    if (flashTimer_ >= kFlashInterval)
    {
        flashTimer_ -= kFlashInterval;
        showCursor_ = !showCursor_;
    }
}

void HeroSelectScene::tryConfirm()
{
    if (nameBuffer_.empty())
    {
        message_ = "Please enter a name first.";
        return;
    }

    // Store selected hero index so other scenes can show the correct portrait.
    GameManager::instance().setSelectedHeroIndex(selectedHero_);

    if (GameManager::instance().createNewSave(nameBuffer_))
    {
        GameManager::instance().enterScene(SceneType::Town);
        return;
    }
    message_ = "Failed to create save.";
}

void HeroSelectScene::drawArrowLeft(engine::IRenderer &renderer, float cx, float cy, float size, engine::Color color) const
{
    // Simple triangle pointing left
    for (float dy = -size / 2.0f; dy <= size / 2.0f; dy += 1.0f)
    {
        float width = (size / 2.0f) - std::abs(dy);
        renderer.drawRect({cx - width, cy + dy, width, 2.0f}, color);
    }
}

void HeroSelectScene::drawArrowRight(engine::IRenderer &renderer, float cx, float cy, float size, engine::Color color) const
{
    // Simple triangle pointing right
    for (float dy = -size / 2.0f; dy <= size / 2.0f; dy += 1.0f)
    {
        float width = (size / 2.0f) - std::abs(dy);
        renderer.drawRect({cx, cy + dy, width, 2.0f}, color);
    }
}

void HeroSelectScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    // Background
    renderer.drawRect({0, 0, 800, 600}, engine::Color(20, 20, 35));

    // Title
    renderer.drawText("Choose Your Hero", {260, 30}, 32, engine::Color::yellow());

    // Hero portrait area (center)
    float portraitX = 280.0f;
    float portraitY = 100.0f;
    float portraitW = 240.0f;
    float portraitH = 300.0f;

    // Portrait background
    renderer.drawRect({portraitX, portraitY, portraitW, portraitH}, engine::Color(40, 40, 60, 200));

    // Draw hero texture
    std::string texId = "hero_" + std::to_string(selectedHero_);
    renderer.drawTexture(texId, {portraitX + 10.0f, portraitY + 10.0f, portraitW - 20.0f, portraitH - 20.0f});

    // Left arrow
    drawArrowLeft(renderer, portraitX - 40.0f, portraitY + portraitH / 2.0f, 30.0f, engine::Color::white());
    // Right arrow
    drawArrowRight(renderer, portraitX + portraitW + 40.0f, portraitY + portraitH / 2.0f, 30.0f, engine::Color::white());

    // Hero index indicator
    renderer.drawText("Hero " + std::to_string(selectedHero_ + 1) + " / " + std::to_string(kHeroCount),
                      {portraitX + 70.0f, portraitY + portraitH + 10.0f}, 18, engine::Color::gray());

    // Name input box (bottom center)
    float inputY = 450.0f;
    renderer.drawText("Enter your name:", {300, inputY - 30}, 20, engine::Color::white());

    engine::Rect box{250, static_cast<float>(inputY), 300, 50};
    renderer.drawRect(box, engine::Color(35, 35, 55, 230));
    renderer.drawRect({box.x, box.y, box.width, 3}, engine::Color::yellow());
    renderer.drawRect({box.x, box.y + box.height - 3, box.width, 3}, engine::Color::yellow());
    renderer.drawRect({box.x, box.y, 3, box.height}, engine::Color::yellow());
    renderer.drawRect({box.x + box.width - 3, box.y, 3, box.height}, engine::Color::yellow());

    std::string display = nameBuffer_;
    if (showCursor_)
        display += "_";
    if (display.empty())
        display = "_";
    renderer.drawText(display, {box.x + 15, box.y + 12}, 24, engine::Color::white());

    // Message
    if (!message_.empty())
        renderer.drawText(message_, {250, inputY + 70}, 18, engine::Color::cyan());

    // Instructions
    renderer.drawText("Left/Right or A/D: switch hero   Enter: confirm   Esc: cancel",
                      {180, 560}, 16, engine::Color::gray());
}
