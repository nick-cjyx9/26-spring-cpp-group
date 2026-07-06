#pragma once

#include <memory>
#include <string>

class Character;

enum class ItemType
{
    Food,
    Potion,
    Equipment
};

class Item
{
public:
    Item(std::string id, std::string name, std::string description, int value, ItemType type);
    virtual ~Item() = default;

    virtual std::unique_ptr<Item> clone() const = 0;
    virtual void use(Character &character) = 0;

    const std::string &id() const { return id_; }
    const std::string &name() const { return name_; }
    const std::string &description() const { return description_; }
    int value() const { return value_; }
    ItemType type() const { return type_; }

    std::string typeString() const;

protected:
    std::string id_;
    std::string name_;
    std::string description_;
    int value_ = 0;
    ItemType type_;
};

class FoodItem : public Item
{
public:
    FoodItem(std::string id, std::string name, std::string description, int value, int healAmount);

    std::unique_ptr<Item> clone() const override;
    void use(Character &character) override;

    int healAmount() const { return healAmount_; }

private:
    int healAmount_ = 0;
};

class PotionItem : public Item
{
public:
    PotionItem(std::string id, std::string name, std::string description, int value, int healAmount);

    std::unique_ptr<Item> clone() const override;
    void use(Character &character) override;

    int healAmount() const { return healAmount_; }

private:
    int healAmount_ = 0;
};

class EquipmentItem : public Item
{
public:
    EquipmentItem(std::string id, std::string name, std::string description, int value,
                  int attackBonus, int defenseBonus);

    std::unique_ptr<Item> clone() const override;
    void use(Character &character) override;

    int attackBonus() const { return attackBonus_; }
    int defenseBonus() const { return defenseBonus_; }

private:
    int attackBonus_ = 0;
    int defenseBonus_ = 0;
};
