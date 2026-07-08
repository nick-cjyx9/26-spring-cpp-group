#include "Inventory.h"

Inventory::Inventory(size_t capacity) : capacity_(capacity) {}

bool Inventory::addItem(std::unique_ptr<Item> item)
{
    if (!item || isFull())
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
    items_[index]->use(character);
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
    auto item = std::move(items_[index]);
    items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(index));
    return item;
}

Item *Inventory::itemAt(size_t index) const
{
    if (index >= items_.size())
        return nullptr;
    return items_[index].get();
}
