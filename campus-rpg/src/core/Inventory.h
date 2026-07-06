#pragma once

#include "Item.h"

#include <memory>
#include <vector>

class Inventory
{
public:
    Inventory() = default;

    void addItem(std::unique_ptr<Item> item);
    bool removeItem(size_t index);
    bool useItem(size_t index, Character &character);
    void clear();

    const std::vector<std::unique_ptr<Item>> &items() const { return items_; }
    size_t size() const { return items_.size(); }
    bool empty() const { return items_.empty(); }

    Item *itemAt(size_t index) const;

private:
    std::vector<std::unique_ptr<Item>> items_;
};
