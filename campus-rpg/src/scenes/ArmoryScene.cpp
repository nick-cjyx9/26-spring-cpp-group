#include "ArmoryScene.h"
#include "GameManager.h"
#include "Character.h"
#include "Item.h"
#include "Inventory.h"

namespace
{
    const char *categoryName(int idx)
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

    engine::Color categoryColor(int idx)
    {
        switch (idx)
        {
        case 0:
            return engine::Color(255, 80, 80);
        case 1:
            return engine::Color(80, 80, 255);
        case 2:
            return engine::Color(255, 200, 0);
        case 3:
            return engine::Color(180, 80, 255);
        }
        return engine::Color::white();
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

    std::vector<std::shared_ptr<EquipmentItem>> filterBySlot(
        const std::vector<std::shared_ptr<EquipmentItem>> &db, EquipmentSlot slot)
    {
        std::vector<std::shared_ptr<EquipmentItem>> result;
        for (const auto &item : db)
        {
            if (item && item->slot() == slot)
                result.push_back(item);
        }
        return result;
    }

    int equipmentPrice(const EquipmentItem &item)
    {
        return (item.strengthBonus() + item.magicBonus() + item.speedBonus()) * 10;
    }

    void drawStatsLine(engine::IRenderer &r, const std::shared_ptr<EquipmentItem> &item,
                       float x, float y, int size, engine::Color color)
    {
        if (!item)
            return;
        std::string stats;
        if (item->strengthBonus() > 0)
            stats += "STR+" + std::to_string(item->strengthBonus()) + " ";
        if (item->magicBonus() > 0)
            stats += "MAG+" + std::to_string(item->magicBonus()) + " ";
        if (item->speedBonus() > 0)
            stats += "SPD+" + std::to_string(item->speedBonus()) + " ";
        if (!stats.empty())
            r.drawText(stats, {x, y}, size, color);
    }
} // namespace

void ArmoryScene::handleInput(engine::IInput &input)
{
    if (input.wasKeyJustPressed(engine::Key::Escape))
    {
        GameManager::instance().enterScene(SceneType::Town);
        return;
    }

    if (focus_ == Focus::LeftPanel)
        handleLeftPanelInput(input);
    else
        handleRightPanelInput(input);
}

void ArmoryScene::handleLeftPanelInput(engine::IInput &input)
{
    if (input.wasKeyJustPressed(engine::Key::Up) || input.wasKeyJustPressed(engine::Key::W))
        categoryIndex_ = (categoryIndex_ - 1 + 4) % 4;
    if (input.wasKeyJustPressed(engine::Key::Down) || input.wasKeyJustPressed(engine::Key::S))
        categoryIndex_ = (categoryIndex_ + 1) % 4;

    if (input.wasKeyJustPressed(engine::Key::Right) || input.wasKeyJustPressed(engine::Key::D))
    {
        focus_ = Focus::RightPanel;
        selectedIndex_ = 0;
        gridScrollOffset_ = 0;
    }
}

void ArmoryScene::handleRightPanelInput(engine::IInput &input)
{
    auto slot = indexToSlot(categoryIndex_);
    auto items = filterBySlot(GameManager::instance().equipmentDatabase(), slot);
    int count = static_cast<int>(items.size());
    if (count == 0)
        return;

    int cols = 3;
    int rows = (count + cols - 1) / cols;
    int row = selectedIndex_ / cols;
    int col = selectedIndex_ % cols;

    if (input.wasKeyJustPressed(engine::Key::Up) || input.wasKeyJustPressed(engine::Key::W))
    {
        if (row > 0)
            selectedIndex_ -= cols;
        else
            selectedIndex_ = std::max(0, selectedIndex_);
    }
    if (input.wasKeyJustPressed(engine::Key::Down) || input.wasKeyJustPressed(engine::Key::S))
    {
        if (row + 1 < rows)
        {
            int newIdx = selectedIndex_ + cols;
            if (newIdx < count)
                selectedIndex_ = newIdx;
        }
    }
    if (input.wasKeyJustPressed(engine::Key::Left) || input.wasKeyJustPressed(engine::Key::A))
    {
        if (col > 0)
            --selectedIndex_;
        else
            focus_ = Focus::LeftPanel;
    }
    if (input.wasKeyJustPressed(engine::Key::Right) || input.wasKeyJustPressed(engine::Key::D))
    {
        if (col + 1 < cols && selectedIndex_ + 1 < count)
            ++selectedIndex_;
    }

    if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
    {
        if (selectedIndex_ >= 0 && selectedIndex_ < count && items[selectedIndex_])
        {
            auto &gm = GameManager::instance();
            const auto &item = items[selectedIndex_];
            int price = equipmentPrice(*item);
            if (gm.character().gold() < price)
            {
                message_ = "Not enough gold.";
            }
            else if (gm.inventory().isFull())
            {
                message_ = "Backpack is full.";
            }
            else if (gm.inventory().addItem(item->clone()))
            {
                gm.character().spendGold(price);
                message_ = "Purchased: " + item->name();
            }
            else
            {
                message_ = "Cannot buy item.";
            }
            messageTimer_ = 2.0f;
        }
    }
}

void ArmoryScene::update(float deltaTime)
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

void ArmoryScene::render(engine::IRenderer &renderer)
{
    renderer.clear();
    renderer.drawRect({0, 0, 800, 600}, engine::Color(15, 15, 30));

    renderLeftPanel(renderer);
    renderRightPanel(renderer);

    renderer.drawText("Weapon Shop   Gold: " + std::to_string(GameManager::instance().character().gold()),
                      {310, 8}, 18, engine::Color::yellow());

    // Hint.
    renderer.drawText("W/S: category   A/D: switch panel   Enter/E: buy equipment   Esc: back",
                      {100, 570}, 14, engine::Color::gray());

    // Feedback.
    if (!message_.empty())
    {
        renderer.drawRect({250, 280, 300, 36}, engine::Color(0, 0, 0, 200));
        renderer.drawText(message_, {260, 291}, 18, engine::Color::green());
    }
}

void ArmoryScene::renderLeftPanel(engine::IRenderer &renderer)
{
    float x = 20.0f, y = 20.0f, w = 180.0f, h = 530.0f;
    engine::Color border = (focus_ == Focus::LeftPanel) ? engine::Color::yellow() : engine::Color(80, 80, 100);
    renderer.drawRect({x, y, w, h}, engine::Color(25, 25, 50, 230));
    renderer.drawRect({x, y, w, 2}, border);
    renderer.drawRect({x, y + h - 2, w, 2}, border);
    renderer.drawRect({x, y, 2, h}, border);
    renderer.drawRect({x + w - 2, y, 2, h}, border);

    renderer.drawText("Weapon Shop", {x + 10, y + 10}, 18, engine::Color::white());

    float catH = 110.0f;
    for (int i = 0; i < 4; ++i)
    {
        float cy = y + 50 + i * (catH + 10);
        bool selected = (focus_ == Focus::LeftPanel && i == categoryIndex_);
        engine::Color boxColor = selected ? engine::Color::yellow() : engine::Color(80, 80, 100);
        renderer.drawRect({x + 10, cy, w - 20, catH}, engine::Color(20, 20, 40, 230));
        renderer.drawRect({x + 10, cy, w - 20, 2}, boxColor);
        renderer.drawRect({x + 10, cy + catH - 2, w - 20, 2}, boxColor);
        renderer.drawRect({x + 10, cy, 2, catH}, boxColor);
        renderer.drawRect({x + 10 + w - 22, cy, 2, catH}, boxColor);

        renderer.drawText(categoryName(i), {x + 20, cy + 42}, 18,
                          selected ? engine::Color::yellow() : categoryColor(i));
    }
}

void ArmoryScene::renderRightPanel(engine::IRenderer &renderer)
{
    float x = 220.0f, y = 20.0f, w = 560.0f, h = 530.0f;
    engine::Color border = (focus_ == Focus::RightPanel) ? engine::Color::yellow() : engine::Color(80, 80, 100);
    renderer.drawRect({x, y, w, h}, engine::Color(25, 25, 50, 230));
    renderer.drawRect({x, y, w, 2}, border);
    renderer.drawRect({x, y + h - 2, w, 2}, border);
    renderer.drawRect({x, y, 2, h}, border);
    renderer.drawRect({x + w - 2, y, 2, h}, border);

    // Detail preview at top.
    renderItemDetail(renderer);

    // Grid below.
    renderItemGrid(renderer);
}

void ArmoryScene::renderItemDetail(engine::IRenderer &renderer)
{
    float x = 240.0f, y = 40.0f;
    auto slot = indexToSlot(categoryIndex_);
    auto items = filterBySlot(GameManager::instance().equipmentDatabase(), slot);

    renderer.drawText("Buy Equipment", {x, y}, 20, engine::Color::white());

    if (!items.empty() && selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items.size()) && items[selectedIndex_])
    {
        const auto &item = items[selectedIndex_];
        renderer.drawText(item->name() + "  " + std::to_string(equipmentPrice(*item)) + "G", {x + 150, y}, 20, engine::Color::yellow());
        drawStatsLine(renderer, item, x + 150, y + 25, 14, engine::Color(200, 200, 200));
    }
    else
    {
        renderer.drawText("(select an item)", {x + 150, y}, 16, engine::Color(100, 100, 100));
    }
}

void ArmoryScene::renderItemGrid(engine::IRenderer &renderer)
{
    float x = 240.0f, y = 90.0f;
    auto slot = indexToSlot(categoryIndex_);
    auto items = filterBySlot(GameManager::instance().equipmentDatabase(), slot);
    int count = static_cast<int>(items.size());
    if (count == 0)
    {
        renderer.drawText("No items in this category.", {x, y + 20}, 16, engine::Color(100, 100, 100));
        return;
    }

    float cellW = 160.0f, cellH = 130.0f;
    float gapX = 15.0f, gapY = 15.0f;
    int cols = 3;
    int rows = (count + cols - 1) / cols;

    // Scrolling: keep the selected row in view.
    float panelH = 530.0f - 90.0f; // visible area height
    int visibleRows = std::max(1, static_cast<int>(panelH / (cellH + gapY)));

    int selectedRow = selectedIndex_ / cols;
    if (selectedRow < gridScrollOffset_)
        gridScrollOffset_ = selectedRow;
    if (selectedRow >= gridScrollOffset_ + visibleRows)
        gridScrollOffset_ = selectedRow - visibleRows + 1;
    // Clamp scroll.
    if (gridScrollOffset_ < 0)
        gridScrollOffset_ = 0;
    if (gridScrollOffset_ > rows - visibleRows)
        gridScrollOffset_ = std::max(0, rows - visibleRows);

    int startIdx = gridScrollOffset_ * cols;
    int endIdx = std::min(count, (gridScrollOffset_ + visibleRows) * cols);

    for (int i = startIdx; i < endIdx; ++i)
    {
        if (!items[i])
            continue;
        int col = i % cols;
        int row = i / cols - gridScrollOffset_; // relative row for rendering
        float cx = x + col * (cellW + gapX);
        float cy = y + row * (cellH + gapY);
        bool selected = (focus_ == Focus::RightPanel && i == selectedIndex_);

        engine::Color boxBorder = selected ? engine::Color::yellow() : engine::Color(80, 80, 100);
        renderer.drawRect({cx, cy, cellW, cellH}, engine::Color(20, 20, 40, 230));
        renderer.drawRect({cx, cy, cellW, 2}, boxBorder);
        renderer.drawRect({cx, cy + cellH - 2, cellW, 2}, boxBorder);
        renderer.drawRect({cx, cy, 2, cellH}, boxBorder);
        renderer.drawRect({cx + cellW - 2, cy, 2, cellH}, boxBorder);

        // Draw item icon using the equipment texture.
        std::string texPath = std::string("equipment/") + items[i]->textureId();
        renderer.drawTexture(texPath, {cx + (cellW - 48) / 2, cy + 8, 48, 48});

        // Name / price.
        renderer.drawText(items[i]->name(), {cx + 8, cy + 62}, 13, engine::Color::white());
        renderer.drawText(std::to_string(equipmentPrice(*items[i])) + "G", {cx + 8, cy + 78}, 12, engine::Color::yellow());

        // Stats.
        drawStatsLine(renderer, items[i], cx + 8, cy + 96, 11, engine::Color::gray());
    }

    // Scrollbar indicator on the right edge.
    if (rows > visibleRows)
    {
        float barX = x + cols * (cellW + gapX) + 5.0f;
        float barY = y;
        float barH = visibleRows * (cellH + gapY) - gapY;
        float thumbH = barH * static_cast<float>(visibleRows) / static_cast<float>(rows);
        float thumbY = barY + static_cast<float>(gridScrollOffset_) / static_cast<float>(rows) * barH;
        renderer.drawRect({barX, barY, 4, barH}, engine::Color(60, 60, 80));
        renderer.drawRect({barX, thumbY, 4, thumbH}, engine::Color(200, 200, 200));
    }
}
