#pragma once

#include "Element.h"

#include <memory>
#include <string>

class Character;
class Enemy;

enum class SkillCostType
{
    HP,
    SP
};

// A battle skill with an element, power and HP/SP cost.
// This base class is intentionally lightweight; support/healing skills can be
// added later as subclasses.
class Skill
{
public:
    Skill(std::string id, std::string name, Element element,
          int power, int cost, SkillCostType costType, bool healing = false);
    virtual ~Skill() = default;

    virtual std::unique_ptr<Skill> clone() const;

    const std::string &id() const { return id_; }
    const std::string &name() const { return name_; }
    Element element() const { return element_; }
    int power() const { return power_; }
    int cost() const { return cost_; }
    SkillCostType costType() const { return costType_; }
    bool isHealing() const { return healing_; }

    // Whether the caster can currently pay the skill's cost.
    bool canUse(const Character &caster) const;

    // Consumes the skill cost from the caster.
    void applyCost(Character &caster) const;

    // Calculates raw damage before target affinity is applied.
    int calculateRawDamage(const Character &caster, const Enemy &target) const;

    // Apply the skill: returns the actual HP change dealt to the target.
    // Positive = damage dealt; negative = healing received; 0 = no effect.
    // For damaging skills the result is signed so callers can log it uniformly.
    virtual int use(Character &caster, Enemy &target) const;

protected:
    std::string id_;
    std::string name_;
    Element element_;
    int power_ = 0;
    int cost_ = 0;
    SkillCostType costType_ = SkillCostType::SP;
    bool healing_ = false;
};
