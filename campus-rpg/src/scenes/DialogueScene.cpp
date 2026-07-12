#include "DialogueScene.h"
#include "GameManager.h"
#include "SocialLinkManager.h"
#include "SocialLink.h"

namespace
{
    // Draw a refined double-line border: outer thin bright + inner thin dark.
    void drawBorder(engine::IRenderer &r, float x, float y, float w, float h,
                    engine::Color bright, engine::Color dark)
    {
        // Outer bright line
        r.drawRect({x, y, w, 2}, bright);
        r.drawRect({x, y + h - 2, w, 2}, bright);
        r.drawRect({x, y, 2, h}, bright);
        r.drawRect({x + w - 2, y, 2, h}, bright);
        // Inner dark line (1px inset)
        r.drawRect({x + 3, y + 3, w - 6, 1}, dark);
        r.drawRect({x + 3, y + h - 4, w - 6, 1}, dark);
        r.drawRect({x + 3, y + 3, 1, h - 6}, dark);
        r.drawRect({x + w - 4, y + 3, 1, h - 6}, dark);
    }

    // Draw a soft drop shadow below-right of a rect.
    void drawShadow(engine::IRenderer &r, float x, float y, float w, float h, float offset = 4.0f)
    {
        r.drawRect({x + offset, y + offset, w, h}, engine::Color(0, 0, 0, 60));
    }

    // Draw a panel: shadow + semi-transparent fill + double border.
    void drawPanel(engine::IRenderer &r, float x, float y, float w, float h,
                   engine::Color fill, engine::Color bright, engine::Color dark)
    {
        drawShadow(r, x, y, w, h);
        r.drawRect({x, y, w, h}, fill);
        drawBorder(r, x, y, w, h, bright, dark);
    }

    // Wrap text so no line exceeds maxChars characters.
    // Breaks at spaces when possible; inserts '\n' between lines.
    std::string wrapText(const std::string &text, size_t maxChars)
    {
        std::string result;
        size_t pos = 0;
        while (pos < text.size())
        {
            if (pos + maxChars >= text.size())
            {
                result += text.substr(pos);
                break;
            }
            // Find last space within the limit
            size_t breakPos = text.rfind(' ', pos + maxChars);
            if (breakPos == std::string::npos || breakPos <= pos)
            {
                // No space found, hard break
                breakPos = pos + maxChars;
            }
            result += text.substr(pos, breakPos - pos);
            result += "\n";
            pos = (breakPos < text.size() && text[breakPos] == ' ') ? breakPos + 1 : breakPos;
        }
        return result;
    }
} // namespace

void DialogueScene::handleInput(engine::IInput &input)
{
    if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E) ||
        input.wasKeyJustPressed(engine::Key::Escape))
    {
        GameManager::instance().enterScene(GameManager::instance().isNight() ? SceneType::Night : SceneType::Town);
    }
}

void DialogueScene::update(float /*deltaTime*/)
{
    if (firstFrame_)
    {
        firstFrame_ = false;

        TileMap &map = GameManager::instance().currentMap();
        Entity *player = nullptr;
        for (const auto &e : map.entities())
        {
            if (e && e->type() == "player")
            {
                player = e.get();
                break;
            }
        }

        if (player)
        {
            engine::Rect area{player->position().x - 28.0f, player->position().y - 28.0f, 56.0f, 56.0f};
            Entity *npc = map.firstEntityAt(area, "npc");
            if (npc)
                npcId_ = static_cast<NpcEntity *>(npc)->socialLinkId();
        }

        // Pick the NPC portrait from the SocialLink's stored portraitId (the
        // pool NPCs carry a randomly-assigned texture id).
        if (!npcId_.empty())
        {
            const SocialLink *link = GameManager::instance().socialLinkManager().getLink(npcId_);
            if (link && !link->portraitId().empty())
            {
                npcTexId_ = link->portraitId();
                hasNpcTex_ = true;
            }
            else if (npcId_.size() > 3 && npcId_.substr(0, 3) == "sl_")
            {
                npcTexId_ = "npc_" + npcId_.substr(3);
                hasNpcTex_ = true;
            }
        }

        int heroIdx = GameManager::instance().selectedHeroIndex();
        heroTexId_ = "hero_" + std::to_string(heroIdx);
        hasHeroTex_ = true;

        playerName_ = GameManager::instance().character().name();

        int beforeRank = 0;
        if (const SocialLink *before = GameManager::instance().socialLinkManager().getLink(npcId_))
            beforeRank = before->rank();

        if (!npcId_.empty())
            dialogueText_ = GameManager::instance().talkToNpc(npcId_);

        const SocialLink *after = GameManager::instance().socialLinkManager().getLink(npcId_);
        if (after && after->rank() > beforeRank)
        {
            showRankUpBanner_ = true;
            rankUpTimer_ = kRankUpDuration;
        }
    }

    if (showRankUpBanner_)
    {
        rankUpTimer_ -= 0.016f;
        if (rankUpTimer_ <= 0.0f)
            showRankUpBanner_ = false;
    }
}

void DialogueScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    SocialLink *link = GameManager::instance().socialLinkManager().getLink(npcId_);
    std::string npcName = link ? link->name() : "???";
    int rank = link ? link->rank() : 0;

    // ---- Background: keep the active map backdrop while talking ----
    renderer.drawTexture(GameManager::instance().onSecondMap() ? "town_bg2" : "town_bg", {0, 0, 800, 600});

    // Subtle dark overlay so UI panels pop
    renderer.drawRect({0, 0, 800, 600}, engine::Color(0, 0, 0, 50));

    // Color palette
    engine::Color panelFill(18, 18, 32, 215);
    engine::Color panelBright(120, 130, 180, 255);
    engine::Color panelDark(8, 8, 16, 200);
    engine::Color nameFill(40, 35, 60, 235);
    engine::Color accent(180, 160, 50, 255);
    engine::Color accentDark(60, 50, 20, 200);

    // ================================================================
    // RIGHT: NPC portrait panel
    // ================================================================
    float npcX = 500.0f, npcY = 55.0f, npcW = 300.0f, npcH = 510.0f;
    drawPanel(renderer, npcX, npcY, npcW, npcH, panelFill, panelBright, panelDark);

    // NPC portrait image
    float portraitY = npcY + 12.0f;
    float portraitH = npcH - 100.0f;
    if (hasNpcTex_)
        renderer.drawTexture(npcTexId_, {npcX + 12.0f, portraitY, npcW - 24.0f, portraitH});
    else
    {
        renderer.drawRect({npcX + 12.0f, portraitY, npcW - 24.0f, portraitH},
                          engine::Color(50, 40, 70, 180));
        renderer.drawText(npcName, {npcX + 70.0f, portraitY + portraitH / 2.0f}, 18, engine::Color::white());
    }

    // Inner border around portrait image
    drawBorder(renderer, npcX + 10.0f, portraitY - 2.0f, npcW - 20.0f, portraitH + 4.0f,
               engine::Color(80, 80, 110, 200), engine::Color(20, 20, 30, 200));

    // NPC name tag (inside panel, below portrait)
    float nameTagY = npcY + npcH - 72.0f;
    drawPanel(renderer, npcX + 16.0f, nameTagY, npcW - 32.0f, 32.0f,
              nameFill, accent, accentDark);
    renderer.drawText(npcName, {npcX + 28.0f, nameTagY + 7.0f}, 18, engine::Color::white());

    // Rank hearts row
    float rankY = npcY + npcH - 28.0f;
    renderer.drawText("Rank", {npcX + 16.0f, rankY}, 14, engine::Color::gray());
    drawHearts(renderer, npcX + 60.0f, rankY, rank);

    // ================================================================
    // LEFT: Protagonist portrait panel
    // ================================================================
    float heroX = 0.0f, heroY = 55.0f, heroW = 200.0f, heroH = 340.0f;
    drawPanel(renderer, heroX, heroY, heroW, heroH, panelFill, panelBright, panelDark);

    // Hero portrait
    float heroPortY = heroY + 12.0f;
    float heroPortH = heroH - 80.0f;
    if (hasHeroTex_)
        renderer.drawTexture(heroTexId_, {heroX + 12.0f, heroPortY, heroW - 24.0f, heroPortH});
    else
        renderer.drawRect({heroX + 12.0f, heroPortY, heroW - 24.0f, heroPortH},
                          engine::Color(50, 50, 70, 180));

    drawBorder(renderer, heroX + 10.0f, heroPortY - 2.0f, heroW - 20.0f, heroPortH + 4.0f,
               engine::Color(80, 80, 110, 200), engine::Color(20, 20, 30, 200));

    // Player name tag
    float pNameY = heroY + heroH - 55.0f;
    drawPanel(renderer, heroX + 14.0f, pNameY, heroW - 28.0f, 30.0f,
              nameFill, accent, accentDark);
    renderer.drawText(playerName_.empty() ? "Hero" : playerName_,
                      {heroX + 24.0f, pNameY + 6.0f}, 16, engine::Color::white());

    // ================================================================
    // CENTER: Dialogue box
    // ================================================================
    float dlgX = 200.0f, dlgY = 365.0f, dlgW = 300.0f, dlgH = 210.0f;
    drawPanel(renderer, dlgX, dlgY, dlgW, dlgH, panelFill, panelBright, panelDark);

    // NPC name tag on top edge of dialogue box (straddling the border)
    float tagW = 140.0f;
    float tagX = dlgX + 16.0f;
    float tagY = dlgY - 16.0f;
    drawShadow(renderer, tagX, tagY, tagW, 30.0f, 3.0f);
    renderer.drawRect({tagX, tagY, tagW, 30.0f}, nameFill);
    drawBorder(renderer, tagX, tagY, tagW, 30.0f, accent, accentDark);
    renderer.drawText(npcName, {tagX + 12.0f, tagY + 6.0f}, 17, engine::Color::white());

    // Dialogue text (wrapped to fit the box width)
    // Box is 300px wide, 32px padding total, font 16px (~8px/char) => ~33 chars/line
    std::string wrapped = wrapText(dialogueText_.empty() ? "\"...\"" : dialogueText_, 33);
    renderer.drawText(wrapped, {dlgX + 16.0f, dlgY + 28.0f}, 16, engine::Color::white());

    // Continue hint at bottom of dialogue box
    int remaining = GameManager::kMaxTalksPerNpc - GameManager::instance().talkCountToday(npcId_);
    renderer.drawText("Enter / Esc to continue   Today's talks left: " + std::to_string(remaining > 0 ? remaining : 0),
                      {dlgX + 16.0f, dlgY + dlgH - 22.0f}, 12, engine::Color(120, 120, 140, 255));

    // ================================================================
    // TOP CENTER: Rank-Up banner (overlay)
    // ================================================================
    if (showRankUpBanner_)
    {
        // Full screen dim
        renderer.drawRect({0, 0, 800, 600}, engine::Color(0, 0, 0, 130));

        // Banner panel
        float bX = 200.0f, bY = 40.0f, bW = 400.0f, bH = 80.0f;
        engine::Color bannerFill(120, 30, 30, 235);
        engine::Color bannerBright(255, 220, 80, 255);
        engine::Color bannerDark(40, 10, 10, 200);
        drawPanel(renderer, bX, bY, bW, bH, bannerFill, bannerBright, bannerDark);

        renderer.drawText("RANK UP !!", {bX + 120.0f, bY + 10.0f}, 30,
                          engine::Color(255, 255, 0, 255));
        renderer.drawText(npcName + " reached Rank " + std::to_string(rank) + "!",
                          {bX + 90.0f, bY + 50.0f}, 18, engine::Color::white());

        // Pulsing glow border
        unsigned char glow = static_cast<unsigned char>(
            120 + 100.0f * (rankUpTimer_ / kRankUpDuration));
        renderer.drawRect({bX - 2, bY - 2, bW + 4, bH + 4},
                          engine::Color(255, 200, 0, glow));
    }
}

void DialogueScene::drawHearts(engine::IRenderer &renderer, float x, float y, int rank) const
{
    int fullHearts = rank / 2;
    bool hasHalf = (rank % 2) != 0;

    for (int i = 0; i < 5; ++i)
    {
        float heartX = x + static_cast<float>(i) * 22.0f;
        // Heart background slot (dark)
        renderer.drawRect({heartX - 1, y - 1, 20.0f, 20.0f}, engine::Color(20, 20, 30, 200));

        if (i < fullHearts)
        {
            // Full heart - bright red
            renderer.drawRect({heartX, y, 18.0f, 18.0f}, engine::Color(255, 60, 60, 240));
            // Highlight pixel
            renderer.drawRect({heartX + 3, y + 3, 4, 4}, engine::Color(255, 180, 180, 180));
        }
        else if (i == fullHearts && hasHalf)
        {
            // Half heart - left red, right dark
            renderer.drawRect({heartX, y, 9.0f, 18.0f}, engine::Color(255, 60, 60, 240));
            renderer.drawRect({heartX + 9.0f, y, 9.0f, 18.0f}, engine::Color(60, 60, 70, 200));
            renderer.drawRect({heartX + 2, y + 3, 3, 4}, engine::Color(255, 180, 180, 180));
        }
        else
        {
            // Empty heart - dark gray
            renderer.drawRect({heartX, y, 18.0f, 18.0f}, engine::Color(60, 60, 70, 200));
        }
    }
}
