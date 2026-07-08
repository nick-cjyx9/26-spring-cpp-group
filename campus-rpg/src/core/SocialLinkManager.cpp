#include "SocialLinkManager.h"

#include "Skill.h"

void SocialLinkManager::addLink(const SocialLink &link)
{
    links_.emplace(link.id(), link);
}

void SocialLinkManager::addLink(SocialLink &&link)
{
    links_.emplace(link.id(), std::move(link));
}

SocialLink *SocialLinkManager::getLink(const std::string &id)
{
    auto it = links_.find(id);
    if (it != links_.end())
        return &it->second;
    return nullptr;
}

const SocialLink *SocialLinkManager::getLink(const std::string &id) const
{
    auto it = links_.find(id);
    if (it != links_.end())
        return &it->second;
    return nullptr;
}

std::vector<SocialLink *> SocialLinkManager::allLinks()
{
    std::vector<SocialLink *> result;
    for (auto &[id, link] : links_)
        result.push_back(&link);
    return result;
}

std::vector<const SocialLink *> SocialLinkManager::allLinks() const
{
    std::vector<const SocialLink *> result;
    for (const auto &[id, link] : links_)
        result.push_back(&link);
    return result;
}

int SocialLinkManager::addPoints(const std::string &id, int points)
{
    SocialLink *link = getLink(id);
    if (!link)
        return 0;
    int rankBefore = link->rank();
    int gained = link->addPoints(points);
    if (gained > 0)
    {
        pendingRankUps_.push_back(id);
        if (rankUpCb_)
            rankUpCb_(id, link->rank());
    }
    return gained;
}

std::vector<std::string> SocialLinkManager::consumePendingRankUps()
{
    std::vector<std::string> result;
    result.swap(pendingRankUps_);
    return result;
}

std::string SocialLinkManager::dialogueFor(const std::string &id) const
{
    const SocialLink *link = getLink(id);
    if (!link)
        return {};
    return link->currentDialogue();
}

std::shared_ptr<Skill> SocialLinkManager::currentSkillReward(const std::string &id) const
{
    const SocialLink *link = getLink(id);
    if (!link)
        return nullptr;
    const SocialLinkReward *reward = link->currentReward();
    if (!reward)
        return nullptr;
    return reward->newSkill;
}
