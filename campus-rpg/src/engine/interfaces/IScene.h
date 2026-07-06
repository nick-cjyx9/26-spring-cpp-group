#pragma once

#include "IInput.h"
#include "IRenderer.h"

namespace engine
{

// Abstract game scene. Each scene handles its own input, update and render.
class IScene
{
public:
    virtual ~IScene() = default;

    virtual void handleInput(IInput &input) = 0;
    virtual void update(float deltaTime) = 0;
    virtual void render(IRenderer &renderer) = 0;
};

} // namespace engine
