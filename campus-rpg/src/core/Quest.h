#pragma once

#include <string>

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

    // Check whether the quest condition is satisfied. For now this is a stub.
    bool checkCompletion(int killCount = 0) const;

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
};
