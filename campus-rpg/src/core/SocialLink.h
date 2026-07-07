#pragma once

#include "Persona.h" // for PersonaStat

#include <memory>
#include <string>
#include <vector>

class Skill;

// Reward granted when a Social Link reaches a specific rank (>= 1).
// All fields are optional; an empty reward just means "dialogue only".
struct SocialLinkReward
{
    // Persona stat bonus applied while this link is at >= this rank.
    // Use PersonaStat::Strength ... Luck, or leave unset for no stat bonus.
    bool hasStatBonus = false;
    PersonaStat stat = PersonaStat::Strength;
    int statBonus = 0;

    // Passive skill granted to the character (one-time unlock at this rank).
    std::shared_ptr<Skill> passiveSkill;

    bool hasReward() const { return hasStatBonus || passiveSkill != nullptr; }
};

// Per-rank data: dialogue shown at this rank + reward unlocked upon reaching it.
struct SocialLinkRankData
{
    std::string dialogue;
    SocialLinkReward reward;
};

// A Social Link (commu) tracks the player's bond with one NPC.
// Rank goes 0..10. Each rank has its own dialogue text and optional reward.
class SocialLink
{
public:
    static constexpr int kMaxRank = 10;

    SocialLink() = default;
    SocialLink(std::string id, std::string name, std::string arcana);

    // Identity
    const std::string &id() const { return id_; }
    const std::string &name() const { return name_; }
    const std::string &arcana() const { return arcana_; }

    // Progression
    int rank() const { return rank_; }
    int points() const { return points_; }
    int pointsToNextRank() const;
    bool canRankUp() const;
    bool isMaxRank() const { return rank_ >= kMaxRank; }
    void rankUp();

    // Adds points, auto-ranking-up while the threshold is met.
    // Returns the number of ranks gained during this call (0 if none / maxed).
    int addPoints(int points);

    // Per-rank data (rank 0..kMaxRank). Index out of range returns nullptr.
    void setRankData(int rank, SocialLinkRankData data);
    SocialLinkRankData *rankData(int rank);
    const SocialLinkRankData *rankData(int rank) const;
    const SocialLinkRankData *currentRankData() const { return rankData(rank_); }
    const SocialLinkReward *currentReward() const;
    std::string currentDialogue() const;

private:
    std::string id_;
    std::string name_;
    std::string arcana_;
    int rank_ = 0;
    int points_ = 0;
    // Indexed by rank 0..kMaxRank. Grows lazily via setRankData.
    std::vector<SocialLinkRankData> rankData_;
};
