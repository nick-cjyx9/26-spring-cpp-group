#include "SaveSlotScene.h"
#include "GameManager.h"

namespace
{
    constexpr int kVisibleRows = 7;
    constexpr float kRowH = 60.0f;
    constexpr float kRowGap = 10.0f;
    constexpr float kListX = 80.0f;
    constexpr float kListW = 640.0f;
    constexpr float kListY0 = 110.0f;

    void drawBorder(engine::IRenderer &renderer, const engine::Rect &r, engine::Color color)
    {
        renderer.drawRect({r.x, r.y, r.width, 3}, color);
        renderer.drawRect({r.x, r.y + r.height - 3, r.width, 3}, color);
        renderer.drawRect({r.x, r.y, 3, r.height}, color);
        renderer.drawRect({r.x + r.width - 3, r.y, 3, r.height}, color);
    }
} // namespace

SaveSlotScene::SaveSlotScene(Mode mode) : mode_(mode)
{
    if (mode_ == Mode::Create)
    {
        // Legacy naming flow (kept for compatibility; the primary new-game
        // entry is now HeroSelectScene, reached directly from TitleScene).
        subState_ = SubState::Naming;
        nameBuffer_.clear();
        message_ = "Enter character id, then press Enter.";
    }
    else
    {
        refresh();
    }
}

void SaveSlotScene::refresh()
{
    slots_ = GameManager::instance().listAllSaves();
    if (selectedIndex_ < 0)
        selectedIndex_ = 0;
    if (!slots_.empty() && selectedIndex_ >= static_cast<int>(slots_.size()))
        selectedIndex_ = static_cast<int>(slots_.size()) - 1;
    // Adjust scroll so the selected row is visible.
    if (selectedIndex_ < scrollOffset_)
        scrollOffset_ = selectedIndex_;
    int lastVisible = scrollOffset_ + kVisibleRows - 1;
    if (selectedIndex_ > lastVisible)
        scrollOffset_ = selectedIndex_ - kVisibleRows + 1;
    if (scrollOffset_ < 0)
        scrollOffset_ = 0;
}

void SaveSlotScene::handleInput(engine::IInput &input)
{
    if (subState_ == SubState::Naming)
        handleNamingInput(input);
    else
        handleBrowsingInput(input);
}

void SaveSlotScene::handleBrowsingInput(engine::IInput &input)
{
    if (slots_.empty())
    {
        // Nothing to browse; only Esc (and typing) matters.
        std::string typed = input.consumeTypedText();
        (void)typed;
        if (input.wasKeyJustPressed(engine::Key::Escape))
            GameManager::instance().enterScene(SceneType::Title);
        return;
    }

    if (input.wasKeyJustPressed(engine::Key::Up))
        selectedIndex_ = (selectedIndex_ - 1 + static_cast<int>(slots_.size())) % static_cast<int>(slots_.size());
    if (input.wasKeyJustPressed(engine::Key::Down))
        selectedIndex_ = (selectedIndex_ + 1) % static_cast<int>(slots_.size());

    bool confirm = input.wasKeyJustPressed(engine::Key::Enter) ||
                   input.wasKeyJustPressed(engine::Key::E);

    if (confirm)
    {
        int slotId = slots_[selectedIndex_].slotId;
        if (GameManager::instance().loadFromSlot(slotId))
        {
            GameManager::instance().enterScene(SceneType::Town);
            return;
        }
        message_ = "Load failed. Save may be corrupted.";
    }

    // Backspace (via the text stream) deletes the selected save.
    std::string typed = input.consumeTypedText();
    for (char ch : typed)
    {
        if (ch == '\b')
        {
            int slotId = slots_[selectedIndex_].slotId;
            if (GameManager::instance().deleteSaveSlot(slotId))
            {
                message_ = "Slot " + std::to_string(slotId) + " deleted.";
                refresh();
            }
            else
            {
                message_ = "Delete failed.";
            }
        }
    }

    if (input.wasKeyJustPressed(engine::Key::Escape))
    {
        GameManager::instance().enterScene(SceneType::Title);
    }
}

void SaveSlotScene::handleNamingInput(engine::IInput &input)
{
    std::string typed = input.consumeTypedText();
    for (char ch : typed)
    {
        if (ch == '\b')
        {
            if (!nameBuffer_.empty())
                nameBuffer_.pop_back();
        }
        else if (nameBuffer_.size() < 16)
        {
            nameBuffer_ += ch;
        }
    }

    if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
    {
        if (nameBuffer_.empty())
        {
            message_ = "Character id cannot be empty.";
            return;
        }
        int slotId = GameManager::instance().nextSaveSlotId();
        if (GameManager::instance().createNewSave(nameBuffer_))
        {
            // createNewSave auto-assigned slotId; switch to town.
            GameManager::instance().enterScene(SceneType::Town);
            return;
        }
        message_ = "Failed to create save (slot " + std::to_string(slotId) + ").";
    }

    if (input.wasKeyJustPressed(engine::Key::Escape))
    {
        // Cancel -> back to Title.
        GameManager::instance().enterScene(SceneType::Title);
    }
}

void SaveSlotScene::update(float /*deltaTime*/)
{
}

void SaveSlotScene::render(engine::IRenderer &renderer)
{
    if (subState_ == SubState::Naming)
        renderNaming(renderer);
    else
        renderBrowsing(renderer);
}

void SaveSlotScene::renderBrowsing(engine::IRenderer &renderer)
{
    renderer.clear();
    renderer.drawRect({0, 0, 800, 600}, engine::Color(20, 20, 35));

    renderer.drawText("Load Game", {320, 40}, 36, engine::Color::white());

    if (slots_.empty())
    {
        renderer.drawText("No save data", {330, 280}, 28, engine::Color::gray());
        renderer.drawText("Esc: back to title", {340, 330}, 16, engine::Color::gray());
        if (!message_.empty())
            renderer.drawText(message_, {260, 380}, 18, engine::Color::cyan());
        return;
    }

    int total = static_cast<int>(slots_.size());
    int visibleEnd = std::min(scrollOffset_ + kVisibleRows, total);

    for (int i = scrollOffset_; i < visibleEnd; ++i)
    {
        float y = kListY0 + (i - scrollOffset_) * (kRowH + kRowGap);
        bool selected = (i == selectedIndex_);
        const SaveSlotInfo &info = slots_[i];

        engine::Color border = selected ? engine::Color::yellow() : engine::Color(80, 80, 100);
        engine::Color fillBg = engine::Color(35, 35, 55, 230);

        engine::Rect row{kListX, y, kListW, kRowH};
        renderer.drawRect(row, fillBg);
        drawBorder(renderer, row, border);

        std::string prefix = selected ? "> " : "  ";
        renderer.drawText(prefix + "Slot " + std::to_string(info.slotId),
                          {kListX + 12, y + 8}, 18,
                          selected ? engine::Color::yellow() : engine::Color::white());
        renderer.drawText(info.characterName + "    Lv." + std::to_string(info.level),
                          {kListX + 12, y + 30}, 20, engine::Color::white());
        renderer.drawText(info.updatedAt, {kListX + 380, y + 32}, 16, engine::Color::gray());
    }

    // Scroll indicator.
    if (total > kVisibleRows)
    {
        renderer.drawText("v", {kListX + kListW + 8, kListY0 + (kVisibleRows - 1) * (kRowH + kRowGap) + 20},
                          18, engine::Color::gray());
        renderer.drawText("^", {kListX + kListW + 8, kListY0 + 20}, 18, engine::Color::gray());
    }

    if (!message_.empty())
        renderer.drawText(message_, {kListX, 540}, 18, engine::Color::cyan());

    renderer.drawText("Up/Down: select   Enter: load   Backspace: delete   Esc: back",
                      {kListX, 570}, 16, engine::Color::gray());
}

void SaveSlotScene::renderNaming(engine::IRenderer &renderer)
{
    renderer.clear();
    renderer.drawRect({0, 0, 800, 600}, engine::Color(20, 20, 35));

    renderer.drawText("Create New Save", {260, 80}, 36, engine::Color::white());

    int nextSlot = GameManager::instance().nextSaveSlotId();
    renderer.drawText("New save will be created in slot " + std::to_string(nextSlot),
                      {220, 150}, 22, engine::Color::white());
    renderer.drawText("Enter a character id:", {260, 195}, 22, engine::Color::white());

    // Input box.
    engine::Rect box{200, 240, 400, 60};
    renderer.drawRect(box, engine::Color(35, 35, 55, 230));
    drawBorder(renderer, box, engine::Color::yellow());

    std::string display = nameBuffer_;
    if (display.empty())
        display = "_";
    renderer.drawText(display, {215, 256}, 26, engine::Color::white());

    if (!message_.empty())
        renderer.drawText(message_, {180, 330}, 18, engine::Color::cyan());

    renderer.drawText("Enter: confirm   Esc: cancel   Backspace: erase",
                      {180, 380}, 16, engine::Color::gray());
}
