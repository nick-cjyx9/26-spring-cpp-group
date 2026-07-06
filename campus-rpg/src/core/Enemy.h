#pragma once

#include <string>

class Enemy
{
public:
    Enemy(std::string id, std::string name, int hp, int attack, int defense,
          int rewardExp, int rewardGold);
    virtual ~Enemy() = default;

    virtual std::string battleCry() const = 0;

    const std::string &id() const { return id_; }
    const std::string &name() const { return name_; }
    int hp() const { return hp_; }
    int maxHp() const { return maxHp_; }
    int attack() const { return attack_; }
    int defense() const { return defense_; }
    int rewardExp() const { return rewardExp_; }
    int rewardGold() const { return rewardGold_; }

    void takeDamage(int damage);
    bool isAlive() const { return hp_ > 0; }

protected:
    std::string id_;
    std::string name_;
    int hp_ = 0;
    int maxHp_ = 0;
    int attack_ = 0;
    int defense_ = 0;
    int rewardExp_ = 0;
    int rewardGold_ = 0;
};

class Slime : public Enemy
{
public:
    Slime();
    std::string battleCry() const override { return "Bloop bloop!"; }
};

class Goblin : public Enemy
{
public:
    Goblin();
    std::string battleCry() const override { return "Grrr, shiny things!"; }
};

class Boss : public Enemy
{
public:
    Boss();
    std::string battleCry() const override { return "You dare challenge me?"; }
};
