#pragma once

#include <memory>
#include <string>

class EquipmentItem;

class Character
{
public:
    Character() = default;
    Character(std::string name, int maxHp, int attack, int defense);

    // Getters
    const std::string &name() const { return name_; }
    int level() const { return level_; }
    int hp() const { return hp_; }
    int maxHp() const { return maxHp_; }
    int exp() const { return exp_; }
    int expToNextLevel() const { return expToNextLevel_; }
    int gold() const { return gold_; }
    int attack() const { return baseAttack_ + equipmentAttackBonus_; }
    int defense() const { return baseDefense_ + equipmentDefenseBonus_; }
    int baseAttack() const { return baseAttack_; }
    int baseDefense() const { return baseDefense_; }
    int equipmentAttackBonus() const { return equipmentAttackBonus_; }
    int equipmentDefenseBonus() const { return equipmentDefenseBonus_; }

    // Setters / mutations
    void setName(const std::string &name) { name_ = name; }
    void takeDamage(int damage);
    void heal(int amount);
    void addGold(int amount) { gold_ += amount; }
    bool spendGold(int amount);
    void gainExp(int amount);

    // Equipment support
    void equip(std::shared_ptr<EquipmentItem> equipment);

    bool isAlive() const { return hp_ > 0; }

private:
    void levelUp();

    std::string name_;
    int level_ = 1;
    int hp_ = 100;
    int maxHp_ = 100;
    int exp_ = 0;
    int expToNextLevel_ = 100;
    int gold_ = 0;

    int baseAttack_ = 10;
    int baseDefense_ = 5;
    int equipmentAttackBonus_ = 0;
    int equipmentDefenseBonus_ = 0;
};
