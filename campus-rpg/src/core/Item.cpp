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
    case ItemType::Equipment:
        return "Equipment";
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

EquipmentItem::EquipmentItem(std::string id, std::string name, std::string description, int value,
                             int attackBonus, int defenseBonus)
    : Item(std::move(id), std::move(name), std::move(description), value, ItemType::Equipment),
      attackBonus_(attackBonus), defenseBonus_(defenseBonus) {}

std::unique_ptr<Item> EquipmentItem::clone() const
{
    return std::make_unique<EquipmentItem>(id_, name_, description_, value_, attackBonus_, defenseBonus_);
}

void EquipmentItem::use(Character &character)
{
    character.equip(std::make_shared<EquipmentItem>(id_, name_, description_, value_, attackBonus_, defenseBonus_));
}
