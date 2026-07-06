#pragma once

#include "Item.h"

#include <memory>
#include <vector>

class Inventory
{
public:
    explicit Inventory(size_t capacity = 20);

    bool addItem(std::unique_ptr<Item> item);
    bool removeItem(size_t index);
    bool useItem(size_t index, Character &character);
    void clear();

    const std::vector<std::unique_ptr<Item>> &items() const { return items_; }
    size_t size() const { return items_.size(); }
    size_t capacity() const { return capacity_; }
    bool empty() const { return items_.empty(); }
    bool isFull() const { return items_.size() >= capacity_; }

    Item *itemAt(size_t index) const;

private:
    std::vector<std::unique_ptr<Item>> items_;
    size_t capacity_ = 20;
};
