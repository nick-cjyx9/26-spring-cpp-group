#pragma once

#include <string>

enum class QuestType
{
    Kill,
    Collect
};

class Quest
{
public:
    Quest() = default;
    Quest(std::string id, std::string name, std::string description,
          std::string condition, int rewardGold, int rewardExp);

    const std::string &id() const { return id_; }
    const std::string &name() const { return name_; }
    const std::string &description() const { return description_; }
    const std::string &condition() const { return condition_; }
    int rewardGold() const { return rewardGold_; }
    int rewardExp() const { return rewardExp_; }

    bool isAccepted() const { return accepted_; }
    bool isCompleted() const { return completed_; }
    bool isRewarded() const { return rewarded_; }

    void accept() { accepted_ = true; }
    void complete() { completed_ = true; }
    void reward() { rewarded_ = true; }

    // Task type
    QuestType type() const { return type_; }
    void setType(QuestType type) { type_ = type; }

    // NPC association
    const std::string &npcId() const { return npcId_; }
    void setNpcId(std::string npcId) { npcId_ = std::move(npcId); }

    // Target item for collect quests
    const std::string &targetItemId() const { return targetItemId_; }
    void setTargetItemId(std::string itemId) { targetItemId_ = std::move(itemId); }

    // Progress tracking
    int currentProgress() const { return currentProgress_; }
    void setCurrentProgress(int progress) { currentProgress_ = progress; }
    int targetCount() const { return targetCount_; }
    void setTargetCount(int count) { targetCount_ = count; }

    // Check whether the quest condition is satisfied.
    // For Kill quests, killCount is the number of enemies killed.
    // For Collect quests, inventoryCount is the number of items in inventory.
    bool checkCompletion(int killCount = 0, int inventoryCount = 0) const;

private:
    std::string id_;
    std::string name_;
    std::string description_;
    std::string condition_;
    int rewardGold_ = 0;
    int rewardExp_ = 0;

    bool accepted_ = false;
    bool completed_ = false;
    bool rewarded_ = false;

    QuestType type_ = QuestType::Kill;
    std::string npcId_;
    std::string targetItemId_;
    int currentProgress_ = 0;
    int targetCount_ = 0;
};
