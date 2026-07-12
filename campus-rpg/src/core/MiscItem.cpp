#include "MiscItem.h"
#include "Character.h"

MiscItem::MiscItem(std::string id, std::string name, std::string description, int value, std::string textureId)
    : Item(std::move(id), std::move(name), std::move(description), value, ItemType::Food, std::move(textureId))
{
}

std::unique_ptr<Item> MiscItem::clone() const
{
    auto item = std::make_unique<MiscItem>(id_, name_, description_, value_, textureId_);
    item->setQuantity(quantity_);
    return item;
}

void MiscItem::use(Character & /*character*/)
{
    // Misc items have no direct use effect; they are for quests only.
}
