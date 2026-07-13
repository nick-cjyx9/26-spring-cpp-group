#include "Inventory.h"

#include <algorithm>

Inventory::Inventory(size_t capacity) : capacity_(capacity) {}

bool Inventory::addItem(std::unique_ptr<Item> item)
{
    if (!item)
        return false;

    if (item->type() == ItemType::Equipment)
    {
        for (auto &existing : items_)
        {
            if (existing && existing->id() == item->id())
            {
                existing->addQuantity(1);
                return true;
            }
        }
    }

    if (isFull())
        return false;
    items_.push_back(std::move(item));
    return true;
}

bool Inventory::removeItem(size_t index)
{
    if (index >= items_.size())
        return false;
    items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

bool Inventory::useItem(size_t index, Character &character)
{
    if (index >= items_.size())
        return false;
    auto *item = items_[index].get();
    item->use(character);
    if (item->quantity() > 1)
    {
        item->addQuantity(-1);
        return true;
    }
    items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

void Inventory::clear()
{
    items_.clear();
}

std::unique_ptr<Item> Inventory::takeItem(size_t index)
{
    if (index >= items_.size())
        return nullptr;
    auto *item = items_[index].get();
    if (item->type() == ItemType::Equipment && item->quantity() > 1)
    {
        item->addQuantity(-1);
        return item->clone();
    }
    auto taken = std::move(items_[index]);
    items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(index));
    return taken;
}

Item *Inventory::itemAt(size_t index) const
{
    if (index >= items_.size())
        return nullptr;
    return items_[index].get();
}

Item *Inventory::findItemById(const std::string &id) const
{
    for (const auto &item : items_)
    {
        if (item && item->id() == id)
            return item.get();
    }
    return nullptr;
}

bool Inventory::removeItemById(const std::string &id, int quantity)
{
    if (quantity <= 0)
        return false;
    for (auto it = items_.begin(); it != items_.end(); ++it)
    {
        auto &item = *it;
        if (!item || item->id() != id)
            continue;
        if (item->quantity() > quantity)
        {
            item->addQuantity(-quantity);
            return true;
        }
        if (item->quantity() == quantity)
        {
            items_.erase(it);
            return true;
        }
        return false; // not enough
    }
    return false; // not found
}

int Inventory::countItem(const std::string &id) const
{
    for (const auto &item : items_)
    {
        if (item && item->id() == id)
            return item->quantity();
    }
    return 0;
}
