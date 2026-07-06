#include "Inventory.h"

void Inventory::addItem(std::unique_ptr<Item> item)
{
    if (item)
    {
        items_.push_back(std::move(item));
    }
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

Item *Inventory::itemAt(size_t index) const
{
    if (index >= items_.size())
        return nullptr;
    return items_[index].get();
}
