#pragma once

#include "Affinity.h"
#include "Element.h"
#include "Skill.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

enum class PersonaStat
{
    Strength,
    Magic,
    Endurance,
    Agility,
    Luck
};

inline std::string personaStatName(PersonaStat s)
{
    switch (s)
    {
    case PersonaStat::Strength:
        return "Strength";
    case PersonaStat::Magic:
        return "Magic";
    case PersonaStat::Endurance:
        return "Endurance";
    case PersonaStat::Agility:
        return "Agility";
    case PersonaStat::Luck:
        return "Luck";
    default:
        return "Unknown";
    }
}

// A Persona is a summonable identity that provides stats, resistances and skills.
class Persona
{
public:
    Persona() = default;
    Persona(std::string id, std::string name, std::string arcana, int level,
            int strength, int magic, int endurance, int agility, int luck);

    const std::string &id() const { return id_; }
    const std::string &name() const { return name_; }
    const std::string &arcana() const { return arcana_; }
    int level() const { return level_; }

    // Stat access
    int stat(PersonaStat s) const;
    void setStat(PersonaStat s, int value);
    int strength() const { return stat(PersonaStat::Strength); }
    int magic() const { return stat(PersonaStat::Magic); }
    int endurance() const { return stat(PersonaStat::Endurance); }
    int agility() const { return stat(PersonaStat::Agility); }
    int luck() const { return stat(PersonaStat::Luck); }

    // Affinity
    Affinity affinity(Element e) const;
    void setAffinity(Element e, Affinity a);

    // Skills
    void learnSkill(std::shared_ptr<Skill> skill);
    const std::vector<std::shared_ptr<Skill>> &skills() const { return skills_; }
    Skill *findSkill(const std::string &id) const;

    // Potential skills unlocked at specific levels.
    void addPotentialSkill(int level, std::shared_ptr<Skill> skill);
    std::vector<std::shared_ptr<Skill>> checkSkillUnlocks(int currentLevel);

    void gainExp(int amount);

private:
    void levelUp();

    std::string id_;
    std::string name_;
    std::string arcana_;
    int level_ = 1;
    int exp_ = 0;
    int expToNextLevel_ = 100;
    std::map<PersonaStat, int> stats_;
    std::map<Element, Affinity> affinities_;
    std::vector<std::shared_ptr<Skill>> skills_;
    std::vector<std::pair<int, std::shared_ptr<Skill>>> potentialSkills_;
};
