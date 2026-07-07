#pragma once

#include "IScene.h"
#include "SaveRepository.h"

#include <string>
#include <vector>

// Save slot management screen.
//  - Create: reached from Title "start game". Immediately prompts for a
//    character id and creates a new save in the next free slot.
//  - Load:   reached from Title "load game". Lists ALL existing saves
//    (dynamic count) and supports loading / deleting each one.
class SaveSlotScene : public engine::IScene
{
public:
    enum class Mode
    {
        Create,
        Load
    };

    explicit SaveSlotScene(Mode mode = Mode::Load);

    void handleInput(engine::IInput &input) override;
    void update(float deltaTime) override;
    void render(engine::IRenderer &renderer) override;

private:
    enum class SubState
    {
        Browsing,
        Naming
    };

    void refresh();
    void handleBrowsingInput(engine::IInput &input);
    void handleNamingInput(engine::IInput &input);
    void renderBrowsing(engine::IRenderer &renderer);
    void renderNaming(engine::IRenderer &renderer);

    Mode mode_;
    SubState subState_ = SubState::Browsing;
    int selectedIndex_ = 0;
    int scrollOffset_ = 0;
    std::string nameBuffer_;
    std::string message_;
    std::vector<SaveSlotInfo> slots_;
};
