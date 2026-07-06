#pragma once

#include <memory>
#include <string>

class Character;

enum class ItemType
{
    Food,
    Potion,
    SpRecovery,
    Equipment,
    Persona
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

class SpItem : public Item
{
public:
    SpItem(std::string id, std::string name, std::string description, int value, int spAmount);

    std::unique_ptr<Item> clone() const override;
    void use(Character &character) override;

    int spAmount() const { return spAmount_; }

private:
    int spAmount_ = 0;
};

class EquipmentItem : public Item
{
public:
    EquipmentItem(std::string id, std::string name, std::string description, int value,
                  int attackBonus, int defenseBonus, int speedBonus = 0);

    std::unique_ptr<Item> clone() const override;
    void use(Character &character) override;

    int attackBonus() const { return attackBonus_; }
    int defenseBonus() const { return defenseBonus_; }
    int speedBonus() const { return speedBonus_; }

private:
    int attackBonus_ = 0;
    int defenseBonus_ = 0;
    int speedBonus_ = 0;
};

class PersonaItem : public Item
{
public:
    PersonaItem(std::string id, std::string name, std::string description, int value,
                std::string personaId);

    std::unique_ptr<Item> clone() const override;
    void use(Character &character) override;

    const std::string &personaId() const { return personaId_; }

private:
    std::string personaId_;
};
