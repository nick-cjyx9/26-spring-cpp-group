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
};

} // namespace engine
