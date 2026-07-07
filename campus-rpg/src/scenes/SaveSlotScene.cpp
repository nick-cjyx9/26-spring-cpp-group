#include "SaveSlotScene.h"
#include "GameManager.h"
#include "SaveRepository.h"

#include <algorithm>

namespace
{
    constexpr int kSlotCount = 3;
} // namespace

SaveSlotScene::SaveSlotScene(Mode mode) : mode_(mode)
{
    refresh();
}

void SaveSlotScene::refresh()
{
    slots_ = GameManager::instance().listSaveSlots(kSlotCount);
    if (slots_.size() < static_cast<size_t>(kSlotCount))
        slots_.resize(static_cast<size_t>(kSlotCount));
    if (selectedIndex_ < 0)
        selectedIndex_ = 0;
    if (selectedIndex_ >= kSlotCount)
        selectedIndex_ = kSlotCount - 1;
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
    if (input.wasKeyJustPressed(engine::Key::Up))
        selectedIndex_ = (selectedIndex_ - 1 + kSlotCount) % kSlotCount;
    if (input.wasKeyJustPressed(engine::Key::Down))
        selectedIndex_ = (selectedIndex_ + 1) % kSlotCount;

    bool confirm = input.wasKeyJustPressed(engine::Key::Enter) ||
                   input.wasKeyJustPressed(engine::Key::E);

    int slotId = selectedIndex_ + 1;
    bool filled = static_cast<size_t>(selectedIndex_) < slots_.size() &&
                  slots_[selectedIndex_].exists;

    if (confirm)
    {
        if (mode_ == Mode::Save)
        {
            if (GameManager::instance().saveToSlot(slotId))
            {
                message_ = "Saved to slot " + std::to_string(slotId) + "!";
                refresh();
            }
            else
            {
                message_ = "Save failed. Please try again.";
            }
        }
        else // Load mode
        {
            if (filled)
            {
                if (GameManager::instance().loadFromSlot(slotId))
                {
                    GameManager::instance().enterScene(SceneType::Town);
                    return;
                }
                message_ = "Load failed. Save may be corrupted.";
            }
            else
            {
                // Empty slot -> create a new save: prompt for character id.
                subState_ = SubState::Naming;
                nameBuffer_.clear();
                message_ = "Enter character id, then press Enter.";
            }
        }
    }

    // Backspace (delivered via the text stream) deletes a filled slot.
    std::string typed = input.consumeTypedText();
    for (char ch : typed)
    {
        if (ch == '\b')
        {
            if (filled)
            {
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
    }

    if (input.wasKeyJustPressed(engine::Key::Escape))
    {
        message_.clear();
        if (mode_ == Mode::Save)
            GameManager::instance().enterScene(SceneType::Town);
        else
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
        int slotId = selectedIndex_ + 1;
        if (GameManager::instance().createNewSave(slotId, nameBuffer_))
        {
            GameManager::instance().enterScene(SceneType::Town);
            return;
        }
        message_ = "Failed to create save. Please try again.";
    }

    if (input.wasKeyJustPressed(engine::Key::Escape))
    {
        subState_ = SubState::Browsing;
        nameBuffer_.clear();
        message_.clear();
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

    // Dark background.
    renderer.drawRect({0, 0, 800, 600}, engine::Color(20, 20, 35));

    const char *title = (mode_ == Mode::Save) ? "Save Game" : "Load Game";
    renderer.drawText(title, {300, 40}, 36, engine::Color::white());

    // Slot list: 3 rectangles stacked vertically, each shows name + time.
    const float slotW = 600.0f;
    const float slotH = 90.0f;
    const float slotX = 100.0f;
    const float slotY0 = 120.0f;
    const float gap = 20.0f;

    for (int i = 0; i < kSlotCount; ++i)
    {
        float y = slotY0 + i * (slotH + gap);
        bool selected = (i == selectedIndex_);
        bool filled = static_cast<size_t>(i) < slots_.size() && slots_[i].exists;

        engine::Color border = selected ? engine::Color::yellow() : engine::Color(80, 80, 100);
        engine::Color fillBg = filled ? engine::Color(35, 35, 55, 230)
                                      : engine::Color(25, 25, 35, 230);

        renderer.drawRect({slotX, y, slotW, slotH}, fillBg);
        // Border (drawn as 4 thin rects).
        renderer.drawRect({slotX, y, slotW, 3}, border);
        renderer.drawRect({slotX, y + slotH - 3, slotW, 3}, border);
        renderer.drawRect({slotX, y, 3, slotH}, border);
        renderer.drawRect({slotX + slotW - 3, y, 3, slotH}, border);

        std::string prefix = selected ? "> " : "  ";
        std::string slotLabel = prefix + "Slot " + std::to_string(i + 1);
        renderer.drawText(slotLabel, {slotX + 15, y + 12}, 20,
                          selected ? engine::Color::yellow() : engine::Color::white());

        if (filled)
        {
            renderer.drawText(slots_[i].characterName, {slotX + 15, y + 40}, 22, engine::Color::white());
            renderer.drawText("Lv." + std::to_string(slots_[i].level) + "    " + slots_[i].updatedAt,
                              {slotX + 15, y + 66}, 16, engine::Color::gray());
        }
        else
        {
            renderer.drawText("Empty  -  No save data", {slotX + 15, y + 48}, 18, engine::Color::gray());
        }
    }

    // Feedback message.
    if (!message_.empty())
        renderer.drawText(message_, {100, 510}, 18, engine::Color::cyan());

    // Hints.
    std::string hints;
    if (mode_ == Mode::Save)
        hints = "Up/Down: select   Enter: save   Backspace: delete   Esc: back";
    else
        hints = "Up/Down: select   Enter: load / create new   Backspace: delete   Esc: back";
    renderer.drawText(hints, {100, 560}, 16, engine::Color::gray());
}

void SaveSlotScene::renderNaming(engine::IRenderer &renderer)
{
    renderer.clear();
    renderer.drawRect({0, 0, 800, 600}, engine::Color(20, 20, 35));

    renderer.drawText("Create New Save", {260, 80}, 36, engine::Color::white());
    renderer.drawText("Slot " + std::to_string(selectedIndex_ + 1) + " is empty. Enter a character id:",
                      {180, 160}, 22, engine::Color::white());

    // Input box.
    renderer.drawRect({200, 210, 400, 60}, engine::Color(35, 35, 55, 230));
    renderer.drawRect({200, 210, 400, 3}, engine::Color::yellow());
    renderer.drawRect({200, 267, 400, 3}, engine::Color::yellow());
    renderer.drawRect({200, 210, 3, 60}, engine::Color::yellow());
    renderer.drawRect({597, 210, 3, 60}, engine::Color::yellow());

    std::string display = nameBuffer_;
    if (display.empty())
        display = "_";
    renderer.drawText(display, {215, 226}, 26, engine::Color::white());

    if (!message_.empty())
        renderer.drawText(message_, {180, 300}, 18, engine::Color::cyan());

    renderer.drawText("Enter: confirm   Esc: cancel   Backspace: erase",
                      {180, 350}, 16, engine::Color::gray());
}
