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
    // Detach and return the item at index (returns nullptr if out of range).
    std::unique_ptr<Item> takeItem(size_t index);
    void clear();

    const std::vector<std::unique_ptr<Item>> &items() const { return items_; }
    size_t size() const { return items_.size(); }
    size_t capacity() const { return capacity_; }
    bool empty() const { return items_.empty(); }
    bool isFull() const { return items_.size() >= capacity_; }

    Item *itemAt(size_t index) const;

    // Find the first item with the given id. Returns nullptr if not found.
    Item *findItemById(const std::string &id) const;
    // Remove a specific quantity of an item by id. Returns false if not enough.
    bool removeItemById(const std::string &id, int quantity = 1);
    // Count total quantity of an item by id.
    int countItem(const std::string &id) const;

private:
    std::vector<std::unique_ptr<Item>> items_;
    size_t capacity_ = 20;
};
