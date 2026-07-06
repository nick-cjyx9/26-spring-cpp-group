#include "Character.h"
#include "Item.h"

#include <algorithm>

Character::Character(std::string name, int maxHp, int attack, int defense)
    : name_(std::move(name)), maxHp_(maxHp), hp_(maxHp),
      baseAttack_(attack), baseDefense_(defense) {}

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
    baseAttack_ += 3;
    baseDefense_ += 2;
    expToNextLevel_ = static_cast<int>(expToNextLevel_ * 1.5);
}

void Character::equip(std::shared_ptr<EquipmentItem> equipment)
{
    if (!equipment)
        return;
    equipmentAttackBonus_ += equipment->attackBonus();
    equipmentDefenseBonus_ += equipment->defenseBonus();
}
