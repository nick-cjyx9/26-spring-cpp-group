#include "SocialLinkScene.h"
#include "GameManager.h"
#include "SocialLinkManager.h"
#include "SocialLink.h"

#include <algorithm>

void SocialLinkScene::handleInput(engine::IInput &input)
{
    if (input.wasKeyJustPressed(engine::Key::Escape))
    {
        GameManager::instance().enterScene(SceneType::Town);
        return;
    }

    auto links = GameManager::instance().socialLinkManager().allLinks();
    int count = static_cast<int>(links.size());
    if (count == 0)
        return;

    if (input.wasKeyJustPressed(engine::Key::Up) || input.wasKeyJustPressed(engine::Key::W))
    {
        if (selectedIndex_ > 0)
            --selectedIndex_;
    }
    if (input.wasKeyJustPressed(engine::Key::Down) || input.wasKeyJustPressed(engine::Key::S))
    {
        if (selectedIndex_ < count - 1)
            ++selectedIndex_;
    }
}

void SocialLinkScene::update(float /*deltaTime*/)
{
    float targetScroll = selectedIndex_ * kItemHeight;
    scrollY_ += (targetScroll - scrollY_) * kScrollSmooth;
}

void SocialLinkScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    // Background
    renderer.drawTexture("town_bg", {0, 0, 800, 600});

    // Title
    renderer.drawText("Social Link Panel", {280, 20}, 28, engine::Color::yellow());

    auto links = GameManager::instance().socialLinkManager().allLinks();
    if (links.empty())
    {
        renderer.drawText("No Social Links available.", {250, 300}, 20, engine::Color::gray());
        renderer.drawText("Esc: back", {340, 570}, 16, engine::Color::gray());
        return;
    }

    // Draw list
    for (size_t i = 0; i < links.size(); ++i)
    {
        float y = 80.0f + static_cast<float>(i) * kItemHeight - scrollY_;
        if (y < -kItemHeight || y > 620.0f)
            continue; // cull off-screen

        const SocialLink *link = links[i];
        if (!link)
            continue;

        // Selection highlight
        if (static_cast<int>(i) == selectedIndex_)
        {
            renderer.drawRect({50.0f, y, 700.0f, kItemHeight - 10.0f}, engine::Color(80, 60, 100, 200));
        }

        // Avatar placeholder (left side)
        renderer.drawRect({70.0f, y + 10.0f, 80.0f, 90.0f}, engine::Color(60, 40, 80, 200));
        renderer.drawText(link->name(), {80.0f, y + 40.0f}, 14, engine::Color::white());

        // Name and arcana
        renderer.drawText(link->name(), {170.0f, y + 15.0f}, 22, engine::Color::white());
        renderer.drawText(link->arcana(), {170.0f, y + 45.0f}, 16, engine::Color::cyan());

        // Hearts (ai xin shu liang)
        drawHearts(renderer, 170.0f, y + 72.0f, link->rank());

        // Rank number
        renderer.drawText("Rank " + std::to_string(link->rank()), {600.0f, y + 30.0f}, 18, engine::Color::yellow());
    }

    // Scroll indicator
    int total = static_cast<int>(links.size());
    if (total > 4)
    {
        float barHeight = 400.0f;
        float thumbHeight = barHeight * 4.0f / static_cast<float>(total);
        float thumbY = 80.0f + (barHeight - thumbHeight) * (scrollY_ / ((total - 4) * kItemHeight));
        renderer.drawRect({760.0f, thumbY, 8.0f, thumbHeight}, engine::Color(200, 200, 200, 180));
    }

    // Instructions
    renderer.drawText("Up/Down: select  Esc: back", {250, 570}, 16, engine::Color::gray());
}

void SocialLinkScene::drawHearts(engine::IRenderer &renderer, float x, float y, int rank) const
{
    // Map rank 0..10 to hearts 0..5 (each heart = 2 ranks)
    int fullHearts = rank / 2;
    bool hasHalf = (rank % 2) != 0;

    for (int i = 0; i < 5; ++i)
    {
        float heartX = x + static_cast<float>(i) * 26.0f;
        if (i < fullHearts)
        {
            // Full heart - bright red
            renderer.drawRect({heartX, y, 20.0f, 20.0f}, engine::Color(255, 50, 50, 230));
        }
        else if (i == fullHearts && hasHalf)
        {
            // Half heart - left half red, right half gray
            renderer.drawRect({heartX, y, 10.0f, 20.0f}, engine::Color(255, 50, 50, 230));
            renderer.drawRect({heartX + 10.0f, y, 10.0f, 20.0f}, engine::Color(100, 100, 100, 150));
        }
        else
        {
            // Empty heart - dark gray
            renderer.drawRect({heartX, y, 20.0f, 20.0f}, engine::Color(100, 100, 100, 150));
        }
    }
}
