#include "QuestManager.h"

void QuestManager::addQuest(const Quest &quest)
{
    quests_.emplace(quest.id(), quest);
}

void QuestManager::addQuest(Quest &&quest)
{
    quests_.emplace(quest.id(), std::move(quest));
}

Quest *QuestManager::getQuest(const std::string &id)
{
    auto it = quests_.find(id);
    if (it != quests_.end())
        return &it->second;
    return nullptr;
}

const Quest *QuestManager::getQuest(const std::string &id) const
{
    auto it = quests_.find(id);
    if (it != quests_.end())
        return &it->second;
    return nullptr;
}

std::vector<Quest *> QuestManager::availableQuests() const
{
    std::vector<Quest *> result;
    for (const auto &[id, quest] : quests_)
    {
        if (!quest.isAccepted())
            result.push_back(const_cast<Quest *>(&quest));
    }
    return result;
}

std::vector<Quest *> QuestManager::acceptedQuests() const
{
    std::vector<Quest *> result;
    for (const auto &[id, quest] : quests_)
    {
        if (quest.isAccepted() && !quest.isCompleted())
            result.push_back(const_cast<Quest *>(&quest));
    }
    return result;
}

std::vector<Quest *> QuestManager::completedQuests() const
{
    std::vector<Quest *> result;
    for (const auto &[id, quest] : quests_)
    {
        if (quest.isCompleted() && !quest.isRewarded())
            result.push_back(const_cast<Quest *>(&quest));
    }
    return result;
}

std::vector<Quest *> QuestManager::questsForNpc(const std::string &npcId) const
{
    std::vector<Quest *> result;
    for (const auto &[id, quest] : quests_)
    {
        if (quest.npcId() == npcId)
            result.push_back(const_cast<Quest *>(&quest));
    }
    return result;
}

bool QuestManager::acceptQuest(const std::string &id)
{
    Quest *q = getQuest(id);
    if (!q || q->isAccepted())
        return false;
    q->accept();
    return true;
}

bool QuestManager::completeQuest(const std::string &id, int killCount)
{
    Quest *q = getQuest(id);
    if (!q || !q->isAccepted() || q->isCompleted())
        return false;
    if (q->checkCompletion(killCount))
    {
        q->complete();
        return true;
    }
    return false;
}

bool QuestManager::rewardQuest(const std::string &id)
{
    Quest *q = getQuest(id);
    if (!q || !q->isCompleted() || q->isRewarded())
        return false;
    q->reward();
    return true;
}

void QuestManager::addKillProgress(int defeatedCount)
{
    if (defeatedCount <= 0)
        return;
    for (auto &[id, quest] : quests_)
    {
        if (quest.type() != QuestType::Kill || !quest.isAccepted() || quest.isCompleted())
            continue;
        int next = quest.currentProgress() + defeatedCount;
        if (quest.targetCount() > 0 && next > quest.targetCount())
            next = quest.targetCount();
        quest.setCurrentProgress(next);
        if (quest.targetCount() > 0 && quest.currentProgress() >= quest.targetCount())
            quest.complete();
    }
}
