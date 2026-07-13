#include "QuestScene.h"
#include "GameManager.h"
#include "QuestManager.h"
#include "Quest.h"
#include "Character.h"
#include "Inventory.h"

#include <algorithm>
#include <sstream>

namespace
{
    // Draw a refined double-line border: outer thin bright + inner thin dark.
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
        r.drawRect({x + 4, y + 4, w, h}, engine::Color(0, 0, 0, 60));
        r.drawRect({x, y, w, h}, fill);
        drawBorder(r, x, y, w, h, bright, dark);
    }

    std::string formatReward(int gold, int exp)
    {
        return "Reward: " + std::to_string(gold) + " G, " + std::to_string(exp) + " EXP";
    }
} // namespace

void QuestScene::handleInput(engine::IInput &input)
{
    auto &qm = GameManager::instance().questManager();

    if (input.wasKeyJustPressed(engine::Key::Left) || input.wasKeyJustPressed(engine::Key::A) ||
        input.wasKeyJustPressed(engine::Key::Right) || input.wasKeyJustPressed(engine::Key::D))
    {
        if (input.wasKeyJustPressed(engine::Key::Left) || input.wasKeyJustPressed(engine::Key::A))
        {
            currentTab_ = (currentTab_ == QuestTab::Available) ? QuestTab::Completed
                                                               : static_cast<QuestTab>(static_cast<int>(currentTab_) - 1);
        }
        else
        {
            currentTab_ = (currentTab_ == QuestTab::Completed) ? QuestTab::Available
                                                                : static_cast<QuestTab>(static_cast<int>(currentTab_) + 1);
        }
        selectedIndex_ = 0;
        scrollOffset_ = 0;
        message_.clear();
        return;
    }

    std::vector<Quest *> quests;
    if (currentTab_ == QuestTab::Available)
        quests = qm.availableQuests();
    else if (currentTab_ == QuestTab::Active)
        quests = qm.acceptedQuests();
    else
        quests = qm.completedQuests();

    // Sort by priority (lower value = higher in the list)
    std::sort(quests.begin(), quests.end(), [](Quest *a, Quest *b) {
        if (!a || !b) return false;
        return a->priority() < b->priority();
    });

    if (!quests.empty())
    {
        // Mouse wheel scrolling
        int scroll = input.consumeScrollDelta();
        if (scroll != 0)
        {
            selectedIndex_ -= scroll;
            if (selectedIndex_ < 0)
                selectedIndex_ = 0;
            if (selectedIndex_ >= static_cast<int>(quests.size()))
                selectedIndex_ = static_cast<int>(quests.size()) - 1;
        }

        if (input.wasKeyJustPressed(engine::Key::Up) || input.wasKeyJustPressed(engine::Key::W))
            selectedIndex_ = (selectedIndex_ - 1 + static_cast<int>(quests.size())) % static_cast<int>(quests.size());
        if (input.wasKeyJustPressed(engine::Key::Down) || input.wasKeyJustPressed(engine::Key::S))
            selectedIndex_ = (selectedIndex_ + 1) % static_cast<int>(quests.size());

        // Ensure scrollOffset follows selectedIndex
        if (selectedIndex_ < scrollOffset_)
            scrollOffset_ = selectedIndex_;
        if (selectedIndex_ >= scrollOffset_ + kVisibleCount)
            scrollOffset_ = selectedIndex_ - kVisibleCount + 1;
        // Clamp scrollOffset
        int maxOffset = std::max(0, static_cast<int>(quests.size()) - kVisibleCount);
        if (scrollOffset_ > maxOffset)
            scrollOffset_ = maxOffset;
        if (scrollOffset_ < 0)
            scrollOffset_ = 0;

        if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
        {
            if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(quests.size()))
            {
                Quest *q = quests[selectedIndex_];
                if (q)
                {
                    if (currentTab_ == QuestTab::Available)
                    {
                        if (qm.acceptQuest(q->id()))
                        {
                            message_ = "Quest accepted: " + q->name();
                            messageTimer_ = kMessageDuration;
                            selectedIndex_ = 0;
                            scrollOffset_ = 0;
                        }
                    }
                    else if (currentTab_ == QuestTab::Active)
                    {
                        // Check if collectible quest can be completed
                        bool canComplete = false;
                        if (q->type() == QuestType::Collect && !q->targetItemId().empty())
                        {
                            int count = GameManager::instance().inventory().countItem(q->targetItemId());
                            canComplete = count >= q->targetCount();
                        }
                        else if (q->type() == QuestType::Kill)
                        {
                            canComplete = q->currentProgress() >= q->targetCount();
                        }

                        if (canComplete)
                        {
                            if (q->type() == QuestType::Collect && !q->targetItemId().empty())
                            {
                                GameManager::instance().inventory().removeItemById(q->targetItemId(), q->targetCount());
                            }
                            q->complete(); // Mark as completed before rewarding
                            GameManager::instance().character().addGold(q->rewardGold());
                            GameManager::instance().character().gainExp(q->rewardExp());
                            qm.rewardQuest(q->id());
                            message_ = "Quest completed: " + q->name() + "!";
                            messageTimer_ = kMessageDuration;
                            selectedIndex_ = 0;
                            scrollOffset_ = 0;
                        }
                        else
                        {
                            message_ = "Quest not yet complete.";
                            messageTimer_ = kMessageDuration;
                        }
                    }
                    else if (currentTab_ == QuestTab::Completed)
                    {
                        if (qm.rewardQuest(q->id()))
                        {
                            GameManager::instance().character().addGold(q->rewardGold());
                            GameManager::instance().character().gainExp(q->rewardExp());
                            message_ = "Reward claimed: " + q->name() + "!";
                            messageTimer_ = kMessageDuration;
                            selectedIndex_ = 0;
                            scrollOffset_ = 0;
                        }
                    }
                }
            }
        }
    }

    if (input.wasKeyJustPressed(engine::Key::Escape))
    {
        GameManager::instance().enterScene(GameManager::instance().isNight() ? SceneType::Night : SceneType::Town);
    }
}

void QuestScene::update(float deltaTime)
{
    if (messageTimer_ > 0.0f)
    {
        messageTimer_ -= deltaTime;
        if (messageTimer_ <= 0.0f)
        {
            messageTimer_ = 0.0f;
            message_.clear();
        }
    }
}

void QuestScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    auto &qm = GameManager::instance().questManager();
    std::vector<Quest *> quests;
    if (currentTab_ == QuestTab::Available)
        quests = qm.availableQuests();
    else if (currentTab_ == QuestTab::Active)
        quests = qm.acceptedQuests();
    else
        quests = qm.completedQuests();

    // Sort by priority (lower value = higher in the list)
    std::sort(quests.begin(), quests.end(), [](Quest *a, Quest *b) {
        if (!a || !b) return false;
        return a->priority() < b->priority();
    });

    // Background
    renderer.drawTexture(GameManager::instance().onSecondMap() ? "town_bg2" : "town_bg", {0, 0, 800, 600});
    renderer.drawRect({0, 0, 800, 600}, engine::Color(0, 0, 0, 180));

    // Title
    renderer.drawText("Quest Log", {330, 25}, 32, engine::Color::white());

    // Tabs
    engine::Color tabColors[3] = {engine::Color::gray(), engine::Color::gray(), engine::Color::gray()};
    tabColors[static_cast<int>(currentTab_)] = engine::Color::yellow();
    renderer.drawText("Available", {150, 70}, 18, tabColors[0]);
    renderer.drawText("Active", {360, 70}, 18, tabColors[1]);
    renderer.drawText("Completed", {550, 70}, 18, tabColors[2]);
    renderer.drawText("A/D: switch tab | Up/Down: select | Wheel: scroll | Enter: interact | Esc: close",
                      {140, 580}, 13, engine::Color(180, 180, 180, 255));

    // Quest list
    float startY = 110.0f;
    float panelW = 720.0f;
    float panelH = 60.0f;
    float startX = 40.0f;

    if (quests.empty())
    {
        renderer.drawText("No quests in this category.", {startX + 20, startY + 20}, 18, engine::Color(150, 150, 150, 255));
    }
    else
    {
        for (int i = 0; i < kVisibleCount; ++i)
        {
            int questIdx = scrollOffset_ + i;
            if (questIdx < 0 || questIdx >= static_cast<int>(quests.size()))
                continue;

            Quest *q = quests[questIdx];
            if (!q) continue;

            float y = startY + i * (panelH + 8.0f);
            bool selected = (questIdx == selectedIndex_);

            std::string status;
            if (currentTab_ == QuestTab::Available)
                status = "[Press Enter to Accept]";
            else if (currentTab_ == QuestTab::Active)
            {
                bool canComplete = false;
                if (q->type() == QuestType::Kill)
                    canComplete = q->currentProgress() >= q->targetCount();
                else if (q->type() == QuestType::Collect && !q->targetItemId().empty())
                    canComplete = GameManager::instance().inventory().countItem(q->targetItemId()) >= q->targetCount();

                if (canComplete)
                    status = "[Ready to Complete - Press Enter]";
                else
                {
                    if (q->type() == QuestType::Kill)
                        status = "Progress: " + std::to_string(q->currentProgress()) + "/" + std::to_string(q->targetCount());
                    else
                        status = "Collect: " + std::to_string(GameManager::instance().inventory().countItem(q->targetItemId())) + "/" + std::to_string(q->targetCount());
                }
            }
            else
                status = "[Press Enter to Claim Reward]";

            drawQuestPanel(renderer, startX, y, panelW, panelH, selected,
                           q->name(), q->description(),
                           formatReward(q->rewardGold(), q->rewardExp()),
                           status, q->textColor());
        }
    }

    // Message overlay
    if (!message_.empty())
    {
        renderer.drawRect({200, 260, 400, 80}, engine::Color(20, 20, 30, 230));
        drawBorder(renderer, 200, 260, 400, 80,
                     engine::Color(255, 220, 80, 255), engine::Color(40, 10, 10, 200));
        renderer.drawText(message_, {220, 290}, 18, engine::Color::yellow());
    }
}

void QuestScene::drawQuestPanel(engine::IRenderer &renderer, float x, float y, float w, float h,
                                 bool selected, const std::string &name, const std::string &desc,
                                 const std::string &reward, const std::string &status, int textColor) const
{
    engine::Color fill = selected ? engine::Color(40, 35, 60, 240) : engine::Color(18, 18, 32, 220);
    engine::Color bright = selected ? engine::Color(255, 220, 80, 255) : engine::Color(80, 80, 110, 200);
    engine::Color dark = engine::Color(8, 8, 16, 200);

    drawPanel(renderer, x, y, w, h, fill, bright, dark);

    // Quest name (bold/larger) - use red if textColor == 1
    engine::Color nameColor = (textColor == 1) ? engine::Color(255, 50, 50, 255) : engine::Color::white();
    renderer.drawText(name, {x + 16.0f, y + 8.0f}, 16, nameColor);

    // Description
    renderer.drawText(desc, {x + 16.0f, y + 30.0f}, 12, engine::Color(180, 180, 180, 255));

    // Reward info (right side)
    renderer.drawText(reward, {x + w - 250.0f, y + 8.0f}, 12, engine::Color(255, 220, 120, 255));

    // Status text
    engine::Color statusColor = engine::Color(120, 220, 120, 255);
    if (status.find("Progress") != std::string::npos || status.find("Collect") != std::string::npos)
        statusColor = engine::Color(255, 200, 80, 255);
    renderer.drawText(status, {x + w - 250.0f, y + 30.0f}, 12, statusColor);
}
