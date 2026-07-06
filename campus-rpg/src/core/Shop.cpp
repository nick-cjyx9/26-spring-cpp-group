#include "Shop.h"
#include "Character.h"

void Shop::addItem(std::unique_ptr<Item> item)
{
    if (item)
    {
        items_.push_back(std::move(item));
    }
}

bool Shop::buy(size_t index, Character &buyer, Inventory &targetInventory)
{
    if (index >= items_.size())
        return false;
    int price = items_[index]->value();
    if (!buyer.spendGold(price))
        return false;

    targetInventory.addItem(items_[index]->clone());
    return true;
}

bool Shop::sell(size_t inventoryIndex, Character &seller, Inventory &sellerInventory)
{
    Item *item = sellerInventory.itemAt(inventoryIndex);
    if (!item)
        return false;

    int price = item->value() / 2;
    seller.addGold(price);
    sellerInventory.removeItem(inventoryIndex);
    return true;
}
