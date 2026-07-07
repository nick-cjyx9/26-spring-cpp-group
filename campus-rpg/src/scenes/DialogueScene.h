#pragma once

#include "IScene.h"

#include <string>

class DialogueScene : public engine::IScene
{
public:
    void handleInput(engine::IInput &input) override;
    void update(float deltaTime) override;
    void render(engine::IRenderer &renderer) override;

private:
    std::string npcId_;
    std::string dialogueText_;
    bool firstFrame_ = true;
};
