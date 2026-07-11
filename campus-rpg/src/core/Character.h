#pragma once

#include "Persona.h"

#include <memory>
#include <string>
#include <vector>

class EquipmentItem;

class Character
{
public:
    Character() = default;
    Character(std::string name, int maxHp, int maxSp);

    // Getters
    const std::string &name() const { return name_; }
    int level() const { return level_; }
    int hp() const { return hp_; }
    int maxHp() const { return maxHp_; }
    int sp() const { return sp_; }
    int maxSp() const { return maxSp_; }
    int exp() const { return exp_; }
    int expToNextLevel() const { return expToNextLevel_; }
    int gold() const { return gold_; }

    // Combat stats are derived from current Persona + equipment, scaled by Persona level.
    int attack() const;
    int magic() const;
    int speed() const;
    Affinity affinity(Element e) const;

    int equipmentStrengthBonus() const { return equipmentStrengthBonus_; }
    int equipmentMagicBonus() const { return equipmentMagicBonus_; }
    int equipmentSpeedBonus() const { return equipmentSpeedBonus_; }

    // Persona management
    Persona *currentPersona() const { return persona_.get(); }
    std::shared_ptr<Persona> persona() const { return persona_; }
    void setPersona(std::shared_ptr<Persona> persona);

    const std::vector<std::shared_ptr<Persona>> &ownedPersonas() const { return ownedPersonas_; }
    std::vector<std::shared_ptr<Persona>> &ownedPersonas() { return ownedPersonas_; }
    static constexpr size_t kMaxOwnedPersonas = 6;
    bool addPersona(std::shared_ptr<Persona> persona);
    bool removePersona(const std::string &id);
    void clearOwnedPersonas() { ownedPersonas_.clear(); }
    bool ownsPersona(const std::string &id) const;

    // Level-up snapshot (cleared on read). Records only character-level changes
    // (HP/SP max); Persona growth is tracked separately via Persona::checkSkillUnlocks.
    struct LevelUpSnapshot
    {
        int oldLevel = 0, newLevel = 0;
        int oldMaxHp = 0, newMaxHp = 0;
        int oldMaxSp = 0, newMaxSp = 0;
    };
    bool hasLevelUpSnapshot() const { return hasSnapshot_; }
    const LevelUpSnapshot &levelUpSnapshot() const { return snapshot_; }
    void clearLevelUpSnapshot() { hasSnapshot_ = false; snapshot_ = LevelUpSnapshot(); }

    // Setters / mutations
    void setName(const std::string &name) { name_ = name; }
    void takeDamage(int damage);
    void heal(int amount);
    void consumeSp(int amount);
    void recoverSp(int amount);
    void addGold(int amount) { gold_ += amount; }
    void setGold(int v) { gold_ = v; }
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

    void setEquipmentBonuses(int strength, int magic, int speed);
    void equip(std::shared_ptr<EquipmentItem> equipment);

    bool isAlive() const { return hp_ > 0; }

private:
    void levelUp();

    // Final stat = (personaStat + equipmentBonus) * levelMultiplier
    int computePersonaStat(PersonaStat s) const;

    std::string name_;
    int level_ = 1;
    int hp_ = 100;
    int maxHp_ = 100;
    int sp_ = 50;
    int maxSp_ = 50;
    int exp_ = 0;
    int expToNextLevel_ = 100;
    int gold_ = 0;

    int equipmentStrengthBonus_ = 0;
    int equipmentMagicBonus_ = 0;
    int equipmentSpeedBonus_ = 0;

    std::shared_ptr<Persona> persona_;
    std::vector<std::shared_ptr<Persona>> ownedPersonas_;

    LevelUpSnapshot snapshot_;
    bool hasSnapshot_ = false;
};
