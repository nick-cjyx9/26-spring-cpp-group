#pragma once

#include "Affinity.h"
#include "Element.h"
#include "Skill.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class Enemy
{
public:
    Enemy(std::string id, std::string name, int hp, int attack, int defense, int speed,
          int rewardExp, int rewardGold);
    virtual ~Enemy() = default;

    virtual std::string battleCry() const = 0;
    virtual std::unique_ptr<Enemy> clone() const = 0;

    const std::string &id() const { return id_; }
    const std::string &name() const { return name_; }
    int hp() const { return hp_; }
    int maxHp() const { return maxHp_; }
    int attack() const { return attack_; }
    int defense() const { return defense_; }
    int speed() const { return speed_; }
    int rewardExp() const { return rewardExp_; }
    int rewardGold() const { return rewardGold_; }

    void takeDamage(int damage);
    bool isAlive() const { return hp_ > 0; }

    Affinity affinity(Element e) const;
    void setAffinity(Element e, Affinity a);

    void addSkill(std::shared_ptr<Skill> skill);
    const std::vector<std::shared_ptr<Skill>> &skills() const { return skills_; }
    virtual std::shared_ptr<Skill> chooseSkill() const;

    void addDropItemId(const std::string &itemId);
    const std::vector<std::string> &dropItemIds() const { return dropItemIds_; }

protected:
    std::string id_;
    std::string name_;
    int hp_ = 0;
    int maxHp_ = 0;
    int attack_ = 0;
    int defense_ = 0;
    int speed_ = 0;
    int rewardExp_ = 0;
    int rewardGold_ = 0;

    std::map<Element, Affinity> affinities_;
    std::vector<std::shared_ptr<Skill>> skills_;
    std::vector<std::string> dropItemIds_;
};

class Slime : public Enemy
{
public:
    Slime();
    std::string battleCry() const override { return "Bloop bloop!"; }
    std::unique_ptr<Enemy> clone() const override { return std::make_unique<Slime>(); }
};

class Goblin : public Enemy
{
public:
    Goblin();
    std::string battleCry() const override { return "Grrr, shiny things!"; }
    std::unique_ptr<Enemy> clone() const override { return std::make_unique<Goblin>(); }
};

class Boss : public Enemy
{
public:
    Boss();
    std::string battleCry() const override { return "You dare challenge me?"; }
    std::unique_ptr<Enemy> clone() const override { return std::make_unique<Boss>(); }
};
