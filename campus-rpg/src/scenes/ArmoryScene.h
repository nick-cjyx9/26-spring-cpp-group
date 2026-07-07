#pragma once

#include "IScene.h"

#include <memory>
#include <string>
#include <vector>

class EquipmentItem;

// Armory / Equipment library scene.
// Left side: 4 categories (Weapon/Armor/Accessory/Relic).
// Right side: 3-column grid showing all items of the selected category.
// Each item shows a placeholder icon (colored rect) + name + stats.
// Top: detailed preview of the hovered/selected item.
class ArmoryScene : public engine::IScene
{
public:
    void handleInput(engine::IInput &input) override;
    void update(float deltaTime) override;
    void render(engine::IRenderer &renderer) override;

private:
    enum class Focus
    {
        LeftPanel,
        RightPanel
    };

    void handleLeftPanelInput(engine::IInput &input);
    void handleRightPanelInput(engine::IInput &input);

    void renderLeftPanel(engine::IRenderer &renderer);
    void renderRightPanel(engine::IRenderer &renderer);
    void renderItemDetail(engine::IRenderer &renderer);
    void renderItemGrid(engine::IRenderer &renderer);

    Focus focus_ = Focus::LeftPanel;
    int categoryIndex_ = 0;    // 0=Weapon, 1=Armor, 2=Accessory, 3=Relic
    int selectedIndex_ = 0;    // index in the filtered list
    int gridScrollOffset_ = 0; // row offset for the right-panel grid
    std::string message_;
    float messageTimer_ = 0.0f;
};
