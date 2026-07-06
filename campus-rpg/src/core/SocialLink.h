#pragma once

#include <string>

class SocialLink
{
public:
    SocialLink() = default;
    SocialLink(std::string id, std::string name, std::string arcana);

    const std::string &id() const { return id_; }
    const std::string &name() const { return name_; }
    const std::string &arcana() const { return arcana_; }

    int rank() const { return rank_; }
    int points() const { return points_; }
    int pointsToNextRank() const;

    void addPoints(int points);
    bool canRankUp() const;
    void rankUp();

private:
    std::string id_;
    std::string name_;
    std::string arcana_;
    int rank_ = 0;
    int points_ = 0;
};
