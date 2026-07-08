#include "Item.h"
#include "Character.h"

Item::Item(std::string id, std::string name, std::string description, int value, ItemType type, std::string textureId)
    : id_(std::move(id)), name_(std::move(name)),
      description_(std::move(description)), value_(value), type_(type), textureId_(std::move(textureId)) {}

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

FoodItem::FoodItem(std::string id, std::string name, std::string description, int value, int healAmount, std::string textureId)
    : Item(std::move(id), std::move(name), std::move(description), value, ItemType::Food, std::move(textureId)),
      healAmount_(healAmount) {}

std::unique_ptr<Item> FoodItem::clone() const
{
    auto item = std::make_unique<FoodItem>(id_, name_, description_, value_, healAmount_, textureId_);
    item->setQuantity(quantity_);
    return item;
}

void FoodItem::use(Character &character)
{
    character.heal(healAmount_);
}

PotionItem::PotionItem(std::string id, std::string name, std::string description, int value, int healAmount, std::string textureId)
    : Item(std::move(id), std::move(name), std::move(description), value, ItemType::Potion, std::move(textureId)),
      healAmount_(healAmount) {}

std::unique_ptr<Item> PotionItem::clone() const
{
    auto item = std::make_unique<PotionItem>(id_, name_, description_, value_, healAmount_, textureId_);
    item->setQuantity(quantity_);
    return item;
}

void PotionItem::use(Character &character)
{
    character.heal(healAmount_);
}

SpItem::SpItem(std::string id, std::string name, std::string description, int value, int spAmount, std::string textureId)
    : Item(std::move(id), std::move(name), std::move(description), value, ItemType::SpRecovery, std::move(textureId)),
      spAmount_(spAmount) {}

std::unique_ptr<Item> SpItem::clone() const
{
    auto item = std::make_unique<SpItem>(id_, name_, description_, value_, spAmount_, textureId_);
    item->setQuantity(quantity_);
    return item;
}

void SpItem::use(Character &character)
{
    character.recoverSp(spAmount_);
}

EquipmentItem::EquipmentItem(std::string id, std::string name, std::string description, int value,
                             int strengthBonus, int magicBonus, int speedBonus,
                             EquipmentSlot slot, std::string textureId)
    : Item(std::move(id), std::move(name), std::move(description), value, ItemType::Equipment, std::move(textureId)),
      strengthBonus_(strengthBonus), magicBonus_(magicBonus), speedBonus_(speedBonus),
      slot_(slot) {}

std::unique_ptr<Item> EquipmentItem::clone() const
{
    auto item = std::make_unique<EquipmentItem>(id_, name_, description_, value_,
                                               strengthBonus_, magicBonus_, speedBonus_,
                                               slot_, textureId_);
    item->setQuantity(quantity_);
    return item;
}

void EquipmentItem::use(Character &character)
{
    character.equip(std::make_shared<EquipmentItem>(id_, name_, description_, value_,
                                                     strengthBonus_, magicBonus_, speedBonus_,
                                                     slot_, textureId_));
}

PersonaItem::PersonaItem(std::string id, std::string name, std::string description, int value,
                          std::string personaId, std::string textureId)
    : Item(std::move(id), std::move(name), std::move(description), value, ItemType::Persona, std::move(textureId)),
      personaId_(std::move(personaId)) {}

std::unique_ptr<Item> PersonaItem::clone() const
{
    auto item = std::make_unique<PersonaItem>(id_, name_, description_, value_, personaId_, textureId_);
    item->setQuantity(quantity_);
    return item;
}

void PersonaItem::use(Character & /*character*/)
{
    // Persona acquisition is handled by GameManager / inventory logic.
}
