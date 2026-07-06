#pragma once

#include "IWindow.h"
#include "MockRenderer.h"

namespace tests
{

class MockWindow : public engine::IWindow
{
public:
    bool isOpen() const override { return isOpen_; }
    void close() override { isOpen_ = false; }
    void pollEvents(engine::IInput & /*input*/) override {}
    engine::IRenderer &renderer() override { return renderer_; }

    MockRenderer &mockRenderer() { return renderer_; }
    void setOpen(bool open) { isOpen_ = open; }

private:
    bool isOpen_ = true;
    MockRenderer renderer_;
};

} // namespace tests
