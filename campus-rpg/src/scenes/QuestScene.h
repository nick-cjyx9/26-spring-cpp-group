#pragma once

#include "IScene.h"

#include <string>
#include <vector>

class QuestScene : public engine::IScene
{
public:
    void handleInput(engine::IInput &input) override;
    void update(float deltaTime) override;
    void render(engine::IRenderer &renderer) override;

private:
    int selectedIndex_ = 0;
    int scrollOffset_ = 0;
    static constexpr int kVisibleCount = 7;
    std::string message_;
    float messageTimer_ = 0.0f;
    static constexpr float kMessageDuration = 2.0f;

    enum class QuestTab
    {
        Available,
        Active,
        Completed
    };
    QuestTab currentTab_ = QuestTab::Available;

    void drawQuestPanel(engine::IRenderer &renderer, float x, float y, float w, float h,
                         bool selected, const std::string &name, const std::string &desc,
                         const std::string &reward, const std::string &status, int textColor = 0) const;
};
