#pragma once

#include "IScene.h"
#include <string>
#include <vector>

class FinalBossIntroScene : public engine::IScene
{
public:
    void handleInput(engine::IInput &input) override;
    void update(float deltaTime) override;
    void render(engine::IRenderer &renderer) override;

private:
    std::vector<std::string> dialogues_ = {
        "You have come far, mortal...",
        "I am the Shadow Overlord, the true ruler of this realm.",
        "Those who dared to challenge me before have all become dust.",
        "Now, face your destiny!"
    };
    int currentDialogueIndex_ = 0;
    bool firstFrame_ = true;
    float timer_ = 0.0f;
    std::string message_;
    float messageTimer_ = 0.0f;
    static constexpr float kMessageDuration = 3.0f;
};