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
    return baseStrength_ + personaBonus + equipmentAttackBonus_;
}

int Character::defense() const
{
    int personaBonus = persona_ ? persona_->endurance() : 0;
    return baseEndurance_ + personaBonus + equipmentDefenseBonus_;
}

int Character::speed() const
{
    int personaBonus = persona_ ? persona_->agility() : 0;
    return baseAgility_ + personaBonus + equipmentSpeedBonus_;
}

int Character::magic() const
{
    return baseMagic_ + (persona_ ? persona_->magic() : 0);
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
    ++level_;
    maxHp_ += 20;
    hp_ = maxHp_;
    maxSp_ += 10;
    sp_ = maxSp_;
    baseStrength_ += 1;
    baseMagic_ += 1;
    baseEndurance_ += 1;
    baseAgility_ += 1;
    baseLuck_ += 1;
    expToNextLevel_ = static_cast<int>(expToNextLevel_ * 1.5);
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
