#include "HeroSelectScene.h"
#include "GameManager.h"

#include <cctype>

HeroSelectScene::HeroSelectScene()
    : selectedHero_(0), flashTimer_(0.0f), showCursor_(true)
{
}

void HeroSelectScene::handleInput(engine::IInput &input)
{
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

    if (input.wasKeyJustPressed(engine::Key::Left))
    {
        selectedHero_ = (selectedHero_ - 1 + kHeroCount) % kHeroCount;
    }
    if (input.wasKeyJustPressed(engine::Key::Right))
    {
        selectedHero_ = (selectedHero_ + 1) % kHeroCount;
    }

    if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
    {
        tryConfirm();
    }

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
    for (float dy = -size / 2.0f; dy <= size / 2.0f; dy += 1.0f)
    {
        float width = (size / 2.0f) - std::abs(dy);
        renderer.drawRect({cx - width, cy + dy, width, 2.0f}, color);
    }
}

void HeroSelectScene::drawArrowRight(engine::IRenderer &renderer, float cx, float cy, float size, engine::Color color) const
{
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

    // ---- Top center: Create your name + input box ----
    renderer.drawText("Create your name", {310, 30}, 26, engine::Color::yellow());

    engine::Rect nameBox{250, 60, 300, 45};
    renderer.drawRect(nameBox, engine::Color(35, 35, 55, 230));
    renderer.drawRect({nameBox.x, nameBox.y, nameBox.width, 3}, engine::Color::yellow());
    renderer.drawRect({nameBox.x, nameBox.y + nameBox.height - 3, nameBox.width, 3}, engine::Color::yellow());
    renderer.drawRect({nameBox.x, nameBox.y, 3, nameBox.height}, engine::Color::yellow());
    renderer.drawRect({nameBox.x + nameBox.width - 3, nameBox.y, 3, nameBox.height}, engine::Color::yellow());

    std::string display = nameBuffer_;
    if (showCursor_)
        display += "_";
    if (display.empty())
        display = "_";
    renderer.drawText(display, {nameBox.x + 15, nameBox.y + 10}, 22, engine::Color::white());

    // ---- Center: current hero portrait (large) ----
    float centerX = 300.0f, centerY = 140.0f, centerW = 200.0f, centerH = 260.0f;
    renderer.drawRect({centerX, centerY, centerW, centerH}, engine::Color(40, 40, 60, 200));

    std::string centerTex = "hero_" + std::to_string(selectedHero_);
    renderer.drawTexture(centerTex, {centerX + 10.0f, centerY + 10.0f, centerW - 20.0f, centerH - 20.0f});

    // Hero index below center portrait
    renderer.drawText("Hero " + std::to_string(selectedHero_ + 1) + " / " + std::to_string(kHeroCount),
                      {centerX + 45.0f, centerY + centerH + 10.0f}, 18, engine::Color::gray());

    // ---- Left side: previous hero preview (small) ----
    int prevHero = (selectedHero_ - 1 + kHeroCount) % kHeroCount;
    float prevX = 90.0f, prevY = 220.0f, prevW = 100.0f, prevH = 130.0f;
    renderer.drawRect({prevX, prevY, prevW, prevH}, engine::Color(50, 45, 65, 180));
    renderer.drawTexture("hero_" + std::to_string(prevHero), {prevX + 5.0f, prevY + 5.0f, prevW - 10.0f, prevH - 10.0f});

    // ---- Right side: next hero preview (small) ----
    int nextHero = (selectedHero_ + 1) % kHeroCount;
    float nextX = 610.0f, nextY = 220.0f, nextW = 100.0f, nextH = 130.0f;
    renderer.drawRect({nextX, nextY, nextW, nextH}, engine::Color(50, 45, 65, 180));
    renderer.drawTexture("hero_" + std::to_string(nextHero), {nextX + 5.0f, nextY + 5.0f, nextW - 10.0f, nextH - 10.0f});

    // ---- Bottom: left/right switch buttons ----
    // Left button
    engine::Rect leftBtn{120, 460, 160, 50};
    renderer.drawRect(leftBtn, engine::Color(60, 50, 80, 220));
    drawArrowLeft(renderer, leftBtn.x + 30, leftBtn.y + 25, 20, engine::Color::white());
    renderer.drawText("Prev", {leftBtn.x + 60, leftBtn.y + 15}, 18, engine::Color::white());

    // Right button
    engine::Rect rightBtn{520, 460, 160, 50};
    renderer.drawRect(rightBtn, engine::Color(60, 50, 80, 220));
    drawArrowRight(renderer, rightBtn.x + 130, rightBtn.y + 25, 20, engine::Color::white());
    renderer.drawText("Next", {rightBtn.x + 60, rightBtn.y + 15}, 18, engine::Color::white());

    // ---- Message ----
    if (!message_.empty())
        renderer.drawText(message_, {250, 530}, 18, engine::Color::cyan());

    // ---- Instructions ----
    renderer.drawText("Left/Right: switch hero   Enter: confirm   Esc: cancel",
                      {180, 570}, 16, engine::Color::gray());
}
