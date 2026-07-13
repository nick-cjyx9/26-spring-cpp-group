#pragma once

#include <memory>

#include "Item.h"

// Miscellaneous item used for quest collectibles (e.g. monster drops, tarot fragments).
// Unlike Food/Potion, using a MiscItem has no gameplay effect.
class MiscItem : public Item
{
public:
    MiscItem(std::string id, std::string name, std::string description, int value, std::string textureId = "");

    std::unique_ptr<Item> clone() const override;
    void use(Character &character) override;
};
