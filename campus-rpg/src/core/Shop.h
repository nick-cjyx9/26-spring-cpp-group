#pragma once

#include "Inventory.h"
#include "Item.h"

#include <memory>
#include <vector>

class Shop
{
public:
    Shop() = default;

    void addItem(std::unique_ptr<Item> item);

    const std::vector<std::unique_ptr<Item>> &items() const { return items_; }

    // Buy: index is the shop item index, target receives a clone of the item.
    bool buy(size_t index, Character &buyer, Inventory &targetInventory);

    // Sell: removes item from inventory at index and gives buyer gold.
    bool sell(size_t inventoryIndex, Character &seller, Inventory &sellerInventory);

private:
    std::vector<std::unique_ptr<Item>> items_;
};
