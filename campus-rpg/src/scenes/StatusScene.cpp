#include "StatusScene.h"
#include "GameManager.h"
#include "Character.h"
#include "Inventory.h"
#include "Item.h"
#include "Persona.h"

#include <algorithm>

namespace
{
    const char *slotName(int idx)
    {
        switch (idx)
        {
        case 0: return "Weapon";
        case 1: return "Armor";
        case 2: return "Accessory";
        case 3: return "Relic";
        }
        return "";
    }

    EquipmentSlot indexToSlot(int idx)
    {
        switch (idx)
        {
        case 0: return EquipmentSlot::Weapon;
        case 1: return EquipmentSlot::Armor;
        case 2: return EquipmentSlot::Accessory;
        case 3: return EquipmentSlot::Relic;
        }
        return EquipmentSlot::None;
    }

    std::shared_ptr<EquipmentItem> equippedAt(const GameManager::EquippedGear &gear, int idx)
    {
        switch (idx)
        {
        case 0: return gear.weapon;
        case 1: return gear.armor;
        case 2: return gear.accessory;
        case 3: return gear.relic;
        }
        return nullptr;
    }

    std::string statsLine(const EquipmentItem &item)
    {
        std::string stats;
        if (item.strengthBonus() > 0) stats += "STR+" + std::to_string(item.strengthBonus()) + " ";
        if (item.magicBonus() > 0) stats += "MAG+" + std::to_string(item.magicBonus()) + " ";
        if (item.speedBonus() > 0) stats += "SPD+" + std::to_string(item.speedBonus()) + " ";
        return stats.empty() ? "No bonus" : stats;
    }

    std::string skillCostText(const Skill &skill)
    {
        if (skill.cost() <= 0)
            return "Free";
        return std::to_string(skill.cost()) + (skill.costType() == SkillCostType::HP ? " HP" : " SP");
    }
}

void StatusScene::handleInput(engine::IInput &input)
{
    if (input.wasKeyJustPressed(engine::Key::Escape))
    {
        GameManager::instance().enterScene(GameManager::instance().isNight() ? SceneType::Night : SceneType::Town);
        return;
    }

    if (input.wasKeyJustPressed(engine::Key::Left) || input.wasKeyJustPressed(engine::Key::A))
    {
        section_ = Section::Persona;
        return;
    }
    if (input.wasKeyJustPressed(engine::Key::Right) || input.wasKeyJustPressed(engine::Key::D))
    {
        section_ = Section::Equipment;
        return;
    }

    if (section_ == Section::Persona)
        handlePersonaInput(input);
    else
        handleEquipmentInput(input);
}

void StatusScene::handlePersonaInput(engine::IInput &input)
{
    auto &gm = GameManager::instance();
    auto &owned = gm.character().ownedPersonas();
    int count = static_cast<int>(owned.size());
    if (count <= 0)
    {
        personaIndex_ = 0;
        return;
    }

    if (input.wasKeyJustPressed(engine::Key::Up) || input.wasKeyJustPressed(engine::Key::W))
        personaIndex_ = (personaIndex_ - 1 + count) % count;
    if (input.wasKeyJustPressed(engine::Key::Down) || input.wasKeyJustPressed(engine::Key::S))
        personaIndex_ = (personaIndex_ + 1) % count;

    if ((input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E)) &&
        personaIndex_ >= 0 && personaIndex_ < count && owned[personaIndex_])
    {
        gm.setPlayerPersona(owned[personaIndex_]);
        message_ = "Persona equipped: " + owned[personaIndex_]->name();
        messageTimer_ = 2.0f;
    }

    if (input.wasKeyJustPressed(engine::Key::Space) &&
        personaIndex_ >= 0 && personaIndex_ < count && owned[personaIndex_])
    {
        std::string id = owned[personaIndex_]->id();
        std::string name = owned[personaIndex_]->name();
        if (gm.destroyPlayerPersona(id))
        {
            message_ = "Destroyed Persona: " + name;
            personaIndex_ = std::min(personaIndex_, static_cast<int>(gm.character().ownedPersonas().size()) - 1);
            if (personaIndex_ < 0) personaIndex_ = 0;
        }
        else
        {
            message_ = "Cannot destroy current/last Persona.";
        }
        messageTimer_ = 2.0f;
    }
}

void StatusScene::handleEquipmentInput(engine::IInput &input)
{
    auto eqIndices = equipmentInventoryIndices();
    int count = 4 + static_cast<int>(eqIndices.size());
    if (count <= 0)
        return;

    if (input.wasKeyJustPressed(engine::Key::Up) || input.wasKeyJustPressed(engine::Key::W))
        equipmentIndex_ = (equipmentIndex_ - 1 + count) % count;
    if (input.wasKeyJustPressed(engine::Key::Down) || input.wasKeyJustPressed(engine::Key::S))
        equipmentIndex_ = (equipmentIndex_ + 1) % count;

    if (!(input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E)))
        return;

    auto &gm = GameManager::instance();
    if (equipmentIndex_ < 4)
    {
        EquipmentSlot slot = indexToSlot(equipmentIndex_);
        auto item = equippedAt(gm.equippedGear(), equipmentIndex_);
        if (!item)
            message_ = "That equipment slot is empty.";
        else if (gm.unequipToInventory(slot))
            message_ = "Unequipped: " + item->name();
        else
            message_ = "Backpack full. Cannot unequip.";
    }
    else
    {
        size_t inventoryIndex = eqIndices[static_cast<size_t>(equipmentIndex_ - 4)];
        Item *item = gm.inventory().itemAt(inventoryIndex);
        std::string name = item ? item->name() : "";
        if (gm.equipFromInventory(inventoryIndex))
        {
            message_ = "Equipped: " + name;
            auto refreshed = equipmentInventoryIndices();
            equipmentIndex_ = std::min(equipmentIndex_, 4 + static_cast<int>(refreshed.size()) - 1);
        }
        else
        {
            message_ = "Cannot equip item.";
        }
    }
    messageTimer_ = 2.0f;
}

void StatusScene::update(float deltaTime)
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

void StatusScene::render(engine::IRenderer &renderer)
{
    renderer.clear();
    renderer.drawRect({0, 0, 800, 600}, engine::Color(15, 15, 30));

    renderPersonaPanel(renderer);
    renderEquipmentPanel(renderer);

    renderer.drawText("A/D: switch panel   Persona Enter: switch   Persona Space: destroy   Equipment Enter: equip/unequip   Esc: back",
                      {30, 570}, 13, engine::Color::gray());

    if (!message_.empty())
    {
        renderer.drawRect({230, 282, 340, 36}, engine::Color(0, 0, 0, 220));
        renderer.drawText(message_, {242, 293}, 16, engine::Color::green());
    }
}

void StatusScene::renderPersonaPanel(engine::IRenderer &renderer)
{
    float x = 20.0f, y = 20.0f, w = 280.0f, h = 530.0f;
    engine::Color border = (section_ == Section::Persona) ? engine::Color::yellow() : engine::Color(80, 80, 100);
    renderer.drawRect({x, y, w, h}, engine::Color(25, 25, 50, 230));
    renderer.drawRect({x, y, w, 2}, border);
    renderer.drawRect({x, y + h - 2, w, 2}, border);
    renderer.drawRect({x, y, 2, h}, border);
    renderer.drawRect({x + w - 2, y, 2, h}, border);

    auto &character = GameManager::instance().character();
    const auto &owned = character.ownedPersonas();
    renderer.drawText("Persona Slots " + std::to_string(owned.size()) + "/" + std::to_string(Character::kMaxOwnedPersonas),
                      {x + 10, y + 10}, 18, engine::Color::white());
    renderer.drawText(character.name() + "  Lv." + std::to_string(character.level()) +
                          "  HP " + std::to_string(character.hp()) + "/" + std::to_string(character.maxHp()) +
                          "  SP " + std::to_string(character.sp()) + "/" + std::to_string(character.maxSp()),
                      {x + 10, y + 36}, 12, engine::Color(200, 200, 220));

    float rowY = y + 65.0f;
    for (int i = 0; i < static_cast<int>(Character::kMaxOwnedPersonas); ++i)
    {
        bool selected = section_ == Section::Persona && i == personaIndex_;
        engine::Color boxBorder = selected ? engine::Color::yellow() : engine::Color(80, 80, 100);
        renderer.drawRect({x + 10, rowY, w - 20, 48}, engine::Color(20, 20, 40, 230));
        renderer.drawRect({x + 10, rowY, w - 20, 2}, boxBorder);
        renderer.drawRect({x + 10, rowY + 46, w - 20, 2}, boxBorder);
        renderer.drawRect({x + 10, rowY, 2, 48}, boxBorder);
        renderer.drawRect({x + w - 12, rowY, 2, 48}, boxBorder);

        if (i < static_cast<int>(owned.size()) && owned[i])
        {
            bool current = character.currentPersona() && character.currentPersona()->id() == owned[i]->id();
            renderer.drawText(std::string(selected ? "> " : "  ") + owned[i]->name() + (current ? " *" : ""),
                              {x + 18, rowY + 7}, 15, selected ? engine::Color::yellow() : engine::Color::white());
            renderer.drawText(owned[i]->arcana() + " Lv" + std::to_string(owned[i]->level()) +
                                  "  " + std::to_string(owned[i]->strength()) + "/" +
                                  std::to_string(owned[i]->magic()) + "/" + std::to_string(owned[i]->speed()),
                              {x + 28, rowY + 28}, 12, engine::Color::gray());
        }
        else
        {
            renderer.drawText("(empty)", {x + 28, rowY + 16}, 14, engine::Color(100, 100, 100));
        }
        rowY += 56.0f;
    }

    renderSelectedPersonaDetail(renderer, x + 10, y + 410);
}

void StatusScene::renderSelectedPersonaDetail(engine::IRenderer &renderer, float x, float y)
{
    const auto &owned = GameManager::instance().character().ownedPersonas();
    if (personaIndex_ < 0 || personaIndex_ >= static_cast<int>(owned.size()) || !owned[personaIndex_])
        return;

    const auto &p = owned[personaIndex_];
    renderer.drawText("Skills:", {x, y}, 14, engine::Color::cyan());
    float sy = y + 20.0f;
    int shown = 0;
    for (const auto &skill : p->skills())
    {
        if (!skill || shown >= 4) continue;
        renderer.drawText("- " + skill->name() + " (" + skillCostText(*skill) + ")",
                          {x, sy}, 12, engine::Color(200, 200, 200));
        sy += 18.0f;
        ++shown;
    }
}

void StatusScene::renderEquipmentPanel(engine::IRenderer &renderer)
{
    float x = 320.0f, y = 20.0f, w = 460.0f, h = 530.0f;
    engine::Color border = (section_ == Section::Equipment) ? engine::Color::yellow() : engine::Color(80, 80, 100);
    renderer.drawRect({x, y, w, h}, engine::Color(25, 25, 50, 230));
    renderer.drawRect({x, y, w, 2}, border);
    renderer.drawRect({x, y + h - 2, w, 2}, border);
    renderer.drawRect({x, y, 2, h}, border);
    renderer.drawRect({x + w - 2, y, 2, h}, border);

    renderer.drawText("Equipment", {x + 10, y + 10}, 20, engine::Color::white());
    renderer.drawText("Equipped slots + backpack equipment only", {x + 140, y + 15}, 13, engine::Color::gray());

    const auto &gear = GameManager::instance().equippedGear();
    float rowY = y + 50.0f;
    for (int i = 0; i < 4; ++i)
    {
        auto item = equippedAt(gear, i);
        bool selected = section_ == Section::Equipment && equipmentIndex_ == i;
        engine::Color color = selected ? engine::Color::yellow() : engine::Color::white();
        renderer.drawText(std::string(selected ? "> " : "  ") + slotName(i) + ": " + (item ? item->name() : "(empty)"),
                          {x + 18, rowY}, 15, color);
        if (item)
            renderer.drawText(statsLine(*item), {x + 245, rowY}, 12, engine::Color::gray());
        rowY += 30.0f;
    }

    rowY += 16.0f;
    renderer.drawText("Backpack Equipment", {x + 10, rowY}, 17, engine::Color::cyan());
    rowY += 28.0f;

    auto eqIndices = equipmentInventoryIndices();
    if (eqIndices.empty())
    {
        renderer.drawText("(no equipment in backpack)", {x + 18, rowY}, 14, engine::Color(100, 100, 100));
        return;
    }

    int maxVisible = 10;
    int listIndex = std::max(0, equipmentIndex_ - 4);
    int start = std::max(0, listIndex - maxVisible / 2);
    if (start + maxVisible > static_cast<int>(eqIndices.size()))
        start = std::max(0, static_cast<int>(eqIndices.size()) - maxVisible);
    int end = std::min(static_cast<int>(eqIndices.size()), start + maxVisible);

    for (int i = start; i < end; ++i)
    {
        Item *raw = GameManager::instance().inventory().itemAt(eqIndices[static_cast<size_t>(i)]);
        auto *item = dynamic_cast<EquipmentItem *>(raw);
        if (!item) continue;
        bool selected = section_ == Section::Equipment && equipmentIndex_ == i + 4;
        engine::Color color = selected ? engine::Color::yellow() : engine::Color::white();
        renderer.drawText(std::string(selected ? "> " : "  ") + item->name() + " [" + slotName(static_cast<int>(item->slot()) - 1) + "]",
                          {x + 18, rowY}, 14, color);
        renderer.drawText(statsLine(*item), {x + 255, rowY}, 12, engine::Color::gray());
        rowY += 28.0f;
    }
}

std::vector<size_t> StatusScene::equipmentInventoryIndices() const
{
    std::vector<size_t> result;
    const auto &items = GameManager::instance().inventory().items();
    for (size_t i = 0; i < items.size(); ++i)
    {
        if (items[i] && items[i]->type() == ItemType::Equipment)
            result.push_back(i);
    }
    return result;
}
