#include "InventoryScene.h"
#include "GameManager.h"
#include "Inventory.h"
#include "Item.h"
#include "Persona.h"

#include <algorithm>
#include <string>

namespace
{
    std::string truncateName(const std::string &name, size_t maxLen)
    {
        if (name.length() <= maxLen)
            return name;
        return name.substr(0, maxLen - 3) + "...";
    }
}

void InventoryScene::handleInput(engine::IInput &input)
{
    auto visible = visibleItemIndices();
    int count = static_cast<int>(visible.size());

    if (count > 0)
    {
        if (input.wasKeyJustPressed(engine::Key::Up))
            selectedIndex_ = (selectedIndex_ - 1 + count) % count;
        if (input.wasKeyJustPressed(engine::Key::Down))
            selectedIndex_ = (selectedIndex_ + 1) % count;
    }
    else
    {
        selectedIndex_ = 0;
    }

    if ((input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E)) && count > 0)
    {
        auto &inventory = GameManager::instance().inventory();
        size_t inventoryIndex = visible[static_cast<size_t>(selectedIndex_)];
        Item *item = inventory.itemAt(inventoryIndex);
        if (!item)
        {
            message_ = "Cannot use item.";
        }
        else if (item->type() == ItemType::Persona)
        {
            auto *personaItem = dynamic_cast<PersonaItem *>(item);
            auto persona = personaItem ? GameManager::instance().findPersona(personaItem->personaId()) : nullptr;
            if (persona)
            {
                if (GameManager::instance().addPersonaToPlayer(persona))
                {
                    inventory.removeItem(inventoryIndex);
                    message_ = "Obtained Persona: " + persona->name();
                }
                else
                {
                    message_ = "Persona slots full or already owned.";
                }
            }
            else
            {
                message_ = "Unknown Persona.";
            }
        }
        else if (inventory.useItem(inventoryIndex, GameManager::instance().character()))
        {
            message_ = "Item used.";
        }
        else
        {
            message_ = "Cannot use item.";
        }

        visible = visibleItemIndices();
        if (selectedIndex_ >= static_cast<int>(visible.size()))
            selectedIndex_ = std::max(0, static_cast<int>(visible.size()) - 1);
        messageTimer_ = 2.0f;
    }

    if (input.wasKeyJustPressed(engine::Key::Escape))
        GameManager::instance().enterScene(GameManager::instance().isNight() ? SceneType::Night : SceneType::Town);
}

void InventoryScene::update(float deltaTime)
{
    if (messageTimer_ > 0.0f)
    {
        messageTimer_ -= deltaTime;
        if (messageTimer_ <= 0.0f)
        {
            messageTimer_ = 0.0f;
            message_.clear();
        }
    }
}

void InventoryScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    renderer.drawTexture(GameManager::instance().onSecondMap() ? "town_bg2" : "town_bg", {0, 0, 800, 600});
    renderer.drawRect({0, 0, 800, 600}, engine::Color(0, 0, 0, 160));

    renderer.drawRect({150, 50, 500, 500}, engine::Color(30, 30, 50, 230));
    renderer.drawText("Backpack", {170, 70}, 28, engine::Color::white());
    renderer.drawText("Consumables, Persona contracts, and other non-equipment items", {170, 104}, 13, engine::Color::gray());

    const auto &inventory = GameManager::instance().inventory();
    auto visible = visibleItemIndices();
    renderer.drawText("Items: " + std::to_string(visible.size()) + "   Capacity: " +
                          std::to_string(inventory.size()) + "/" + std::to_string(inventory.capacity()),
                      {170, 130}, 16, engine::Color::white());

    if (visible.empty())
    {
        renderer.drawText("(no usable backpack items)", {260, 290}, 18, engine::Color(120, 120, 120));
    }

    float cellW = 110.0f;
    float cellH = 100.0f;
    int cols = 4;
    int rows = 4;
    float gridX = 170.0f;
    float gridY = 165.0f;

    for (size_t rowIndex = 0; rowIndex < visible.size() && rowIndex < static_cast<size_t>(cols * rows); ++rowIndex)
    {
        size_t itemIndex = visible[rowIndex];
        Item *item = inventory.itemAt(itemIndex);
        if (!item)
            continue;

        int col = static_cast<int>(rowIndex) % cols;
        int row = static_cast<int>(rowIndex) / cols;
        float cx = gridX + col * cellW;
        float cy = gridY + row * cellH;
        bool selected = (static_cast<int>(rowIndex) == selectedIndex_);

        engine::Color borderColor = selected ? engine::Color::yellow() : engine::Color(80, 80, 100);
        renderer.drawRect({cx, cy, cellW - 4, cellH - 4}, engine::Color(20, 20, 40, 230));
        renderer.drawRect({cx, cy, cellW - 4, 2}, borderColor);
        renderer.drawRect({cx, cy + cellH - 6, cellW - 4, 2}, borderColor);
        renderer.drawRect({cx, cy, 2, cellH - 6}, borderColor);
        renderer.drawRect({cx + cellW - 6, cy, 2, cellH - 6}, borderColor);

        if (!item->textureId().empty())
            renderer.drawTexture(item->textureId(), {cx + (cellW - 48) / 2, cy + 4, 48, 48});
        else
            renderer.drawRect({cx + (cellW - 48) / 2, cy + 4, 48, 48}, engine::Color(60, 60, 80));

        renderer.drawText(truncateName(item->name(), 10), {cx + 4, cy + 54}, 11,
                          selected ? engine::Color::yellow() : engine::Color::white());
        renderer.drawText(item->typeString() + " x" + std::to_string(item->quantity()),
                          {cx + 4, cy + 66}, 10, engine::Color(200, 200, 0));
    }

    if (!message_.empty())
        renderer.drawText(message_, {170, 500}, 16, engine::Color::green());
    renderer.drawText("Enter: use item / obtain Persona   Esc: back   Equipment is managed in C:Status", {170, 525}, 13, engine::Color::gray());
}

std::vector<size_t> InventoryScene::visibleItemIndices() const
{
    std::vector<size_t> result;
    const auto &items = GameManager::instance().inventory().items();
    for (size_t i = 0; i < items.size(); ++i)
    {
        if (items[i] && items[i]->type() != ItemType::Equipment)
            result.push_back(i);
    }
    return result;
}
