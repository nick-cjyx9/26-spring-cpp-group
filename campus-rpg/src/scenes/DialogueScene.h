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

    // Rank-up banner state (shown when a rank-up occurs during this talk).
    bool showRankUpBanner_ = false;
    float rankUpTimer_ = 0.0f;
    static constexpr float kRankUpDuration = 2.5f; // seconds the banner stays visible

    // Cached texture IDs so we don't rebuild strings every frame.
    std::string avatarTexId_; // small portrait top-left
    std::string fullTexId_;   // full sprite bottom-right
    bool hasTextures_ = false;

    // Draws 5 heart slots below the NPC sprite.
    void drawHearts(engine::IRenderer &renderer, float x, float y, int rank) const;
};
