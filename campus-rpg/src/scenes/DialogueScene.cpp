#include "DialogueScene.h"
#include "GameManager.h"
#include "SocialLinkManager.h"
#include "SocialLink.h"
#include "QuestManager.h"
#include "Quest.h"
#include "Inventory.h"

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
    if (showRankUpBanner_)
    {
        if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
        {
            showRankUpBanner_ = false;
        }
        return;
    }

    if (showQuestUi_ && !npcQuests_.empty())
    {
        // Quest navigation
        if (input.wasKeyJustPressed(engine::Key::Up) || input.wasKeyJustPressed(engine::Key::W))
        {
            questSelection_ = (questSelection_ - 1 + static_cast<int>(npcQuests_.size())) % static_cast<int>(npcQuests_.size());
        }
        if (input.wasKeyJustPressed(engine::Key::Down) || input.wasKeyJustPressed(engine::Key::S))
        {
            questSelection_ = (questSelection_ + 1) % static_cast<int>(npcQuests_.size());
        }
        bool questActionTaken = false;
        if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
        {
            if (questSelection_ >= 0 && questSelection_ < static_cast<int>(npcQuests_.size()))
            {
                auto &info = npcQuests_[questSelection_];
                if (info.state == NpcQuestInfo::State::Available)
                {
                    tryAcceptQuest(questSelection_);
                    questActionTaken = true;
                }
                else if (info.state == NpcQuestInfo::State::Completable)
                {
                    tryCompleteQuest(questSelection_);
                    questActionTaken = true;
                }
            }
        }
        if (input.wasKeyJustPressed(engine::Key::Escape))
        {
            showQuestUi_ = false;
            GameManager::instance().enterScene(GameManager::instance().isNight() ? SceneType::Night : SceneType::Town);
            return;
        }
        if (questActionTaken)
            return;
    }

    // Multi-line dialogue: Enter/E advances to next line, Escape exits immediately
    if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
    {
        if (hasMoreDialogues_)
        {
            advanceDialogue();
        }
        else
        {
            GameManager::instance().enterScene(GameManager::instance().isNight() ? SceneType::Night : SceneType::Town);
        }
    }
    if (input.wasKeyJustPressed(engine::Key::Escape))
    {
        GameManager::instance().enterScene(GameManager::instance().isNight() ? SceneType::Night : SceneType::Town);
    }
}

void DialogueScene::advanceDialogue()
{
    currentDialogueIndex_++;
    if (currentDialogueIndex_ < static_cast<int>(currentDialogues_.size()))
    {
        dialogueText_ = currentDialogues_[currentDialogueIndex_];
        hasMoreDialogues_ = (currentDialogueIndex_ + 1 < static_cast<int>(currentDialogues_.size()));
    }
    else
    {
        hasMoreDialogues_ = false;
        dialogueText_ = currentDialogues_.empty() ? "" : currentDialogues_.back();
    }
}

void DialogueScene::refreshNpcQuests()
{
    npcQuests_.clear();
    if (npcId_.empty())
        return;

    auto *qm = &GameManager::instance().questManager();
    std::vector<Quest *> quests = qm->questsForNpc(npcId_);
    for (Quest *q : quests)
    {
        if (!q)
            continue;
        NpcQuestInfo info;
        info.quest = q;
        if (q->isRewarded())
            info.state = NpcQuestInfo::State::Rewarded;
        else if (q->isCompleted())
            info.state = NpcQuestInfo::State::Completable;
        else if (q->isAccepted())
            info.state = NpcQuestInfo::State::InProgress;
        else
            info.state = NpcQuestInfo::State::Available;
        npcQuests_.push_back(info);
    }
}

void DialogueScene::tryAcceptQuest(size_t index)
{
    if (index >= npcQuests_.size())
        return;
    Quest *q = npcQuests_[index].quest;
    if (!q)
        return;
    GameManager::instance().questManager().acceptQuest(q->id());
    npcQuests_[index].state = NpcQuestInfo::State::InProgress;
    questSubmitted_ = true;
}

void DialogueScene::tryCompleteQuest(size_t index)
{
    if (index >= npcQuests_.size())
        return;
    Quest *q = npcQuests_[index].quest;
    if (!q)
        return;
    // For collect quests, remove items from inventory first.
    if (q->type() == QuestType::Collect && !q->targetItemId().empty())
    {
        GameManager::instance().inventory().removeItemById(q->targetItemId(), q->targetCount());
    }
    q->complete(); // Mark as completed before rewarding
    // Apply rewards
    GameManager::instance().character().addGold(q->rewardGold());
    GameManager::instance().character().gainExp(q->rewardExp());
    GameManager::instance().questManager().rewardQuest(q->id());
    npcQuests_[index].state = NpcQuestInfo::State::Rewarded;
    questSubmitted_ = true;
}

std::string DialogueScene::questStatusText(const NpcQuestInfo &info) const
{
    switch (info.state)
    {
    case NpcQuestInfo::State::Available:
        return "[Available] Press Enter to accept";
    case NpcQuestInfo::State::InProgress:
        return "[In Progress]";
    case NpcQuestInfo::State::Completable:
        return "[Ready] Press Enter to complete";
    case NpcQuestInfo::State::Rewarded:
        return "[Completed]";
    default:
        return "";
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

        // Set up multi-line dialogue
        currentDialogues_.clear();
        currentDialogueIndex_ = 0;
        if (!npcId_.empty())
        {
            const SocialLink *link = GameManager::instance().socialLinkManager().getLink(npcId_);
            if (link)
            {
                const auto *rankData = link->currentRankData();
                if (rankData && !rankData->dialogues.empty())
                {
                    currentDialogues_ = rankData->dialogues;
                    dialogueText_ = currentDialogues_[0];
                    currentDialogueIndex_ = 0;
                    hasMoreDialogues_ = currentDialogues_.size() > 1;
                }
                else
                {
                    // Fallback to legacy single dialogue
                    currentDialogues_.push_back(dialogueText_);
                    hasMoreDialogues_ = false;
                }
            }
        }

        // Refresh quest state for this NPC
        refreshNpcQuests();
        showQuestUi_ = !npcQuests_.empty();
        questSelection_ = 0;
        questSubmitted_ = false;
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
    float npcX = 545.0f, npcY = 55.0f, npcW = 240.0f, npcH = 410.0f;
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
    float heroX = 15.0f, heroY = 175.0f, heroW = 175.0f, heroH = 290.0f;
    drawPanel(renderer, heroX, heroY, heroW, heroH, panelFill, panelBright, panelDark);

    // Hero portrait
    float heroPortY = heroY + 12.0f;
    float heroPortH = heroH - 80.0f;
    if (hasHeroTex_)
        renderer.drawTexture(heroTexId_, {heroX + 12.0f, heroPortY, heroW - 24.0f, heroPortH});
    else
    {
        renderer.drawRect({heroX + 12.0f, heroPortY, heroW - 24.0f, heroPortH},
                          engine::Color(50, 50, 70, 180));
    }

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
    float dlgX = 205.0f, dlgY = 365.0f, dlgW = 325.0f, dlgH = 210.0f;
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
    // Box is 325px wide, 32px padding total, font 16px (~8px/char) => ~36 chars/line
    std::string wrapped = wrapText(dialogueText_.empty() ? "\"...\"" : dialogueText_, 36);
    renderer.drawText(wrapped, {dlgX + 16.0f, dlgY + 28.0f}, 16, engine::Color::white());

    // Continue hint at bottom of dialogue box
    std::string continueHint;
    if (hasMoreDialogues_)
    {
        continueHint = "Enter to continue   (" + std::to_string(currentDialogueIndex_ + 1) + "/" + std::to_string(currentDialogues_.size()) + ")";
    }
    else
    {
        continueHint = "Enter / Esc to continue";
    }
    renderer.drawText(continueHint,
                      {dlgX + 16.0f, dlgY + dlgH - 22.0f}, 12, engine::Color(120, 120, 140, 255));

    // ================================================================
    // QUEST UI (below dialogue box)
    // ================================================================
    if (showQuestUi_ && !npcQuests_.empty())
    {
        float qY = dlgY + dlgH + 10.0f;
        float qH = 80.0f;
        drawPanel(renderer, dlgX, qY, dlgW, qH, panelFill, panelBright, panelDark);
        renderer.drawText("Quests:", {dlgX + 16.0f, qY + 8.0f}, 14, engine::Color::yellow());

        float lineY = qY + 28.0f;
        for (size_t i = 0; i < npcQuests_.size() && i < 2; ++i)
        {
            const auto &info = npcQuests_[i];
            std::string line = (info.quest ? info.quest->name() : "???") + " " + questStatusText(info);
            engine::Color color = engine::Color::white();
            if (static_cast<int>(i) == questSelection_)
            {
                renderer.drawRect({dlgX + 12.0f, lineY - 2.0f, dlgW - 24.0f, 20.0f}, engine::Color(80, 70, 40, 180));
                color = engine::Color::yellow();
            }
            renderer.drawText(line, {dlgX + 16.0f, lineY}, 12, color);
            lineY += 22.0f;
        }
    }

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
