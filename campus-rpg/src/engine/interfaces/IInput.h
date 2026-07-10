#pragma once

#include "EngineTypes.h"

namespace engine
{

// Abstract input interface. Tests can use a mock implementation to inject
// specific key states without creating a real window.
class IInput
{
public:
    virtual ~IInput() = default;

    virtual bool isKeyPressed(Key key) const = 0;
    virtual bool wasKeyJustPressed(Key key) = 0;
    virtual void endFrame() = 0;

    // Returns text typed since the last call (printable ASCII chars plus
    // '\\b' for backspace). Cleared on read. Used for name/ID entry in scenes.
    virtual std::string consumeTypedText() = 0;

    // Returns accumulated mouse wheel delta since the last call.
    // Positive = scrolled up, negative = scrolled down, 0 = no scroll.
    virtual int consumeScrollDelta() = 0;
};

} // namespace engine
