#include "Character.h"
#include "Item.h"

#include <algorithm>

Character::Character(std::string name, int maxHp, int maxSp)
    : name_(std::move(name)), maxHp_(maxHp), hp_(maxHp),
      maxSp_(maxSp), sp_(maxSp) {}

int Character::computePersonaStat(PersonaStat s) const
{
    if (!persona_)
        return 0;

    int base = persona_->stat(s);
    int equipment = 0;
    switch (s)
    {
    case PersonaStat::Strength:
        equipment = equipmentStrengthBonus_;
        break;
    case PersonaStat::Magic:
        equipment = equipmentMagicBonus_;
        break;
    case PersonaStat::Speed:
        equipment = equipmentSpeedBonus_;
        break;
    }

    double levelMultiplier = 1.0 + (persona_->level() - 1) * 0.05;
    return static_cast<int>((base + equipment) * levelMultiplier);
}

int Character::attack() const
{
    return computePersonaStat(PersonaStat::Strength);
}

int Character::magic() const
{
    return computePersonaStat(PersonaStat::Magic);
}

int Character::speed() const
{
    return computePersonaStat(PersonaStat::Speed);
}

Affinity Character::affinity(Element e) const
{
    if (persona_)
        return persona_->affinity(e);
    return Affinity::Normal;
}

void Character::takeDamage(int damage)
{
    // No defense stat; HP is the only buffer.
    int actual = std::max(1, damage);
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

    int prevLevel = level_;
    ++level_;

    // Non-linear growth: higher levels grant bigger bonuses.
    maxHp_ += 15 + prevLevel * 2;
    hp_ = maxHp_;
    maxSp_ += 5 + prevLevel;
    sp_ = maxSp_;
    expToNextLevel_ = static_cast<int>(expToNextLevel_ * 1.5);

    snap.newLevel = level_;
    snap.newMaxHp = maxHp_;
    snap.newMaxSp = maxSp_;
    snapshot_ = snap;
    hasSnapshot_ = true;
}

void Character::setPersona(std::shared_ptr<Persona> persona)
{
    persona_ = std::move(persona);
}

bool Character::addPersona(std::shared_ptr<Persona> persona)
{
    if (!persona || ownsPersona(persona->id()))
        return false;
    if (ownedPersonas_.size() >= kMaxOwnedPersonas)
        return false;
    ownedPersonas_.push_back(std::move(persona));
    return true;
}

bool Character::removePersona(const std::string &id)
{
    for (auto it = ownedPersonas_.begin(); it != ownedPersonas_.end(); ++it)
    {
        if (*it && (*it)->id() == id)
        {
            ownedPersonas_.erase(it);
            if (persona_ && persona_->id() == id)
                persona_ = ownedPersonas_.empty() ? nullptr : ownedPersonas_.front();
            return true;
        }
    }
    return false;
}

bool Character::ownsPersona(const std::string &id) const
{
    for (const auto &p : ownedPersonas_)
    {
        if (p && p->id() == id)
            return true;
    }
    return false;
}

void Character::setEquipmentBonuses(int strength, int magic, int speed)
{
    equipmentStrengthBonus_ = strength;
    equipmentMagicBonus_ = magic;
    equipmentSpeedBonus_ = speed;
}

void Character::equip(std::shared_ptr<EquipmentItem> equipment)
{
    if (!equipment)
        return;
    equipmentStrengthBonus_ += equipment->strengthBonus();
    equipmentMagicBonus_ += equipment->magicBonus();
    equipmentSpeedBonus_ += equipment->speedBonus();
}
