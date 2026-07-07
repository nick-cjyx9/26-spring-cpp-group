#include "SocialLinkScene.h"
#include "GameManager.h"
#include "SocialLinkManager.h"
#include "SocialLink.h"

#include <algorithm>

namespace
{
    void drawBorder(engine::IRenderer &r, float x, float y, float w, float h,
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

    void drawPanel(engine::IRenderer &r, float x, float y, float w, float h,
                   engine::Color fill, engine::Color bright, engine::Color dark)
    {
        r.drawRect({x + 4, y + 4, w, h}, engine::Color(0, 0, 0, 50));
        r.drawRect({x, y, w, h}, fill);
        drawBorder(r, x, y, w, h, bright, dark);
    }

    // Convert social link id "sl_yosuke" -> texture id "npc_yosuke"
    std::string npcTexId(const std::string &socialLinkId)
    {
        if (socialLinkId.size() > 3 && socialLinkId.substr(0, 3) == "sl_")
            return "npc_" + socialLinkId.substr(3);
        return "npc_" + socialLinkId;
    }
} // namespace

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
    renderer.drawRect({0, 0, 800, 600}, engine::Color(0, 0, 0, 60));

    // Title bar
    engine::Color panelFill(18, 18, 32, 220);
    engine::Color panelBright(120, 130, 180, 255);
    engine::Color panelDark(8, 8, 16, 200);
    engine::Color accent(180, 160, 50, 255);
    engine::Color accentDark(60, 50, 20, 200);

    drawPanel(renderer, 30, 15, 740, 50, panelFill, accent, accentDark);
    renderer.drawText("Social Link Panel", {310, 25}, 26, engine::Color::yellow());

    auto links = GameManager::instance().socialLinkManager().allLinks();
    if (links.empty())
    {
        renderer.drawText("No Social Links available.", {250, 300}, 20, engine::Color::gray());
        renderer.drawText("Esc: back", {340, 570}, 16, engine::Color::gray());
        return;
    }

    // Draw list items
    for (size_t i = 0; i < links.size(); ++i)
    {
        float y = 80.0f + static_cast<float>(i) * kItemHeight - scrollY_;
        if (y < -kItemHeight || y > 620.0f)
            continue;

        const SocialLink *link = links[i];
        if (!link)
            continue;

        bool selected = (static_cast<int>(i) == selectedIndex_);

        // Row background panel
        engine::Color rowBright = selected ? engine::Color(255, 220, 80, 255) : panelBright;
        engine::Color rowFill = selected ? engine::Color(50, 40, 70, 225) : panelFill;
        drawPanel(renderer, 40.0f, y, 720.0f, kItemHeight - 10.0f, rowFill, rowBright, panelDark);

        // ---- NPC portrait (left side) ----
        float portX = 55.0f, portY = y + 8.0f, portW = 85.0f, portH = 95.0f;
        // Portrait background
        renderer.drawRect({portX, portY, portW, portH}, engine::Color(35, 30, 50, 200));
        // Draw NPC texture, fitted into the portrait frame
        std::string texId = npcTexId(link->id());
        renderer.drawTexture(texId, {portX + 3.0f, portY + 3.0f, portW - 6.0f, portH - 6.0f});
        // Portrait border
        drawBorder(renderer, portX, portY, portW, portH,
                   selected ? accent : panelBright, panelDark);

        // ---- Name and arcana ----
        float textX = portX + portW + 20.0f;
        renderer.drawText(link->name(), {textX, y + 12.0f}, 22,
                          selected ? engine::Color::yellow() : engine::Color::white());
        renderer.drawText("Arcana: " + link->arcana(), {textX, y + 42.0f}, 16, engine::Color::cyan());

        // ---- Hearts ----
        renderer.drawText("Bond:", {textX, y + 72.0f}, 14, engine::Color::gray());
        drawHearts(renderer, textX + 50.0f, y + 72.0f, link->rank());

        // ---- Rank number (right side) ----
        std::string rankStr = "Rank " + std::to_string(link->rank()) + " / 10";
        renderer.drawText(rankStr, {620.0f, y + 35.0f}, 20,
                          selected ? engine::Color::yellow() : engine::Color(200, 200, 200, 255));

        // Points to next rank
        if (!link->isMaxRank())
        {
            renderer.drawText(std::to_string(link->points()) + " / " +
                                  std::to_string(link->pointsToNextRank()) + " pts",
                              {620.0f, y + 65.0f}, 14, engine::Color::gray());
        }
        else
        {
            renderer.drawText("MAX", {640.0f, y + 65.0f}, 16, engine::Color(255, 200, 50, 255));
        }
    }

    // Scroll indicator
    int total = static_cast<int>(links.size());
    if (total > 4)
    {
        float barHeight = 400.0f;
        float thumbHeight = barHeight * 4.0f / static_cast<float>(total);
        float maxScroll = (total - 4) * kItemHeight;
        float scrollRatio = maxScroll > 0 ? scrollY_ / maxScroll : 0;
        float thumbY = 80.0f + (barHeight - thumbHeight) * scrollRatio;
        renderer.drawRect({765.0f, 80.0f, 6.0f, barHeight}, engine::Color(40, 40, 50, 180));
        renderer.drawRect({765.0f, thumbY, 6.0f, thumbHeight}, engine::Color(200, 200, 200, 200));
    }

    // Instructions
    renderer.drawText("Up/Down: select   Esc: back", {280, 575}, 16, engine::Color::gray());
}

void SocialLinkScene::drawHearts(engine::IRenderer &renderer, float x, float y, int rank) const
{
    int fullHearts = rank / 2;
    bool hasHalf = (rank % 2) != 0;

    for (int i = 0; i < 5; ++i)
    {
        float heartX = x + static_cast<float>(i) * 24.0f;
        // Heart background slot
        renderer.drawRect({heartX - 1, y - 1, 20.0f, 20.0f}, engine::Color(20, 20, 30, 200));

        if (i < fullHearts)
        {
            renderer.drawRect({heartX, y, 18.0f, 18.0f}, engine::Color(255, 60, 60, 240));
            renderer.drawRect({heartX + 3, y + 3, 4, 4}, engine::Color(255, 180, 180, 180));
        }
        else if (i == fullHearts && hasHalf)
        {
            renderer.drawRect({heartX, y, 9.0f, 18.0f}, engine::Color(255, 60, 60, 240));
            renderer.drawRect({heartX + 9.0f, y, 9.0f, 18.0f}, engine::Color(60, 60, 70, 200));
            renderer.drawRect({heartX + 2, y + 3, 3, 4}, engine::Color(255, 180, 180, 180));
        }
        else
        {
            renderer.drawRect({heartX, y, 18.0f, 18.0f}, engine::Color(60, 60, 70, 200));
        }
    }
}
