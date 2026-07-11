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
    Speed
};

inline std::string personaStatName(PersonaStat s)
{
    switch (s)
    {
    case PersonaStat::Strength:
        return "Strength";
    case PersonaStat::Magic:
        return "Magic";
    case PersonaStat::Speed:
        return "Speed";
    default:
        return "Unknown";
    }
}

// A Persona is a summonable identity that provides three combat stats,
// elemental resistances and usable skills.
class Persona
{
public:
    Persona() = default;
    Persona(std::string id, std::string name, std::string arcana, int level,
            int strength, int magic, int speed);

    const std::string &id() const { return id_; }
    const std::string &name() const { return name_; }
    const std::string &arcana() const { return arcana_; }
    int level() const { return level_; }

    // Base stat access. These are the Persona's own stats before equipment
    // and level multiplier are applied externally.
    int stat(PersonaStat s) const;
    void setStat(PersonaStat s, int value);
    int strength() const { return stat(PersonaStat::Strength); }
    int magic() const { return stat(PersonaStat::Magic); }
    int speed() const { return stat(PersonaStat::Speed); }

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

    // Leveling
    void gainExp(int amount);
    int exp() const { return exp_; }
    int expToNextLevel() const { return expToNextLevel_; }
    void setExp(int v) { exp_ = v; }
    void setExpToNextLevel(int v) { expToNextLevel_ = v; }

    // Apply a fixed-ratio growth to all stats. Called on level up.
    void growBaseStats(double multiplier = 1.05);

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
