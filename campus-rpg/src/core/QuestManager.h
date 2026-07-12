#pragma once

#include "Quest.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class QuestManager
{
public:
    QuestManager() = default;

    void addQuest(const Quest &quest);
    void addQuest(Quest &&quest);

    Quest *getQuest(const std::string &id);
    const Quest *getQuest(const std::string &id) const;

    std::vector<Quest *> availableQuests() const;
    std::vector<Quest *> acceptedQuests() const;
    std::vector<Quest *> completedQuests() const;

    bool acceptQuest(const std::string &id);
    bool completeQuest(const std::string &id, int killCount = 0);
    bool rewardQuest(const std::string &id);

    // Find quests associated with a specific NPC.
    std::vector<Quest *> questsForNpc(const std::string &npcId) const;

    size_t questCount() const { return quests_.size(); }

private:
    std::map<std::string, Quest> quests_;
};
