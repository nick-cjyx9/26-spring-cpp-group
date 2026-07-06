#pragma once

#include "SocialLink.h"

#include <map>
#include <string>
#include <vector>

class SocialLinkManager
{
public:
    SocialLinkManager() = default;

    void addLink(const SocialLink &link);
    void addLink(SocialLink &&link);

    SocialLink *getLink(const std::string &id);
    const SocialLink *getLink(const std::string &id) const;

    std::vector<SocialLink *> allLinks();
    std::vector<const SocialLink *> allLinks() const;

    size_t linkCount() const { return links_.size(); }

private:
    std::map<std::string, SocialLink> links_;
};
