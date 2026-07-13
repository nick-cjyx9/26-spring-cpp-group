#include "Enemy.h"

#include <algorithm>
#include <random>

namespace
{
    void initDefaultAffinities(std::map<Element, Affinity> &affs)
    {
        affs[Element::Physical] = Affinity::Normal;
        affs[Element::Fire] = Affinity::Normal;
        affs[Element::Ice] = Affinity::Normal;
        affs[Element::Electric] = Affinity::Normal;
        affs[Element::Wind] = Affinity::Normal;
        affs[Element::Light] = Affinity::Normal;
        affs[Element::Dark] = Affinity::Normal;
        affs[Element::Almighty] = Affinity::Normal;
    }
} // namespace

Enemy::Enemy(std::string id, std::string name, int hp,
             int strength, int magic, int speed,
             int rewardExp, int rewardGold)
    : id_(std::move(id)), name_(std::move(name)), hp_(hp), maxHp_(hp),
      strength_(strength), magic_(magic), speed_(speed),
      baseStrength_(strength), baseMagic_(magic), baseSpeed_(speed),
      rewardExp_(rewardExp), rewardGold_(rewardGold)
{
    initDefaultAffinities(affinities_);
}

void Enemy::takeDamage(int damage)
{
    int actual = std::max(1, damage);
    hp_ -= actual;
    if (hp_ < 0)
        hp_ = 0;
}

Affinity Enemy::affinity(Element e) const
{
    auto it = affinities_.find(e);
    if (it != affinities_.end())
        return it->second;
    return Affinity::Normal;
}

void Enemy::setAffinity(Element e, Affinity a)
{
    affinities_[e] = a;
}

void Enemy::addSkill(std::shared_ptr<Skill> skill)
{
    if (skill)
        skills_.push_back(std::move(skill));
}

void Enemy::setAttackPattern(std::vector<std::string> pattern)
{
    attackPattern_ = std::move(pattern);
}

std::shared_ptr<Skill> Enemy::chooseSkill(size_t turnIndex) const
{
    if (attackPattern_.empty())
        return nullptr;

    const std::string &action = attackPattern_[turnIndex % attackPattern_.size()];
    if (action == "normal")
        return nullptr;

    for (const auto &skill : skills_)
    {
        if (skill && skill->id() == action)
            return skill;
    }
    return nullptr;
}

void Enemy::addDropPersonaId(const std::string &personaId)
{
    dropPersonaIds_.push_back(personaId);
}

void Enemy::scaleToLevel(int playerLevel, double extraMultiplier)
{
    double multiplier = (1.0 + (playerLevel - 1) * 0.1) * extraMultiplier;
    strength_ = static_cast<int>(baseStrength_ * multiplier);
    magic_ = static_cast<int>(baseMagic_ * multiplier);
    speed_ = static_cast<int>(baseSpeed_ * multiplier);
    maxHp_ = static_cast<int>(maxHp_ * multiplier);
    hp_ = maxHp_;
}

Slime::Slime() : Enemy("enemy_slime", "Slime", 30, 6, 4, 3, 15, 5)
{
    setAffinity(Element::Fire, Affinity::Weak);
    setAffinity(Element::Ice, Affinity::Weak);
    addSkill(std::make_shared<Skill>("skill_slash", "Slash", Element::Physical, 8, 0, SkillCostType::HP));
    setAttackPattern({"skill_slash", "normal", "skill_slash"});
    setTextureId("monsters/bunny");
}

Goblin::Goblin() : Enemy("enemy_goblin", "Goblin", 50, 10, 5, 5, 30, 10)
{
    setAffinity(Element::Fire, Affinity::Resist);
    addSkill(std::make_shared<Skill>("skill_cleave", "Cleave", Element::Physical, 12, 0, SkillCostType::HP));
    setAttackPattern({"normal", "skill_cleave", "skill_cleave"});
    setTextureId("monsters/duck");
}

Boss::Boss() : Enemy("enemy_boss", "Campus Guardian", 200, 20, 14, 8, 150, 80)
{
    setAffinity(Element::Physical, Affinity::Resist);
    setAffinity(Element::Fire, Affinity::Null);
    addSkill(std::make_shared<Skill>("skill_mighty_swing", "Mighty Swing", Element::Physical, 25, 0, SkillCostType::HP));
    addSkill(std::make_shared<Skill>("skill_maragi", "Maragi", Element::Fire, 20, 8, SkillCostType::SP));
    setAttackPattern({"skill_mighty_swing", "normal", "skill_maragi", "normal"});
    setTextureId("monsters/treant");
}

FinalBoss::FinalBoss() : Enemy("enemy_final_boss", "Shadow Overlord", 300, 24, 18, 15, 500, 200)
{
    setAffinity(Element::Physical, Affinity::Resist);
    setAffinity(Element::Fire, Affinity::Null);
    setAffinity(Element::Ice, Affinity::Null);
    setAffinity(Element::Electric, Affinity::Resist);
    setAffinity(Element::Dark, Affinity::Drain);
    addSkill(std::make_shared<Skill>("skill_mighty_swing", "Mighty Swing", Element::Physical, 35, 0, SkillCostType::HP));
    addSkill(std::make_shared<Skill>("skill_maragi", "Maragi", Element::Fire, 30, 10, SkillCostType::SP));
    addSkill(std::make_shared<Skill>("skill_mudo", "Mudo", Element::Dark, 40, 15, SkillCostType::SP));
    setAttackPattern({"skill_mighty_swing", "skill_maragi", "skill_mudo", "normal", "skill_mighty_swing"});
    setTextureId("boss_pixel");
}
