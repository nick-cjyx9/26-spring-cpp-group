#include "SocialLink.h"

SocialLink::SocialLink(std::string id, std::string name, std::string arcana)
    : id_(std::move(id)), name_(std::move(name)), arcana_(std::move(arcana)) {}

int SocialLink::pointsToNextRank() const
{
    // Simple linear growth: each rank needs rank * 20 points.
    return (rank_ + 1) * 20;
}

void SocialLink::addPoints(int points)
{
    points_ += points;
    while (canRankUp() && rank_ < 10)
    {
        rankUp();
    }
}

bool SocialLink::canRankUp() const
{
    return rank_ < 10 && points_ >= pointsToNextRank();
}

void SocialLink::rankUp()
{
    if (rank_ >= 10)
        return;
    points_ -= pointsToNextRank();
    ++rank_;
}
