#include "Inventory.h"

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
