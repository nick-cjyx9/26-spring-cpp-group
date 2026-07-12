#include "StatusScene.h"
#include "GameManager.h"
#include "Character.h"
#include "Inventory.h"
#include "Item.h"
#include "Persona.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace
{
    const char *slotName(int idx)
    {
        switch (idx)
        {
        case 0:
            return "Weapon";
        case 1:
            return "Armor";
        case 2:
            return "Accessory";
        case 3:
            return "Relic";
        }
        return "";
    }

    EquipmentSlot indexToSlot(int idx)
    {
        switch (idx)
        {
        case 0:
            return EquipmentSlot::Weapon;
        case 1:
            return EquipmentSlot::Armor;
        case 2:
            return EquipmentSlot::Accessory;
        case 3:
            return EquipmentSlot::Relic;
        }
        return EquipmentSlot::None;
    }

    std::shared_ptr<EquipmentItem> equippedAt(const GameManager::EquippedGear &gear, int idx)
    {
        switch (idx)
        {
        case 0:
            return gear.weapon;
        case 1:
            return gear.armor;
        case 2:
            return gear.accessory;
        case 3:
            return gear.relic;
        }
        return nullptr;
    }

    std::string statsLine(const EquipmentItem &item)
    {
        std::string stats;
        if (item.strengthBonus() > 0)
            stats += "STR+" + std::to_string(item.strengthBonus()) + " ";
        if (item.magicBonus() > 0)
            stats += "MAG+" + std::to_string(item.magicBonus()) + " ";
        if (item.speedBonus() > 0)
            stats += "SPD+" + std::to_string(item.speedBonus()) + " ";
        return stats.empty() ? "No bonus" : stats;
    }

    std::string skillCostText(const Skill &skill)
    {
        if (skill.cost() <= 0)
            return "Free";
        return std::to_string(skill.cost()) + (skill.costType() == SkillCostType::HP ? " HP" : " SP");
    }

    std::string skillDescription(const Skill &skill)
    {
        std::string text = ::elementName(skill.element()) + " / " + skillCostText(skill) + " / Power " + std::to_string(skill.power());
        if (skill.isHealing())
            text += " - restores HP to the caster.";
        else if (skill.element() == Element::Physical)
            text += " - physical damage; can be dodged.";
        else
            text += " - magic damage using caster MAG and target affinity.";
        return text;
    }

    void drawPanel(engine::IRenderer &renderer, float x, float y, float w, float h, engine::Color border)
    {
        renderer.drawRect({x + 6, y + 6, w, h}, engine::Color(0, 0, 0, 100));
        renderer.drawRect({x, y, w, h}, engine::Color(18, 20, 38, 238));
        renderer.drawRect({x, y, w, 3}, border);
        renderer.drawRect({x, y + h - 3, w, 3}, border);
        renderer.drawRect({x, y, 3, h}, border);
        renderer.drawRect({x + w - 3, y, 3, h}, border);
        renderer.drawRect({x + 8, y + 34, w - 16, 1}, engine::Color(95, 105, 150, 190));
    }

    std::string arcanaTextureId(std::string arcana)
    {
        std::transform(arcana.begin(), arcana.end(), arcana.begin(), [](unsigned char c)
                       { return static_cast<char>(std::tolower(c)); });
        std::replace(arcana.begin(), arcana.end(), ' ', '_');
        return "arcana_" + arcana;
    }

    void drawNeonFrame(engine::IRenderer &renderer, const engine::Rect &rect, engine::Color color)
    {
        renderer.drawRect({rect.x - 8, rect.y - 8, rect.width + 16, rect.height + 16}, engine::Color(color.r, color.g, color.b, 26));
        renderer.drawRect({rect.x - 4, rect.y - 4, rect.width + 8, rect.height + 8}, engine::Color(color.r, color.g, color.b, 42));
        renderer.drawRect({rect.x - 2, rect.y - 2, rect.width + 4, 3}, color);
        renderer.drawRect({rect.x - 2, rect.y + rect.height - 1, rect.width + 4, 3}, color);
        renderer.drawRect({rect.x - 2, rect.y - 2, 3, rect.height + 4}, color);
        renderer.drawRect({rect.x + rect.width - 1, rect.y - 2, 3, rect.height + 4}, color);
    }

    void drawScrollbar(engine::IRenderer &renderer, float x, float y, float h, int first, int visible, int total)
    {
        if (total <= visible)
            return;
        renderer.drawRect({x, y, 5.0f, h}, engine::Color(50, 55, 85, 230));
        float thumbH = std::max(24.0f, h * static_cast<float>(visible) / static_cast<float>(total));
        float maxY = y + h - thumbH;
        float t = total <= visible ? 0.0f : static_cast<float>(first) / static_cast<float>(total - visible);
        renderer.drawRect({x, y + (maxY - y) * t, 5.0f, thumbH}, engine::Color(220, 210, 100, 240));
    }

    std::vector<std::string> wrapText(const std::string &text, size_t width)
    {
        std::vector<std::string> lines;
        std::istringstream words(text);
        std::string word;
        std::string line;
        while (words >> word)
        {
            if (!line.empty() && line.size() + 1 + word.size() > width)
            {
                lines.push_back(line);
                line.clear();
            }
            if (!line.empty())
                line += " ";
            line += word;
        }
        if (!line.empty())
            lines.push_back(line);
        return lines;
    }

    void drawEquipmentIcon(engine::IRenderer &renderer, const EquipmentItem *item, float x, float y, float size)
    {
        renderer.drawRect({x, y, size, size}, engine::Color(8, 8, 18, 230));
        renderer.drawRect({x, y, size, 2}, engine::Color(140, 150, 210));
        renderer.drawRect({x, y + size - 2, size, 2}, engine::Color(55, 60, 95));
        renderer.drawRect({x, y, 2, size}, engine::Color(140, 150, 210));
        renderer.drawRect({x + size - 2, y, 2, size}, engine::Color(55, 60, 95));
        if (item && !item->textureId().empty())
            renderer.drawTexture(std::string("equipment/") + item->textureId(), {x + 6, y + 6, size - 12, size - 12});
        else
            renderer.drawRect({x + 12, y + 12, size - 24, size - 24}, engine::Color(55, 55, 80));
    }
}

void StatusScene::handleInput(engine::IInput &input)
{
    if (input.wasKeyJustPressed(engine::Key::Escape))
    {
        if (showSkillDetails_)
        {
            showSkillDetails_ = false;
            return;
        }
        GameManager::instance().enterScene(GameManager::instance().isNight() ? SceneType::Night : SceneType::Town);
        return;
    }

    if (showSkillDetails_)
    {
        if (input.wasKeyJustPressed(engine::Key::N) || input.wasKeyJustPressed(engine::Key::Enter) ||
            input.wasKeyJustPressed(engine::Key::E))
            showSkillDetails_ = false;
        return;
    }

    if (input.wasKeyJustPressed(engine::Key::Left) || input.wasKeyJustPressed(engine::Key::A))
    {
        if (section_ == Section::Equipment)
            section_ = Section::Persona;
        else if (section_ == Section::Persona)
            section_ = Section::Profile;
        return;
    }
    if (input.wasKeyJustPressed(engine::Key::Right) || input.wasKeyJustPressed(engine::Key::D))
    {
        if (section_ == Section::Profile)
            section_ = Section::Persona;
        else if (section_ == Section::Persona)
            section_ = Section::Equipment;
        return;
    }

    if (section_ == Section::Profile)
    {
        if (input.wasKeyJustPressed(engine::Key::Up) || input.wasKeyJustPressed(engine::Key::W) ||
            input.wasKeyJustPressed(engine::Key::Down) || input.wasKeyJustPressed(engine::Key::S) ||
            input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
        {
            profilePage_ = 1 - profilePage_;
        }
    }
    else if (section_ == Section::Persona)
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

    if (input.wasKeyJustPressed(engine::Key::N))
        showSkillDetails_ = !showSkillDetails_;

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
            if (personaIndex_ < 0)
                personaIndex_ = 0;
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
    animationTime_ += deltaTime;
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
    renderer.drawRect({0, 0, 800, 600}, engine::Color(8, 9, 22));
    renderer.drawRect({0, 0, 800, 90}, engine::Color(18, 20, 45, 230));
    renderer.drawRect({0, 88, 800, 2}, engine::Color(220, 190, 80, 230));
    renderer.drawText("CHARACTER / PERSONA", {28, 24}, 30, engine::Color(245, 245, 255));
    renderer.drawText("Out-of-battle Persona management and equipment loadout", {32, 58}, 14, engine::Color(150, 160, 200));

    renderProfilePanel(renderer);
    renderPersonaPanel(renderer);
    renderEquipmentPanel(renderer);

    renderer.drawText("A/D: switch panel   Profile Up/Down: scroll   Persona Enter: switch   N: skills   Equipment Enter: equip   Esc: back",
                      {20, 570}, 12, engine::Color::gray());

    if (!message_.empty())
    {
        renderer.drawRect({230, 282, 340, 36}, engine::Color(0, 0, 0, 220));
        renderer.drawText(message_, {242, 293}, 16, engine::Color::green());
    }

    if (showSkillDetails_)
        renderSkillDetailsOverlay(renderer);
}

void StatusScene::renderProfilePanel(engine::IRenderer &renderer)
{
    float x = 20.0f, y = 105.0f, w = 220.0f, h = 445.0f;
    engine::Color border = (section_ == Section::Profile) ? engine::Color::yellow() : engine::Color(80, 170, 255);
    drawPanel(renderer, x, y, w, h, border);

    auto &character = GameManager::instance().character();
    Persona *persona = character.currentPersona();

    renderer.drawText(profilePage_ == 0 ? "Arcana Card" : "Profile Stats", {x + 14, y + 10}, 18,
                      section_ == Section::Profile ? engine::Color::yellow() : engine::Color::white());
    renderer.drawText("Page " + std::to_string(profilePage_ + 1) + "/2", {x + 156, y + 13}, 12, engine::Color::gray());

    float pulse = (std::sin(animationTime_ * 3.0f) + 1.0f) * 0.5f;
    auto cyanGlow = engine::Color(70, 225, 255, static_cast<unsigned char>(115 + pulse * 80));
    auto goldGlow = engine::Color(255, 220, 90, static_cast<unsigned char>(125 + pulse * 80));

    // Page 1: large contained Arcana card. Nothing is drawn outside the panel.
    if (profilePage_ == 0)
    {
        engine::Rect card{x + 37, y + 58, 146, 218};
        renderer.drawRect({card.x - 6, card.y - 6, card.width + 12, card.height + 12}, engine::Color(40, 190, 255, 32));
        renderer.drawRect({card.x - 2, card.y - 2, card.width + 4, card.height + 4}, engine::Color(8, 8, 24, 235));
        renderer.drawRect({card.x - 2, card.y - 2, card.width + 4, 2}, cyanGlow);
        renderer.drawRect({card.x - 2, card.y + card.height, card.width + 4, 2}, cyanGlow);
        renderer.drawRect({card.x - 2, card.y - 2, 2, card.height + 4}, cyanGlow);
        renderer.drawRect({card.x + card.width, card.y - 2, 2, card.height + 4}, cyanGlow);

        if (persona)
        {
            renderer.drawTexture(arcanaTextureId(persona->arcana()), {card.x + 10, card.y + 12, card.width - 20, card.height - 24});
            renderer.drawText(persona->arcana(), {x + 24, y + 296}, 18, goldGlow);
            std::string personaName = persona->name();
            if (personaName.size() > 18)
                personaName = personaName.substr(0, 17) + ".";
            renderer.drawText(personaName, {x + 24, y + 326}, 16, engine::Color::cyan());
            renderer.drawText("Lv" + std::to_string(persona->level()) + "  " + persona->arcana(), {x + 24, y + 354}, 13, engine::Color::gray());
        }
        else
        {
            renderer.drawText("No Persona", {card.x + 28, card.y + 98}, 15, engine::Color::gray());
        }

        for (int i = 0; i < 4; ++i)
        {
            float sy = card.y + 8.0f + std::fmod(animationTime_ * 30.0f + i * 42.0f, card.height - 16.0f);
            renderer.drawRect({card.x + 8, sy, card.width - 16, 1.0f}, engine::Color(100, 230, 255, 110));
        }

        renderer.drawText("Use Up/Down to view stats", {x + 24, y + 406}, 12, engine::Color::gray());
    }
    // Page 2: compact character/persona stats.
    else
    {
        float infoY = y + 58.0f;
        renderer.drawRect({x + 14, infoY - 10, w - 34, 350}, engine::Color(10, 12, 28, 165));
        renderer.drawText(character.name(), {x + 24, infoY}, 19, engine::Color(245, 245, 255));
        renderer.drawText("Lv." + std::to_string(character.level()), {x + 24, infoY + 34}, 15, engine::Color::yellow());
        renderer.drawText("Gold " + std::to_string(character.gold()), {x + 100, infoY + 34}, 15, engine::Color::yellow());
        renderer.drawText("HP " + std::to_string(character.hp()) + "/" + std::to_string(character.maxHp()),
                          {x + 24, infoY + 72}, 15, engine::Color(110, 255, 130));
        renderer.drawText("SP " + std::to_string(character.sp()) + "/" + std::to_string(character.maxSp()),
                          {x + 24, infoY + 104}, 15, engine::Color(100, 190, 255));
        renderer.drawText("EXP " + std::to_string(character.exp()) + "/" + std::to_string(character.expToNextLevel()),
                          {x + 24, infoY + 136}, 14, engine::Color::white());

        renderer.drawText("Current Persona", {x + 24, infoY + 182}, 15, engine::Color::cyan());
        if (persona)
        {
            std::string personaName = persona->name();
            if (personaName.size() > 18)
                personaName = personaName.substr(0, 17) + ".";
            renderer.drawText(personaName, {x + 24, infoY + 208}, 14, engine::Color::white());
            renderer.drawText("Base " + std::to_string(persona->strength()) + "/" +
                                  std::to_string(persona->magic()) + "/" + std::to_string(persona->speed()),
                              {x + 24, infoY + 232}, 12, engine::Color(170, 170, 190));
        }

        renderer.drawText("Final Stats", {x + 24, infoY + 274}, 14, engine::Color::gray());
        renderer.drawText("STR " + std::to_string(character.attack()), {x + 24, infoY + 302}, 14, engine::Color(255, 150, 150));
        renderer.drawText("MAG " + std::to_string(character.magic()), {x + 88, infoY + 302}, 14, engine::Color(150, 190, 255));
        renderer.drawText("SPD " + std::to_string(character.speed()), {x + 150, infoY + 302}, 14, engine::Color(255, 230, 120));
    }

    drawScrollbar(renderer, x + w - 12, y + 48, h - 88, profilePage_, 1, 2);
}
void StatusScene::renderPersonaPanel(engine::IRenderer &renderer)
{
    float x = 260.0f, y = 105.0f, w = 240.0f, h = 445.0f;
    engine::Color border = (section_ == Section::Persona) ? engine::Color::yellow() : engine::Color(80, 80, 120);
    drawPanel(renderer, x, y, w, h, border);

    auto &character = GameManager::instance().character();
    const auto &owned = character.ownedPersonas();
    renderer.drawText("Persona Deck " + std::to_string(owned.size()) + "/" + std::to_string(Character::kMaxOwnedPersonas),
                      {x + 12, y + 10}, 18, engine::Color::white());

    int visibleRows = 6;
    int start = std::max(0, personaIndex_ - visibleRows / 2);
    if (start + visibleRows > static_cast<int>(owned.size()))
        start = std::max(0, static_cast<int>(owned.size()) - visibleRows);

    float rowY = y + 48.0f;
    for (int row = 0; row < visibleRows; ++row)
    {
        int i = start + row;
        bool selected = section_ == Section::Persona && i == personaIndex_;
        engine::Color boxBorder = selected ? engine::Color::yellow() : engine::Color(80, 80, 100);
        renderer.drawRect({x + 10, rowY, w - 20, 44}, engine::Color(20, 20, 42, 235));
        renderer.drawRect({x + 10, rowY, w - 20, 2}, boxBorder);
        renderer.drawRect({x + 10, rowY + 42, w - 20, 2}, boxBorder);
        renderer.drawRect({x + 10, rowY, 2, 44}, boxBorder);
        renderer.drawRect({x + w - 12, rowY, 2, 44}, boxBorder);

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
        rowY += 49.0f;
    }
    drawScrollbar(renderer, x + w - 9, y + 48, visibleRows * 49.0f - 5.0f, start, visibleRows, static_cast<int>(owned.size()));
    if (static_cast<int>(owned.size()) > visibleRows)
        renderer.drawText("Showing " + std::to_string(start + 1) + "-" + std::to_string(std::min(start + visibleRows, static_cast<int>(owned.size()))) +
                              " / " + std::to_string(owned.size()),
                          {x + 28, y + 342}, 12, engine::Color::gray());

    renderSelectedPersonaDetail(renderer, x + 10, y + 365);
}

void StatusScene::renderSelectedPersonaDetail(engine::IRenderer &renderer, float x, float y)
{
    const auto &owned = GameManager::instance().character().ownedPersonas();
    if (personaIndex_ < 0 || personaIndex_ >= static_cast<int>(owned.size()) || !owned[personaIndex_])
        return;

    const auto &p = owned[personaIndex_];
    renderer.drawText("Skills (N for detail popup)", {x, y}, 14, engine::Color::cyan());
    float sy = y + 20.0f;
    int shown = 0;
    for (const auto &skill : p->skills())
    {
        if (!skill || shown >= 4)
            continue;
        renderer.drawText("- " + skill->name() + " (" + skillCostText(*skill) + ")",
                          {x, sy}, 12, engine::Color(200, 200, 200));
        sy += 18.0f;
        ++shown;
    }
}

void StatusScene::renderSkillDetailsOverlay(engine::IRenderer &renderer)
{
    const auto &owned = GameManager::instance().character().ownedPersonas();
    if (personaIndex_ < 0 || personaIndex_ >= static_cast<int>(owned.size()) || !owned[personaIndex_])
        return;

    const auto &p = owned[personaIndex_];
    renderer.drawRect({0, 0, 800, 600}, engine::Color(0, 0, 0, 155));

    float x = 110.0f, y = 95.0f, w = 580.0f, h = 410.0f;
    drawPanel(renderer, x, y, w, h, engine::Color(220, 190, 80));
    renderer.drawText(p->name() + " Skill Details", {x + 24, y + 12}, 22, engine::Color::white());
    renderer.drawText("N / Enter / E / Esc: close", {x + w - 190, y + 17}, 13, engine::Color::gray());

    float sy = y + 58.0f;
    for (const auto &skill : p->skills())
    {
        if (!skill || sy > y + h - 50.0f)
            continue;

        renderer.drawRect({x + 22, sy - 4, w - 44, 58}, engine::Color(12, 14, 30, 220));
        renderer.drawText(skill->name(), {x + 36, sy + 4}, 16, engine::Color::yellow());
        renderer.drawText(::elementName(skill->element()) + "  " + skillCostText(*skill) +
                              "  Power " + std::to_string(skill->power()),
                          {x + 220, sy + 6}, 13, engine::Color::cyan());

        auto lines = wrapText(skillDescription(*skill), 72);
        float lineY = sy + 28.0f;
        for (size_t i = 0; i < lines.size() && i < 2; ++i)
        {
            renderer.drawText(lines[i], {x + 36, lineY}, 12, engine::Color(210, 210, 225));
            lineY += 15.0f;
        }
        sy += 68.0f;
    }
}

void StatusScene::renderEquipmentPanel(engine::IRenderer &renderer)
{
    float x = 520.0f, y = 105.0f, w = 260.0f, h = 445.0f;
    engine::Color border = (section_ == Section::Equipment) ? engine::Color::yellow() : engine::Color(80, 80, 120);
    drawPanel(renderer, x, y, w, h, border);

    renderer.drawText("Equipment", {x + 12, y + 10}, 20, engine::Color::white());

    const auto &gear = GameManager::instance().equippedGear();
    float rowY = y + 50.0f;
    for (int i = 0; i < 4; ++i)
    {
        auto item = equippedAt(gear, i);
        bool selected = section_ == Section::Equipment && equipmentIndex_ == i;
        engine::Color color = selected ? engine::Color::yellow() : engine::Color::white();
        drawEquipmentIcon(renderer, item.get(), x + 14, rowY, 42.0f);
        renderer.drawText(std::string(selected ? "> " : "  ") + slotName(i), {x + 64, rowY + 2}, 14, color);
        renderer.drawText(item ? item->name() : "(empty)", {x + 64, rowY + 19}, 13,
                          item ? engine::Color::white() : engine::Color(100, 100, 100));
        if (item)
            renderer.drawText(statsLine(*item), {x + 64, rowY + 34}, 11, engine::Color::gray());
        rowY += 52.0f;
    }

    rowY += 8.0f;
    renderer.drawText("Backpack Equipment", {x + 12, rowY}, 16, engine::Color::cyan());
    rowY += 24.0f;

    auto eqIndices = equipmentInventoryIndices();
    if (eqIndices.empty())
    {
        renderer.drawText("(no equipment in backpack)", {x + 18, rowY}, 14, engine::Color(100, 100, 100));
        return;
    }

    const float listTop = rowY;
    const float itemStep = 42.0f;
    const float panelBottom = y + h - 14.0f;
    int maxVisible = std::max(1, static_cast<int>((panelBottom - listTop) / itemStep));
    int listIndex = std::max(0, equipmentIndex_ - 4);
    int start = std::max(0, listIndex - maxVisible / 2);
    if (start + maxVisible > static_cast<int>(eqIndices.size()))
        start = std::max(0, static_cast<int>(eqIndices.size()) - maxVisible);
    int end = std::min(static_cast<int>(eqIndices.size()), start + maxVisible);

    for (int i = start; i < end; ++i)
    {
        Item *raw = GameManager::instance().inventory().itemAt(eqIndices[static_cast<size_t>(i)]);
        auto *item = dynamic_cast<EquipmentItem *>(raw);
        if (!item)
            continue;
        bool selected = section_ == Section::Equipment && equipmentIndex_ == i + 4;
        engine::Color color = selected ? engine::Color::yellow() : engine::Color::white();
        drawEquipmentIcon(renderer, item, x + 14, rowY, 36.0f);
        renderer.drawText(std::string(selected ? "> " : "  ") + item->name(), {x + 56, rowY + 2}, 13, color);
        renderer.drawText(std::string(slotName(static_cast<int>(item->slot()) - 1)) + "  " + statsLine(*item),
                          {x + 56, rowY + 20}, 11, engine::Color::gray());
        rowY += itemStep;
    }

    if (static_cast<int>(eqIndices.size()) > maxVisible)
    {
        float barX = x + w - 12.0f;
        float barY = listTop;
        float barH = panelBottom - listTop;
        float thumbH = std::max(18.0f, barH * static_cast<float>(maxVisible) / static_cast<float>(eqIndices.size()));
        float denom = static_cast<float>(std::max(1, static_cast<int>(eqIndices.size()) - maxVisible));
        float thumbY = barY + (barH - thumbH) * (static_cast<float>(start) / denom);
        renderer.drawRect({barX, barY, 4.0f, barH}, engine::Color(45, 45, 70, 210));
        renderer.drawRect({barX, thumbY, 4.0f, thumbH}, engine::Color(220, 190, 80, 230));
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
