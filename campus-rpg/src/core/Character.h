#pragma once

#include "Persona.h"

#include <memory>
#include <string>

class EquipmentItem;

class Character
{
public:
    Character() = default;
    Character(std::string name, int maxHp, int maxSp,
              int strength, int magic, int endurance, int agility, int luck);

    // Getters
    const std::string &name() const { return name_; }
    int level() const { return level_; }
    int hp() const { return hp_; }
    int maxHp() const { return maxHp_; }
    int totalMaxHp() const { return maxHp_ + equipmentHpBonus_; }
    int sp() const { return sp_; }
    int maxSp() const { return maxSp_; }
    int exp() const { return exp_; }
    int expToNextLevel() const { return expToNextLevel_; }
    int gold() const { return gold_; }

    int attack() const;
    int defense() const;
    int speed() const;
    int magic() const;
    int totalMagic() const { return magic() + equipmentMagicBonus_; }

    int baseStrength() const { return baseStrength_; }
    int baseMagic() const { return baseMagic_; }
    int baseEndurance() const { return baseEndurance_; }
    int baseAgility() const { return baseAgility_; }
    int baseLuck() const { return baseLuck_; }

    int equipmentAttackBonus() const { return equipmentAttackBonus_; }
    int equipmentDefenseBonus() const { return equipmentDefenseBonus_; }
    int equipmentSpeedBonus() const { return equipmentSpeedBonus_; }
    int equipmentHpBonus() const { return equipmentHpBonus_; }
    int equipmentMagicBonus() const { return equipmentMagicBonus_; }

    Affinity affinity(Element e) const;
    Persona *currentPersona() const { return persona_.get(); }
    std::shared_ptr<Persona> persona() const { return persona_; }

    // Setters / mutations
    void setName(const std::string &name) { name_ = name; }
    void takeDamage(int damage);
    void heal(int amount);
    void consumeSp(int amount);
    void recoverSp(int amount);
    void addGold(int amount) { gold_ += amount; }
    bool spendGold(int amount);
    void gainExp(int amount);

    // Persistence restore setters (used by SaveRepository::loadCharacter).
    void setLevel(int v) { level_ = v; }
    void setHp(int v) { hp_ = v; }
    void setMaxHp(int v) { maxHp_ = v; }
    void setSp(int v) { sp_ = v; }
    void setMaxSp(int v) { maxSp_ = v; }
    void setExp(int v) { exp_ = v; }
    void setExpToNextLevel(int v) { expToNextLevel_ = v; }
    void setGold(int v) { gold_ = v; }
    void setBaseStats(int strength, int magic, int endurance, int agility, int luck);
    void setEquipmentBonuses(int attack, int defense, int speed, int hp = 0, int magic = 0);

    void setPersona(std::shared_ptr<Persona> persona);
    void equip(std::shared_ptr<EquipmentItem> equipment);

    bool isAlive() const { return hp_ > 0; }

private:
    void levelUp();

    std::string name_;
    int level_ = 1;
    int hp_ = 100;
    int maxHp_ = 100;
    int sp_ = 50;
    int maxSp_ = 50;
    int exp_ = 0;
    int expToNextLevel_ = 100;
    int gold_ = 0;

    int baseStrength_ = 5;
    int baseMagic_ = 5;
    int baseEndurance_ = 5;
    int baseAgility_ = 5;
    int baseLuck_ = 5;

    int equipmentAttackBonus_ = 0;
    int equipmentDefenseBonus_ = 0;
    int equipmentSpeedBonus_ = 0;
    int equipmentHpBonus_ = 0;
    int equipmentMagicBonus_ = 0;

    std::shared_ptr<Persona> persona_;
};
