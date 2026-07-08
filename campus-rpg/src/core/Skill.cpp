#include "Skill.h"
#include "Character.h"
#include "Enemy.h"

#include <algorithm>
#include <random>

namespace
{
    thread_local std::mt19937 rng{std::random_device{}()};

    int randomVariance(int base)
    {
        // Persona-style ~5% variance on either side.
        std::uniform_int_distribution<int> dist(-base / 20, base / 20);
        return base + dist(rng);
    }
} // namespace

Skill::Skill(std::string id, std::string name, Element element,
             int power, int cost, SkillCostType costType, bool healing, int requiredLevel)
    : id_(std::move(id)), name_(std::move(name)), element_(element),
      power_(power), cost_(cost), costType_(costType), healing_(healing), requiredLevel_(requiredLevel) {}

std::unique_ptr<Skill> Skill::clone() const
{
    return std::make_unique<Skill>(id_, name_, element_, power_, cost_, costType_, healing_, requiredLevel_);
}

bool Skill::canUse(const Character &caster) const
{
    if (cost_ <= 0)
        return true;
    if (costType_ == SkillCostType::SP)
        return caster.sp() >= cost_;
    return caster.hp() > cost_;
}

void Skill::applyCost(Character &caster) const
{
    if (cost_ <= 0)
        return;
    if (costType_ == SkillCostType::SP)
        caster.consumeSp(cost_);
    else
        caster.takeDamage(cost_);
}

int Skill::calculateRawDamage(const Character &caster, const Enemy &target) const
{
    if (healing_)
        return 0;

    // Damage formula: deterministic-ish scaling suitable for course-project numbers.
    // Keeps early-game skills from one-shotting basic enemies while still scaling.
    int attackStat = std::max(1, caster.magic());
    int defenseStat = std::max(1, target.defense());
    int raw = (power_ * attackStat) / (defenseStat + 10);
    return std::max(1, randomVariance(raw));
}

int Skill::use(Character &caster, Enemy &target) const
{
    if (!canUse(caster))
        return 0;

    applyCost(caster);

    if (healing_)
    {
        // Healing skills target the caster themselves in this simplified model.
        caster.heal(power_);
        return -power_;
    }

    int damage = calculateRawDamage(caster, target);
    target.takeDamage(damage);
    return damage;
}
