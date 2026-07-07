#pragma once

#include "SocialLink.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

class Skill;

// Manages all Social Links (one per NPC). Central entry point for the
// progression, dialogue and reward APIs consumed by GameManager / scenes.
class SocialLinkManager
{
public:
    // Fired synchronously whenever a link's rank increases.
    // Arguments: (linkId, newRank). May be empty.
    // The audio layer registers a callback here to play the rank-up SFX.
    using RankUpCallback = std::function<void(const std::string &linkId, int newRank)>;

    SocialLinkManager() = default;

    // Register a listener fired on every rank-up. Replace with {} to detach.
    void setRankUpCallback(RankUpCallback cb) { rankUpCb_ = std::move(cb); }

    // ---- Link CRUD ----
    void addLink(const SocialLink &link);
    void addLink(SocialLink &&link);

    SocialLink *getLink(const std::string &id);
    const SocialLink *getLink(const std::string &id) const;

    std::vector<SocialLink *> allLinks();
    std::vector<const SocialLink *> allLinks() const;

    size_t linkCount() const { return links_.size(); }

    // ---- Progression API ----
    // Adds points to the named link. Returns the number of ranks gained
    // during this call (0 if the link does not exist or did not rank up).
    // Any rank-up is recorded as a pending event; consume it via
    // consumePendingRankUps() so the UI can show a rank-up notice.
    int addPoints(const std::string &id, int points);

    // Whether at least one link ranked up since the last consume.
    bool hasPendingRankUps() const { return !pendingRankUps_.empty(); }
    // Returns ids of links that ranked up since the last call, then clears.
    std::vector<std::string> consumePendingRankUps();

    // ---- Dialogue API ----
    // Returns the dialogue text for the link's current rank, or "" if not found.
    std::string dialogueFor(const std::string &id) const;

    // ---- Reward aggregation API ----
    // Sums the stat bonus across every link whose arcana matches `arcana`,
    // where each link contributes its current-rank reward's statBonus
    // (only if that reward targets the requested stat).
    // `statName` is one of "Strength"/"Magic"/"Endurance"/"Agility"/"Luck".
    int arcanaStatBonus(const std::string &arcana, const std::string &statName) const;

    // Collects all passive skills granted by every link at its current rank.
    std::vector<std::shared_ptr<Skill>> collectPassiveSkills() const;

private:
    std::map<std::string, SocialLink> links_;
    std::vector<std::string> pendingRankUps_;
    RankUpCallback rankUpCb_;
};
