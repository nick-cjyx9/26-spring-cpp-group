#pragma once

#include "IInput.h"
#include "IRenderer.h"

namespace engine
{

    // Abstract window interface. The game loop only depends on this interface.
    class IWindow
    {
    public:
        virtual ~IWindow() = default;

        virtual bool isOpen() const = 0;
        virtual void close() = 0;
        virtual void pollEvents(IInput &input) = 0;
        virtual IRenderer &renderer() = 0;
    };

} // namespace engine
