#include "Quest.h"

Quest::Quest(std::string id, std::string name, std::string description,
             std::string condition, int rewardGold, int rewardExp)
    : id_(std::move(id)), name_(std::move(name)),
      description_(std::move(description)), condition_(std::move(condition)),
      rewardGold_(rewardGold), rewardExp_(rewardExp) {}

bool Quest::checkCompletion(int killCount, int inventoryCount) const
{
    if (type_ == QuestType::Kill)
    {
        // Stub: quests whose condition starts with "kill:" require a kill count.
        if (condition_.rfind("kill:", 0) == 0)
        {
            int required = std::stoi(condition_.substr(5));
            return killCount >= required;
        }
    }
    else if (type_ == QuestType::Collect)
    {
        return inventoryCount >= targetCount_;
    }
    return false;
}
