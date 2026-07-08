#include "StatusScene.h"
#include "GameManager.h"
#include "Character.h"
#include "Item.h"
#include "Inventory.h"

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

    engine::Color slotColor(int idx)
    {
        switch (idx)
        {
        case 0:
            return engine::Color(255, 80, 80); // Weapon - red
        case 1:
            return engine::Color(80, 80, 255); // Armor - blue
        case 2:
            return engine::Color(255, 200, 0); // Accessory - gold
        case 3:
            return engine::Color(180, 80, 255); // Relic - purple
        }
        return engine::Color::white();
    }
} // namespace

void StatusScene::handleInput(engine::IInput &input)
{
    if (input.wasKeyJustPressed(engine::Key::Escape))
    {
        GameManager::instance().enterScene(SceneType::Town);
        return;
    }

    // Switch section with Left/Right or A/D.
    bool goLeft = input.wasKeyJustPressed(engine::Key::Left) || input.wasKeyJustPressed(engine::Key::A);
    bool goRight = input.wasKeyJustPressed(engine::Key::Right) || input.wasKeyJustPressed(engine::Key::D);

    if (goLeft)
    {
        if (section_ == Section::Gear)
            section_ = Section::Stats;
        else if (section_ == Section::Backpack)
            section_ = Section::Gear;
        return;
    }
    if (goRight)
    {
        if (section_ == Section::Stats)
            section_ = Section::Gear;
        else if (section_ == Section::Gear)
            section_ = Section::Backpack;
        return;
    }

    switch (section_)
    {
    case Section::Stats:
        handleStatsInput(input);
        break;
    case Section::Gear:
        handleGearInput(input);
        break;
    case Section::Backpack:
        handleBackpackInput(input);
        break;
    }
}

void StatusScene::handleStatsInput(engine::IInput &input)
{
    (void)input;
}

void StatusScene::handleGearInput(engine::IInput &input)
{
    if (input.wasKeyJustPressed(engine::Key::Up) || input.wasKeyJustPressed(engine::Key::W))
        gearSlotIndex_ = (gearSlotIndex_ - 1 + 4) % 4;
    if (input.wasKeyJustPressed(engine::Key::Down) || input.wasKeyJustPressed(engine::Key::S))
        gearSlotIndex_ = (gearSlotIndex_ + 1) % 4;

    if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
    {
        auto &gm = GameManager::instance();
        auto slot = static_cast<EquipmentSlot>(gearSlotIndex_ + 1);
        std::shared_ptr<EquipmentItem> item;
        switch (slot)
        {
        case EquipmentSlot::Weapon:
            item = gm.equippedGear().weapon;
            break;
        case EquipmentSlot::Armor:
            item = gm.equippedGear().armor;
            break;
        case EquipmentSlot::Accessory:
            item = gm.equippedGear().accessory;
            break;
        case EquipmentSlot::Relic:
            item = gm.equippedGear().relic;
            break;
        default:
            break;
        }
        if (item)
        {
            std::string itemName = item->name();
            if (gm.unequipToInventory(slot))
                message_ = "Unequipped: " + itemName;
            else
                message_ = "Backpack full. Cannot unequip.";
            messageTimer_ = 2.0f;
        }
        else
        {
            message_ = "No item to unequip.";
            messageTimer_ = 2.0f;
        }
    }
}

void StatusScene::handleBackpackInput(engine::IInput &input)
{
    const auto &items = GameManager::instance().inventory().items();
    int count = static_cast<int>(items.size());

    if (count == 0)
        return;

    if (input.wasKeyJustPressed(engine::Key::Up) || input.wasKeyJustPressed(engine::Key::W))
    {
        if (backpackIndex_ > 0)
            --backpackIndex_;
    }
    if (input.wasKeyJustPressed(engine::Key::Down) || input.wasKeyJustPressed(engine::Key::S))
    {
        if (backpackIndex_ < count - 1)
            ++backpackIndex_;
    }

    if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
    {
        if (backpackIndex_ >= 0 && backpackIndex_ < count && items[backpackIndex_])
        {
            Item *item = items[backpackIndex_].get();
            if (dynamic_cast<EquipmentItem *>(item))
            {
                std::string itemName = item->name();
                if (GameManager::instance().equipFromInventory(static_cast<size_t>(backpackIndex_)))
                {
                    message_ = "Equipped: " + itemName;
                    if (backpackIndex_ >= static_cast<int>(GameManager::instance().inventory().size()))
                        backpackIndex_ = std::max(0, static_cast<int>(GameManager::instance().inventory().size()) - 1);
                }
                else
                {
                    message_ = "Cannot equip item.";
                }
                messageTimer_ = 2.0f;
            }
            else
            {
                message_ = "Select equipment to equip here.";
                messageTimer_ = 2.0f;
            }
        }
    }
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

    renderStats(renderer);
    renderGear(renderer);
    renderBackpack(renderer);

    // Section tabs.
    engine::Color statsColor = (section_ == Section::Stats) ? engine::Color::yellow() : engine::Color::gray();
    engine::Color gearColor = (section_ == Section::Gear) ? engine::Color::yellow() : engine::Color::gray();
    engine::Color bpColor = (section_ == Section::Backpack) ? engine::Color::yellow() : engine::Color::gray();
    renderer.drawText("Stats", {60, 570}, 18, statsColor);
    renderer.drawText("Gear", {360, 570}, 18, gearColor);
    renderer.drawText("Backpack", {660, 570}, 18, bpColor);
    renderer.drawText("Esc: back", {720, 570}, 14, engine::Color::gray());

    // Feedback.
    if (!message_.empty())
    {
        renderer.drawRect({250, 280, 300, 36}, engine::Color(0, 0, 0, 200));
        renderer.drawText(message_, {260, 291}, 18, engine::Color::green());
    }
}

void StatusScene::renderStats(engine::IRenderer &renderer)
{
    float x = 20.0f, y = 20.0f, w = 260.0f, h = 530.0f;
    engine::Color border = (section_ == Section::Stats) ? engine::Color::yellow() : engine::Color(80, 80, 100);
    renderer.drawRect({x, y, w, h}, engine::Color(25, 25, 50, 230));
    renderer.drawRect({x, y, w, 2}, border);
    renderer.drawRect({x, y + h - 2, w, 2}, border);
    renderer.drawRect({x, y, 2, h}, border);
    renderer.drawRect({x + w - 2, y, 2, h}, border);

    renderer.drawText("Status", {x + 10, y + 10}, 20, engine::Color::white());

    const auto &character = GameManager::instance().character();
    const Persona *persona = character.currentPersona();
    float textY = y + 45;
    int step = 26;
    auto white = engine::Color::white();

    // ---- Character vitals ----
    renderer.drawText("Name: " + character.name(), {x + 10, textY}, 16, white);
    textY += step;
    renderer.drawText("Level: " + std::to_string(character.level()), {x + 10, textY}, 16, white);
    textY += step;
    renderer.drawText("HP:  " + std::to_string(character.hp()) + "/" + std::to_string(character.maxHp()), {x + 10, textY}, 16, engine::Color(100, 255, 100));
    textY += step;
    renderer.drawText("SP:  " + std::to_string(character.sp()) + "/" + std::to_string(character.maxSp()), {x + 10, textY}, 16, engine::Color(100, 200, 255));
    textY += step;
    renderer.drawText("Gold: " + std::to_string(character.gold()), {x + 10, textY}, 16, engine::Color::yellow());
    textY += step + 10;

    // ---- Current Persona (the real source of combat stats) ----
    renderer.drawRect({x + 8, textY - 4, w - 16, 2}, engine::Color(120, 130, 180, 200));
    textY += 10;
    renderer.drawText("Current Persona", {x + 10, textY}, 18, engine::Color::cyan());
    textY += step;

    if (persona)
    {
        renderer.drawText(persona->name() + " (" + persona->arcana() + ")", {x + 10, textY}, 16, white);
        textY += step;
        renderer.drawText("Lv: " + std::to_string(persona->level()) +
                              "  Exp: " + std::to_string(persona->exp()) + "/" + std::to_string(persona->expToNextLevel()),
                          {x + 10, textY}, 14, engine::Color(200, 200, 200));
        textY += step;

        int baseStr = persona->strength();
        int baseMag = persona->magic();
        int baseSpd = persona->speed();
        int eqStr = character.equipmentStrengthBonus();
        int eqMag = character.equipmentMagicBonus();
        int eqSpd = character.equipmentSpeedBonus();
        int finalStr = character.attack();
        int finalMag = character.magic();
        int finalSpd = character.speed();

        renderer.drawText("Base STR/MAG/SPD", {x + 10, textY}, 14, engine::Color(180, 180, 180));
        textY += step - 4;
        renderer.drawText(std::to_string(baseStr) + " / " + std::to_string(baseMag) + " / " + std::to_string(baseSpd),
                          {x + 10, textY}, 16, white);
        textY += step;

        renderer.drawText("Equip +STR/+MAG/+SPD", {x + 10, textY}, 14, engine::Color(180, 180, 180));
        textY += step - 4;
        renderer.drawText("+" + std::to_string(eqStr) + " / +" + std::to_string(eqMag) + " / +" + std::to_string(eqSpd),
                          {x + 10, textY}, 16, engine::Color(255, 220, 100));
        textY += step;

        renderer.drawText("Final STR/MAG/SPD", {x + 10, textY}, 14, engine::Color(180, 180, 180));
        textY += step - 4;
        renderer.drawText(std::to_string(finalStr) + " / " + std::to_string(finalMag) + " / " + std::to_string(finalSpd),
                          {x + 10, textY}, 16, engine::Color(100, 255, 100));
        textY += step + 4;

        // Skills
        renderer.drawText("Skills:", {x + 10, textY}, 16, engine::Color::cyan());
        textY += step - 4;
        const auto &skills = persona->skills();
        if (skills.empty())
        {
            renderer.drawText("(none)", {x + 10, textY}, 14, engine::Color(100, 100, 100));
        }
        else
        {
            for (const auto &skill : skills)
            {
                if (!skill)
                    continue;
                std::string line = skill->name();
                if (skill->cost() > 0)
                    line += " (" + std::to_string(skill->cost()) + (skill->costType() == SkillCostType::HP ? " HP" : " SP") + ")";
                renderer.drawText("- " + line, {x + 10, textY}, 13, engine::Color(200, 200, 200));
                textY += 20;
                if (textY > y + h - 20)
                    break;
            }
        }
    }
    else
    {
        renderer.drawText("(no Persona equipped)", {x + 10, textY}, 14, engine::Color(100, 100, 100));
    }
}

void StatusScene::renderGear(engine::IRenderer &renderer)
{
    float x = 300.0f, y = 20.0f, w = 260.0f, h = 530.0f;
    engine::Color border = (section_ == Section::Gear) ? engine::Color::yellow() : engine::Color(80, 80, 100);
    renderer.drawRect({x, y, w, h}, engine::Color(25, 25, 50, 230));
    renderer.drawRect({x, y, w, 2}, border);
    renderer.drawRect({x, y + h - 2, w, 2}, border);
    renderer.drawRect({x, y, 2, h}, border);
    renderer.drawRect({x + w - 2, y, 2, h}, border);

    renderer.drawText("Equipped Gear", {x + 10, y + 10}, 20, engine::Color::white());

    const auto &gear = GameManager::instance().equippedGear();
    float slotH = 100.0f;
    float startY = y + 50;

    for (int i = 0; i < 4; ++i)
    {
        float sy = startY + i * (slotH + 10);
        bool selected = (section_ == Section::Gear && i == gearSlotIndex_);
        engine::Color boxBorder = selected ? engine::Color::yellow() : engine::Color(80, 80, 100);
        renderer.drawRect({x + 10, sy, w - 20, slotH}, engine::Color(20, 20, 40, 230));
        renderer.drawRect({x + 10, sy, w - 20, 2}, boxBorder);
        renderer.drawRect({x + 10, sy + slotH - 2, w - 20, 2}, boxBorder);
        renderer.drawRect({x + 10, sy, 2, slotH}, boxBorder);
        renderer.drawRect({x + 10 + w - 22, sy, 2, slotH}, boxBorder);

        renderer.drawText(slotName(i), {x + 20, sy + 6}, 14, slotColor(i));

        std::shared_ptr<EquipmentItem> item;
        switch (i)
        {
        case 0:
            item = gear.weapon;
            break;
        case 1:
            item = gear.armor;
            break;
        case 2:
            item = gear.accessory;
            break;
        case 3:
            item = gear.relic;
            break;
        }

        if (item)
        {
            renderer.drawText(item->name(), {x + 20, sy + 28}, 16, engine::Color::white());
            std::string stats;
            if (item->strengthBonus() > 0)
                stats += "STR+" + std::to_string(item->strengthBonus()) + " ";
            if (item->magicBonus() > 0)
                stats += "MAG+" + std::to_string(item->magicBonus()) + " ";
            if (item->speedBonus() > 0)
                stats += "SPD+" + std::to_string(item->speedBonus()) + " ";
            renderer.drawText(stats, {x + 20, sy + 52}, 12, engine::Color::gray());
        }
        else
        {
            renderer.drawText("(empty)", {x + 20, sy + 40}, 14, engine::Color(100, 100, 100));
        }
    }
}

void StatusScene::renderBackpack(engine::IRenderer &renderer)
{
    float x = 580.0f, y = 20.0f, w = 210.0f, h = 530.0f;
    engine::Color border = (section_ == Section::Backpack) ? engine::Color::yellow() : engine::Color(80, 80, 100);
    renderer.drawRect({x, y, w, h}, engine::Color(25, 25, 50, 230));
    renderer.drawRect({x, y, w, 2}, border);
    renderer.drawRect({x, y + h - 2, w, 2}, border);
    renderer.drawRect({x, y, 2, h}, border);
    renderer.drawRect({x + w - 2, y, 2, h}, border);

    renderer.drawText("Backpack", {x + 10, y + 10}, 20, engine::Color::white());

    const auto &items = GameManager::instance().inventory().items();
    if (items.empty())
    {
        renderer.drawText("(empty)", {x + 10, y + 50}, 16, engine::Color(100, 100, 100));
        return;
    }

    float rowH = 28.0f;
    float visibleH = h - 50;
    int maxVisible = static_cast<int>(visibleH / rowH);
    int start = std::max(0, backpackIndex_ - maxVisible / 2);
    if (start + maxVisible > static_cast<int>(items.size()))
        start = std::max(0, static_cast<int>(items.size()) - maxVisible);
    int end = std::min(static_cast<int>(items.size()), start + maxVisible);

    for (int i = start; i < end; ++i)
    {
        if (!items[i])
            continue;
        float rowY = y + 45 + (i - start) * rowH;
        bool selected = (section_ == Section::Backpack && i == backpackIndex_);
        engine::Color color = selected ? engine::Color::yellow() : engine::Color::white();
        std::string prefix = selected ? "> " : "  ";
        renderer.drawText(prefix + items[i]->name(), {x + 8, rowY}, 13, color);
    }
}
