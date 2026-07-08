#include "Persona.h"

Persona::Persona(std::string id, std::string name, std::string arcana, int level,
                 int strength, int magic, int speed)
    : id_(std::move(id)), name_(std::move(name)), arcana_(std::move(arcana)), level_(level)
{
    stats_[PersonaStat::Strength] = strength;
    stats_[PersonaStat::Magic] = magic;
    stats_[PersonaStat::Speed] = speed;

    // Default all elements to Normal.
    affinities_[Element::Physical] = Affinity::Normal;
    affinities_[Element::Fire] = Affinity::Normal;
    affinities_[Element::Ice] = Affinity::Normal;
    affinities_[Element::Electric] = Affinity::Normal;
    affinities_[Element::Wind] = Affinity::Normal;
    affinities_[Element::Light] = Affinity::Normal;
    affinities_[Element::Dark] = Affinity::Normal;
    affinities_[Element::Almighty] = Affinity::Normal;
}

int Persona::stat(PersonaStat s) const
{
    auto it = stats_.find(s);
    if (it != stats_.end())
        return it->second;
    return 0;
}

void Persona::setStat(PersonaStat s, int value)
{
    stats_[s] = value;
}

Affinity Persona::affinity(Element e) const
{
    auto it = affinities_.find(e);
    if (it != affinities_.end())
        return it->second;
    return Affinity::Normal;
}

void Persona::setAffinity(Element e, Affinity a)
{
    affinities_[e] = a;
}

void Persona::learnSkill(std::shared_ptr<Skill> skill)
{
    if (skill)
        skills_.push_back(std::move(skill));
}

Skill *Persona::findSkill(const std::string &id) const
{
    for (const auto &skill : skills_)
    {
        if (skill && skill->id() == id)
            return skill.get();
    }
    return nullptr;
}

void Persona::gainExp(int amount)
{
    exp_ += amount;
    while (exp_ >= expToNextLevel_)
    {
        exp_ -= expToNextLevel_;
        levelUp();
    }
}

void Persona::levelUp()
{
    ++level_;
    expToNextLevel_ = static_cast<int>(expToNextLevel_ * 1.5);
    growBaseStats(1.05);

    // Unlock any skills that meet the level requirement.
    checkSkillUnlocks(level_);
}

void Persona::addPotentialSkill(int level, std::shared_ptr<Skill> skill)
{
    if (!skill)
        return;
    potentialSkills_.emplace_back(level, std::move(skill));
}

std::vector<std::shared_ptr<Skill>> Persona::checkSkillUnlocks(int currentLevel)
{
    std::vector<std::shared_ptr<Skill>> newlyUnlocked;
    for (auto it = potentialSkills_.begin(); it != potentialSkills_.end();)
    {
        if (it->first <= currentLevel)
        {
            newlyUnlocked.push_back(it->second);
            learnSkill(std::move(it->second));
            it = potentialSkills_.erase(it);
        }
        else
        {
            ++it;
        }
    }
    return newlyUnlocked;
}

void Persona::growBaseStats(double multiplier)
{
    for (auto &kv : stats_)
    {
        kv.second = static_cast<int>(kv.second * multiplier);
        if (kv.second < 1)
            kv.second = 1;
    }
}
