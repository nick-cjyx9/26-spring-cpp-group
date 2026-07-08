#include "Character.h"
#include "Item.h"

#include <algorithm>

Character::Character(std::string name, int maxHp, int maxSp,
                     int strength, int magic, int endurance, int agility, int luck)
    : name_(std::move(name)), maxHp_(maxHp), hp_(maxHp),
      maxSp_(maxSp), sp_(maxSp),
      baseStrength_(strength), baseMagic_(magic), baseEndurance_(endurance),
      baseAgility_(agility), baseLuck_(luck) {}

int Character::attack() const
{
    int personaBonus = persona_ ? persona_->strength() : 0;
    return baseStrength_ + personaBonus + equipmentAttackBonus_ + slStrengthBonus_;
}

int Character::defense() const
{
    int personaBonus = persona_ ? persona_->endurance() : 0;
    return baseEndurance_ + personaBonus + equipmentDefenseBonus_ + slEnduranceBonus_;
}

int Character::speed() const
{
    int personaBonus = persona_ ? persona_->agility() : 0;
    return baseAgility_ + personaBonus + equipmentSpeedBonus_ + slAgilityBonus_;
}

int Character::magic() const
{
    return baseMagic_ + (persona_ ? persona_->magic() : 0) + slMagicBonus_;
}

Affinity Character::affinity(Element e) const
{
    if (persona_)
        return persona_->affinity(e);
    return Affinity::Normal;
}

void Character::takeDamage(int damage)
{
    int actual = std::max(1, damage - defense());
    hp_ -= actual;
    if (hp_ < 0)
        hp_ = 0;
}

void Character::heal(int amount)
{
    hp_ += amount;
    if (hp_ > maxHp_)
        hp_ = maxHp_;
}

void Character::consumeSp(int amount)
{
    sp_ -= amount;
    if (sp_ < 0)
        sp_ = 0;
}

void Character::recoverSp(int amount)
{
    sp_ += amount;
    if (sp_ > maxSp_)
        sp_ = maxSp_;
}

bool Character::spendGold(int amount)
{
    if (amount > gold_)
        return false;
    gold_ -= amount;
    return true;
}

void Character::gainExp(int amount)
{
    exp_ += amount;
    while (exp_ >= expToNextLevel_)
    {
        exp_ -= expToNextLevel_;
        levelUp();
    }
}

void Character::levelUp()
{
    LevelUpSnapshot snap;
    snap.oldLevel = level_;
    snap.oldMaxHp = maxHp_;
    snap.oldMaxSp = maxSp_;
    snap.oldStrength = baseStrength_;
    snap.oldMagic = baseMagic_;
    snap.oldEndurance = baseEndurance_;
    snap.oldAgility = baseAgility_;
    snap.oldLuck = baseLuck_;
    snap.oldAttack = attack();
    snap.oldDefense = defense();
    snap.oldSpeed = speed();

    int prevLevel = level_;
    ++level_;

    // Non-linear growth: higher levels grant bigger bonuses.
    // HP  grows fastest, followed by SP, then combat stats.
    maxHp_ += 15 + prevLevel * 2;      // e.g. L1:+17, L5:+25, L10:+35
    hp_ = maxHp_;
    maxSp_ += 5 + prevLevel;            // e.g. L1:+6, L5:+10, L10:+15
    sp_ = maxSp_;
    baseStrength_ += 1 + (prevLevel >= 5 ? 1 : 0);   // extra at L5+
    baseEndurance_ += 1 + (prevLevel >= 7 ? 1 : 0);  // extra at L7+
    baseAgility_ += 1 + (prevLevel >= 6 ? 1 : 0);    // extra at L6+
    baseMagic_ += 1 + (prevLevel >= 8 ? 1 : 0);      // extra at L8+
    baseLuck_ += 1;
    expToNextLevel_ = static_cast<int>(expToNextLevel_ * 1.5);

    snap.newLevel = level_;
    snap.newMaxHp = maxHp_;
    snap.newMaxSp = maxSp_;
    snap.newStrength = baseStrength_;
    snap.newMagic = baseMagic_;
    snap.newEndurance = baseEndurance_;
    snap.newAgility = baseAgility_;
    snap.newLuck = baseLuck_;
    snap.newAttack = attack();
    snap.newDefense = defense();
    snap.newSpeed = speed();

    // Check for persona skill unlocks
    if (persona_)
    {
        auto unlocked = persona_->checkSkillUnlocks(level_);
        for (const auto &s : unlocked)
        {
            if (s)
                snap.unlockedSkills.push_back(s->name());
        }
    }

    snapshot_ = snap;
    hasSnapshot_ = true;
}

void Character::setPersona(std::shared_ptr<Persona> persona)
{
    persona_ = std::move(persona);
}

void Character::setBaseStats(int strength, int magic, int endurance, int agility, int luck)
{
    baseStrength_ = strength;
    baseMagic_ = magic;
    baseEndurance_ = endurance;
    baseAgility_ = agility;
    baseLuck_ = luck;
}

void Character::setEquipmentBonuses(int attack, int defense, int speed, int hp, int magic)
{
    equipmentAttackBonus_ = attack;
    equipmentDefenseBonus_ = defense;
    equipmentSpeedBonus_ = speed;
    equipmentHpBonus_ = hp;
    equipmentMagicBonus_ = magic;
}

void Character::equip(std::shared_ptr<EquipmentItem> equipment)
{
    if (!equipment)
        return;
    equipmentAttackBonus_ += equipment->attackBonus();
    equipmentDefenseBonus_ += equipment->defenseBonus();
    equipmentSpeedBonus_ += equipment->speedBonus();
    equipmentHpBonus_ += equipment->hpBonus();
    equipmentMagicBonus_ += equipment->magicBonus();
}

void Character::applySocialLinkBonus(PersonaStat s, int value)
{
    switch (s)
    {
    case PersonaStat::Strength:
        slStrengthBonus_ += value;
        break;
    case PersonaStat::Magic:
        slMagicBonus_ += value;
        break;
    case PersonaStat::Endurance:
        slEnduranceBonus_ += value;
        break;
    case PersonaStat::Agility:
        slAgilityBonus_ += value;
        break;
    case PersonaStat::Luck:
        slLuckBonus_ += value;
        break;
    }
}

void Character::clearSocialLinkBonuses()
{
    slStrengthBonus_ = 0;
    slMagicBonus_ = 0;
    slEnduranceBonus_ = 0;
    slAgilityBonus_ = 0;
    slLuckBonus_ = 0;
}
