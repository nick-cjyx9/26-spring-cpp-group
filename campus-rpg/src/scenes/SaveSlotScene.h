#pragma once

#include "IScene.h"
#include "SaveRepository.h"

#include <string>
#include <vector>

// Save slot management screen. Reached from the Title (load/create) or from
// the Town (save). Displays each slot's character name + last-updated time,
// and supports creating a new save (with name entry), loading, saving and
// deleting.
class SaveSlotScene : public engine::IScene
{
public:
    enum class Mode
    {
        Load, // from Title: browse / load / delete / create-new
        Save  // from Town: save current game to a slot / delete
    };

    explicit SaveSlotScene(Mode mode = Mode::Load);

    void handleInput(engine::IInput &input) override;
    void update(float deltaTime) override;
    void render(engine::IRenderer &renderer) override;

private:
    enum class SubState
    {
        Browsing,
        Naming // entering a character id for a new save
    };

    void refresh();
    void handleBrowsingInput(engine::IInput &input);
    void handleNamingInput(engine::IInput &input);
    void renderBrowsing(engine::IRenderer &renderer);
    void renderNaming(engine::IRenderer &renderer);

    Mode mode_;
    SubState subState_ = SubState::Browsing;
    int selectedIndex_ = 0;
    std::string nameBuffer_;
    std::string message_;
    // Cached slot metadata; refreshed on entry and after mutations.
    std::vector<SaveSlotInfo> slots_;
};
