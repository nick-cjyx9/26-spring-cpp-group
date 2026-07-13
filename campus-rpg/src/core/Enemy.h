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
    Enemy(std::string id, std::string name, int hp,
          int strength, int magic, int speed,
          int rewardExp, int rewardGold);
    virtual ~Enemy() = default;

    virtual std::string battleCry() const = 0;
    virtual std::unique_ptr<Enemy> clone() const = 0;

    const std::string &id() const { return id_; }
    const std::string &name() const { return name_; }
    const std::string &textureId() const { return textureId_; }
    void setTextureId(const std::string &id) { textureId_ = id; }
    int hp() const { return hp_; }
    int maxHp() const { return maxHp_; }
    int strength() const { return strength_; }
    int magic() const { return magic_; }
    int speed() const { return speed_; }
    int rewardExp() const { return rewardExp_; }
    int rewardGold() const { return rewardGold_; }

    void takeDamage(int damage);
    bool isAlive() const { return hp_ > 0; }

    Affinity affinity(Element e) const;
    void setAffinity(Element e, Affinity a);

    void addSkill(std::shared_ptr<Skill> skill);
    const std::vector<std::shared_ptr<Skill>> &skills() const { return skills_; }

    // Fixed attack pattern: sequence of skill IDs or "normal" for basic attack.
    void setAttackPattern(std::vector<std::string> pattern);
    const std::vector<std::string> &attackPattern() const { return attackPattern_; }
    std::shared_ptr<Skill> chooseSkill(size_t turnIndex) const;

    void addDropPersonaId(const std::string &personaId);
    const std::vector<std::string> &dropPersonaIds() const { return dropPersonaIds_; }

    // Scale stats based on player level.
    // extraMultiplier > 1 makes the enemy tougher (e.g. second-map night mobs).
    void scaleToLevel(int playerLevel, double extraMultiplier = 1.0);

protected:
    std::string id_;
    std::string name_;
    std::string textureId_;
    int hp_ = 0;
    int maxHp_ = 0;
    int strength_ = 0;
    int magic_ = 0;
    int speed_ = 0;
    int baseStrength_ = 0;
    int baseMagic_ = 0;
    int baseSpeed_ = 0;
    int rewardExp_ = 0;
    int rewardGold_ = 0;

    std::map<Element, Affinity> affinities_;
    std::vector<std::shared_ptr<Skill>> skills_;
    std::vector<std::string> attackPattern_;
    std::vector<std::string> dropPersonaIds_;
};

class Slime : public Enemy
{
public:
    Slime();
    std::string battleCry() const override { return "Bloop bloop!"; }
    std::unique_ptr<Enemy> clone() const override { return std::make_unique<Slime>(*this); }
};

class Goblin : public Enemy
{
public:
    Goblin();
    std::string battleCry() const override { return "Grrr, shiny things!"; }
    std::unique_ptr<Enemy> clone() const override { return std::make_unique<Goblin>(*this); }
};

class Boss : public Enemy
{
public:
    Boss();
    std::string battleCry() const override { return "You dare challenge me?"; }
    std::unique_ptr<Enemy> clone() const override { return std::make_unique<Boss>(*this); }
};

class FinalBoss : public Enemy
{
public:
    FinalBoss();
    std::string battleCry() const override { return "I am your end!"; }
    std::unique_ptr<Enemy> clone() const override { return std::make_unique<FinalBoss>(*this); }
};
