#include "Item.h"
#include "Character.h"

Item::Item(std::string id, std::string name, std::string description, int value, ItemType type)
    : id_(std::move(id)), name_(std::move(name)),
      description_(std::move(description)), value_(value), type_(type) {}

std::string Item::typeString() const
{
    switch (type_)
    {
    case ItemType::Food:
        return "Food";
    case ItemType::Potion:
        return "Potion";
    case ItemType::SpRecovery:
        return "SP";
    case ItemType::Equipment:
        return "Equipment";
    case ItemType::Persona:
        return "Persona";
    }
    return "Unknown";
}

FoodItem::FoodItem(std::string id, std::string name, std::string description, int value, int healAmount)
    : Item(std::move(id), std::move(name), std::move(description), value, ItemType::Food),
      healAmount_(healAmount) {}

std::unique_ptr<Item> FoodItem::clone() const
{
    return std::make_unique<FoodItem>(id_, name_, description_, value_, healAmount_);
}

void FoodItem::use(Character &character)
{
    character.heal(healAmount_);
}

PotionItem::PotionItem(std::string id, std::string name, std::string description, int value, int healAmount)
    : Item(std::move(id), std::move(name), std::move(description), value, ItemType::Potion),
      healAmount_(healAmount) {}

std::unique_ptr<Item> PotionItem::clone() const
{
    return std::make_unique<PotionItem>(id_, name_, description_, value_, healAmount_);
}

void PotionItem::use(Character &character)
{
    character.heal(healAmount_);
}

SpItem::SpItem(std::string id, std::string name, std::string description, int value, int spAmount)
    : Item(std::move(id), std::move(name), std::move(description), value, ItemType::SpRecovery),
      spAmount_(spAmount) {}

std::unique_ptr<Item> SpItem::clone() const
{
    return std::make_unique<SpItem>(id_, name_, description_, value_, spAmount_);
}

void SpItem::use(Character &character)
{
    character.recoverSp(spAmount_);
}

EquipmentItem::EquipmentItem(std::string id, std::string name, std::string description, int value,
                             int attackBonus, int defenseBonus, int speedBonus,
                             int hpBonus, int magicBonus, EquipmentSlot slot,
                             std::string textureId)
    : Item(std::move(id), std::move(name), std::move(description), value, ItemType::Equipment),
      attackBonus_(attackBonus), defenseBonus_(defenseBonus), speedBonus_(speedBonus),
      hpBonus_(hpBonus), magicBonus_(magicBonus), slot_(slot), textureId_(std::move(textureId)) {}

std::unique_ptr<Item> EquipmentItem::clone() const
{
    return std::make_unique<EquipmentItem>(id_, name_, description_, value_,
                                           attackBonus_, defenseBonus_, speedBonus_,
                                           hpBonus_, magicBonus_, slot_, textureId_);
}

void EquipmentItem::use(Character &character)
{
    character.equip(std::make_shared<EquipmentItem>(id_, name_, description_, value_,
                                                    attackBonus_, defenseBonus_, speedBonus_,
                                                    hpBonus_, magicBonus_, slot_, textureId_));
}

PersonaItem::PersonaItem(std::string id, std::string name, std::string description, int value,
                         std::string personaId)
    : Item(std::move(id), std::move(name), std::move(description), value, ItemType::Persona),
      personaId_(std::move(personaId)) {}

std::unique_ptr<Item> PersonaItem::clone() const
{
    return std::make_unique<PersonaItem>(id_, name_, description_, value_, personaId_);
}

void PersonaItem::use(Character & /*character*/)
{
    // Persona acquisition is handled by GameManager / inventory logic.
}
