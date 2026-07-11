#include "InventoryScene.h"
#include "GameManager.h"
#include "Inventory.h"
#include "Item.h"
#include "Persona.h"

#include <algorithm>
#include <string>

namespace
{
    const char *slotName(EquipmentSlot s)
    {
        switch (s)
        {
        case EquipmentSlot::Weapon: return "Weapon";
        case EquipmentSlot::Armor: return "Armor";
        case EquipmentSlot::Accessory: return "Accessory";
        case EquipmentSlot::Relic: return "Relic";
        default: return "None";
        }
    }

    EquipmentSlot digitToSlot(int key)
    {
        switch (key)
        {
        case 1: return EquipmentSlot::Weapon;
        case 2: return EquipmentSlot::Armor;
        case 3: return EquipmentSlot::Accessory;
        case 4: return EquipmentSlot::Relic;
        default: return EquipmentSlot::None;
        }
    }

    std::vector<std::string> wrapText(const std::string &text, size_t maxChars)
    {
        std::vector<std::string> lines;
        if (text.empty())
            return lines;
        size_t start = 0;
        while (start < text.size())
        {
            size_t len = std::min(maxChars, text.size() - start);
            lines.push_back(text.substr(start, len));
            start += len;
        }
        return lines;
    }
} // namespace

void InventoryScene::handleInput(engine::IInput &input)
{
    auto &inventory = GameManager::instance().inventory();
    size_t count = inventory.size();

    if (count > 0)
    {
        if (input.wasKeyJustPressed(engine::Key::Up))
            selectedIndex_ = (selectedIndex_ - 1 + static_cast<int>(count)) % static_cast<int>(count);
        if (input.wasKeyJustPressed(engine::Key::Down))
            selectedIndex_ = (selectedIndex_ + 1) % static_cast<int>(count);
    }

    // Unequip a gear slot directly with keys 1..4.
    for (int d = 1; d <= 4; ++d)
    {
        engine::Key key = static_cast<engine::Key>(static_cast<int>(engine::Key::Num1) + (d - 1));
        if (input.wasKeyJustPressed(key))
        {
            EquipmentSlot s = digitToSlot(d);
            const auto &gear = GameManager::instance().equippedGear();
            bool has = (s == EquipmentSlot::Weapon && gear.weapon) ||
                       (s == EquipmentSlot::Armor && gear.armor) ||
                       (s == EquipmentSlot::Accessory && gear.accessory) ||
                       (s == EquipmentSlot::Relic && gear.relic);
            if (has)
            {
                if (GameManager::instance().unequipToInventory(s))
                    message_ = std::string("Unequipped ") + slotName(s) + ".";
                else
                    message_ = "Backpack full. Cannot unequip.";
                messageTimer_ = 2.0f;
            }
            else
            {
                message_ = std::string(slotName(s)) + " slot is empty.";
                messageTimer_ = 2.0f;
            }
        }
    }

    if ((input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E)) && count > 0)
    {
        Item *item = inventory.itemAt(static_cast<size_t>(selectedIndex_));
        if (!item)
        {
            message_ = "Cannot use item.";
        }
        else if (item->type() == ItemType::Equipment)
        {
            std::string itemName = item->name();
            if (GameManager::instance().equipFromInventory(static_cast<size_t>(selectedIndex_)))
                message_ = "Equipped: " + itemName;
            else
                message_ = "Cannot equip item.";
        }
        else if (item->type() == ItemType::Persona)
        {
            auto *personaItem = dynamic_cast<PersonaItem *>(item);
            auto persona = personaItem ? GameManager::instance().findPersona(personaItem->personaId()) : nullptr;
            if (persona)
            {
                GameManager::instance().addPersonaToPlayer(persona);
                inventory.removeItem(static_cast<size_t>(selectedIndex_));
                message_ = "Obtained Persona: " + persona->name();
            }
            else
            {
                message_ = "Unknown Persona.";
            }
        }
        else if (inventory.useItem(static_cast<size_t>(selectedIndex_), GameManager::instance().character()))
        {
            message_ = "Item used.";
        }
        else
        {
            message_ = "Cannot use item.";
        }

        if (selectedIndex_ >= static_cast<int>(inventory.size()))
            selectedIndex_ = std::max(0, static_cast<int>(inventory.size()) - 1);
        messageTimer_ = 2.0f;
    }

    if (input.wasKeyJustPressed(engine::Key::Escape))
    {
        GameManager::instance().enterScene(GameManager::instance().isNight() ? SceneType::Night : SceneType::Town);
    }
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

    renderer.drawTexture("town_bg", {0, 0, 800, 600});
    renderer.drawRect({0, 0, 800, 600}, engine::Color(0, 0, 0, 160));

    renderer.drawRect({150, 50, 500, 500}, engine::Color(30, 30, 50, 230));
    renderer.drawText("Inventory", {170, 70}, 28, engine::Color::white());

    // ---- Equipment slots panel ----
    const auto &gear = GameManager::instance().equippedGear();
    renderer.drawText("Equipped (1-4 to unequip)", {170, 108}, 14, engine::Color(200, 200, 0));

    auto drawSlot = [&](int idx, const std::shared_ptr<EquipmentItem> &g, float y) {
        std::string label = std::to_string(idx) + ". " + std::string(slotName(static_cast<EquipmentSlot>(
            static_cast<int>(EquipmentSlot::Weapon) + (idx - 1))));
        std::string val = g ? g->name() : "(empty)";
        renderer.drawText(label + ": " + val, {170, y}, 15,
                          g ? engine::Color::green() : engine::Color(120, 120, 120));
    };
    drawSlot(1, gear.weapon, 128);
    drawSlot(2, gear.armor, 148);
    drawSlot(3, gear.accessory, 168);
    drawSlot(4, gear.relic, 188);

    // ---- Inventory grid ----
    const auto &inventory = GameManager::instance().inventory();
    const auto &items = inventory.items();
    renderer.drawText("Capacity: " + std::to_string(items.size()) + "/" + std::to_string(inventory.capacity()),
                      {170, 212}, 16, engine::Color::white());

    float cellW = 110.0f;
    float cellH = 100.0f;
    int cols = 4;
    int rows = 3;
    float gridX = 170.0f;
    float gridY = 235.0f;

    for (size_t i = 0; i < items.size() && i < static_cast<size_t>(cols * rows); ++i)
    {
        int col = static_cast<int>(i) % cols;
        int row = static_cast<int>(i) / cols;
        float cx = gridX + col * cellW;
        float cy = gridY + row * cellH;

        engine::Color borderColor = (static_cast<int>(i) == selectedIndex_)
            ? engine::Color::yellow() : engine::Color(80, 80, 100);
        renderer.drawRect({cx, cy, cellW - 4, cellH - 4}, engine::Color(20, 20, 40, 230));
        renderer.drawRect({cx, cy, cellW - 4, 2}, borderColor);
        renderer.drawRect({cx, cy + cellH - 6, cellW - 4, 2}, borderColor);
        renderer.drawRect({cx, cy, 2, cellH - 6}, borderColor);
        renderer.drawRect({cx + cellW - 6, cy, 2, cellH - 6}, borderColor);

        // Item image
        const auto &texId = items[i]->textureId();
        float imgSize = 44.0f;
        if (!texId.empty())
        {
            std::string texPath;
            if (texId.find('/') != std::string::npos)
                texPath = texId;
            else if (items[i]->type() == ItemType::Equipment)
                texPath = std::string("equipment/") + texId;
            else
                texPath = std::string("items/") + texId;
            renderer.drawTexture(texPath, {cx + (cellW - 4 - imgSize) / 2, cy + 4, imgSize, imgSize});
        }
        else
        {
            renderer.drawRect({cx + (cellW - 4 - imgSize) / 2, cy + 4, imgSize, imgSize},
                              engine::Color(60, 60, 80));
        }

        // Wrapped name (max 2 lines, ~12 chars per line)
        auto lines = wrapText(items[i]->name(), 12);
        engine::Color nameColor = (static_cast<int>(i) == selectedIndex_)
            ? engine::Color::yellow() : engine::Color::white();
        float nameY = cy + 4 + imgSize + 4;
        for (size_t li = 0; li < lines.size() && li < 2; ++li)
        {
            renderer.drawText(lines[li], {cx + 4, nameY + li * 14.0f}, 11, nameColor);
        }

        // Quantity
        float qtyY = nameY + std::min(lines.size(), size_t(2)) * 14.0f;
        renderer.drawText("x" + std::to_string(items[i]->quantity()),
                          {cx + 4, qtyY}, 10, engine::Color(200, 200, 0));
    }

    if (!message_.empty())
        renderer.drawText(message_, {170, 475}, 16, engine::Color::green());
    renderer.drawText("Enter: use/equip   1-4: unequip   Esc: back", {170, 520}, 14, engine::Color::gray());
}
