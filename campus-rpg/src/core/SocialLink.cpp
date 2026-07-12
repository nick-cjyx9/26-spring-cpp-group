#include "SocialLink.h"

#include <algorithm>

SocialLink::SocialLink(std::string id, std::string name, std::string arcana)
    : id_(std::move(id)), name_(std::move(name)), arcana_(std::move(arcana)) {}

int SocialLink::pointsToNextRank() const
{
    if (isMaxRank())
        return 0;
    // Simple linear growth: each rank needs (rank+1) * 20 points.
    return (rank_ + 1) * 20;
}

bool SocialLink::canRankUp() const
{
    return !isMaxRank() && points_ >= pointsToNextRank();
}

void SocialLink::rankUp()
{
    if (isMaxRank())
        return;
    points_ -= pointsToNextRank();
    if (points_ < 0)
        points_ = 0;
    ++rank_;
}

int SocialLink::addPoints(int points)
{
    if (points <= 0)
        return 0;
    points_ += points;
    int gained = 0;
    while (canRankUp())
    {
        rankUp();
        ++gained;
    }
    return gained;
}

void SocialLink::setRankData(int rank, SocialLinkRankData data)
{
    if (rank < 0 || rank > kMaxRank)
        return;
    if (static_cast<int>(rankData_.size()) <= rank)
        rankData_.resize(rank + 1);
    rankData_[rank] = std::move(data);
}

SocialLinkRankData *SocialLink::rankData(int rank)
{
    if (rank < 0 || rank >= static_cast<int>(rankData_.size()))
        return nullptr;
    return &rankData_[rank];
}

const SocialLinkRankData *SocialLink::rankData(int rank) const
{
    if (rank < 0 || rank >= static_cast<int>(rankData_.size()))
        return nullptr;
    return &rankData_[rank];
}

const SocialLinkReward *SocialLink::currentReward() const
{
    const auto *data = currentRankData();
    return data ? &data->reward : nullptr;
}

std::string SocialLink::currentDialogue() const
{
    const auto *data = currentRankData();
    return data ? data->dialogue : std::string();
}
