#include "SocialLinkManager.h"

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
