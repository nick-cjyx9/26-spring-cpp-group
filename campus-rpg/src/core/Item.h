#pragma once

#include <algorithm>
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

// Equipment slot types for the gear system.
enum class EquipmentSlot
{
    None,
    Weapon,
    Armor,
    Accessory,
    Relic
};

class Item
{
public:
    Item(std::string id, std::string name, std::string description, int value, ItemType type, std::string textureId = "");
    virtual ~Item() = default;

    virtual std::unique_ptr<Item> clone() const = 0;
    virtual void use(Character &character) = 0;

    const std::string &id() const { return id_; }
    const std::string &name() const { return name_; }
    const std::string &description() const { return description_; }
    int value() const { return value_; }
    ItemType type() const { return type_; }

    std::string typeString() const;

    int quantity() const { return quantity_; }
    void setQuantity(int qty) { quantity_ = std::max(1, qty); }
    void addQuantity(int delta) { quantity_ += delta; }

    const std::string &textureId() const { return textureId_; }
    void setTextureId(const std::string &id) { textureId_ = id; }

protected:
    std::string id_;
    std::string name_;
    std::string description_;
    int value_ = 0;
    ItemType type_;
    int quantity_ = 1;
    std::string textureId_;
};

class FoodItem : public Item
{
public:
    FoodItem(std::string id, std::string name, std::string description, int value, int healAmount, std::string textureId = "");

    std::unique_ptr<Item> clone() const override;
    void use(Character &character) override;

    int healAmount() const { return healAmount_; }

private:
    int healAmount_ = 0;
};

class PotionItem : public Item
{
public:
    PotionItem(std::string id, std::string name, std::string description, int value, int healAmount, std::string textureId = "");

    std::unique_ptr<Item> clone() const override;
    void use(Character &character) override;

    int healAmount() const { return healAmount_; }

private:
    int healAmount_ = 0;
};

class SpItem : public Item
{
public:
    SpItem(std::string id, std::string name, std::string description, int value, int spAmount, std::string textureId = "");

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
                  int strengthBonus, int magicBonus, int speedBonus = 0,
                  EquipmentSlot slot = EquipmentSlot::None,
                  std::string textureId = "");

    std::unique_ptr<Item> clone() const override;
    void use(Character &character) override;

    int strengthBonus() const { return strengthBonus_; }
    int magicBonus() const { return magicBonus_; }
    int speedBonus() const { return speedBonus_; }
    EquipmentSlot slot() const { return slot_; }

private:
    int strengthBonus_ = 0;
    int magicBonus_ = 0;
    int speedBonus_ = 0;
    EquipmentSlot slot_ = EquipmentSlot::None;
};

class PersonaItem : public Item
{
public:
    PersonaItem(std::string id, std::string name, std::string description, int value,
                std::string personaId, std::string textureId = "");

    std::unique_ptr<Item> clone() const override;
    void use(Character & /*character*/) override;

    const std::string &personaId() const { return personaId_; }

private:
    std::string personaId_;
};
