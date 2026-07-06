#pragma once

#include "EngineTypes.h"

namespace engine
{

// Abstract renderer interface. All drawing requests go through this interface
// so that the game logic and scenes do not depend on SFML directly.
// Mock implementations can record draw calls for unit testing.
class IRenderer
{
public:
    virtual ~IRenderer() = default;

    virtual void clear() = 0;
    virtual void present() = 0;

    virtual void drawRect(const Rect &rect, Color color) = 0;
    virtual void drawTexture(const std::string &textureId, const Vec2 &pos) = 0;
    virtual void drawTexture(const std::string &textureId, const Rect &dstRect) = 0;
    virtual void drawText(const std::string &text, const Vec2 &pos, int size, Color color) = 0;
};

} // namespace engine
