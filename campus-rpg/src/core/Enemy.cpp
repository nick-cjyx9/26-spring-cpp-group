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

Enemy::Enemy(std::string id, std::string name, int hp, int attack, int defense, int speed,
             int rewardExp, int rewardGold)
    : id_(std::move(id)), name_(std::move(name)), hp_(hp), maxHp_(hp),
      attack_(attack), defense_(defense), speed_(speed),
      rewardExp_(rewardExp), rewardGold_(rewardGold)
{
    initDefaultAffinities(affinities_);
}

void Enemy::takeDamage(int damage)
{
    int actual = std::max(1, damage - defense_);
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

std::shared_ptr<Skill> Enemy::chooseSkill() const
{
    if (skills_.empty())
        return nullptr;
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<size_t> dist(0, skills_.size() - 1);
    return skills_[dist(rng)];
}

void Enemy::addDropItemId(const std::string &itemId)
{
    dropItemIds_.push_back(itemId);
}

Slime::Slime() : Enemy("enemy_slime", "Slime", 30, 8, 2, 3, 15, 5)
{
    setAffinity(Element::Fire, Affinity::Weak);
    setAffinity(Element::Ice, Affinity::Weak);
    addSkill(std::make_shared<Skill>("skill_slash", "Slash", Element::Physical, 8, 0, SkillCostType::HP));
}

Goblin::Goblin() : Enemy("enemy_goblin", "Goblin", 50, 12, 4, 5, 30, 10)
{
    setAffinity(Element::Fire, Affinity::Resist);
    addSkill(std::make_shared<Skill>("skill_cleave", "Cleave", Element::Physical, 12, 0, SkillCostType::HP));
}

Boss::Boss() : Enemy("enemy_boss", "Campus Guardian", 120, 20, 8, 6, 100, 50)
{
    setAffinity(Element::Physical, Affinity::Resist);
    setAffinity(Element::Fire, Affinity::Null);
    addSkill(std::make_shared<Skill>("skill_mighty_swing", "Mighty Swing", Element::Physical, 25, 0, SkillCostType::HP));
}
