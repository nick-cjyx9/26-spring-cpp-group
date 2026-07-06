#pragma once

#include "IInput.h"

#include <array>

namespace tests
{

class MockInput : public engine::IInput
{
public:
    MockInput()
    {
        currentStates_.fill(false);
        previousStates_.fill(false);
    }

    bool isKeyPressed(engine::Key key) const override
    {
        size_t idx = static_cast<size_t>(key);
        if (idx >= currentStates_.size())
            return false;
        return currentStates_[idx];
    }

    bool wasKeyJustPressed(engine::Key key) override
    {
        size_t idx = static_cast<size_t>(key);
        if (idx >= currentStates_.size() || idx >= previousStates_.size())
            return false;
        return currentStates_[idx] && !previousStates_[idx];
    }

    void setKeyPressed(engine::Key key, bool pressed)
    {
        size_t idx = static_cast<size_t>(key);
        if (idx < currentStates_.size())
            currentStates_[idx] = pressed;
    }

    void endFrame() override
    {
        previousStates_ = currentStates_;
    }

private:
    std::array<bool, static_cast<size_t>(engine::Key::Space) + 1> currentStates_;
    std::array<bool, static_cast<size_t>(engine::Key::Space) + 1> previousStates_;
};

} // namespace tests
