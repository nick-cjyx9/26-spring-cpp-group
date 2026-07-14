#pragma once

#include "IInput.h"

#include <array>
#include <string>

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

        std::string consumeTypedText() override
        {
            std::string text;
            text.swap(typedBuffer_);
            return text;
        }

        int consumeScrollDelta() override
        {
            int d = scrollDelta_;
            scrollDelta_ = 0;
            return d;
        }

        // Test helper: inject text as if the user typed it.
        void typeText(const std::string &text) { typedBuffer_ += text; }

        void endFrame() override
        {
            previousStates_ = currentStates_;
        }

        engine::Vec2 mousePosition() const override { return {0.0f, 0.0f}; }

    private:
        std::array<bool, static_cast<size_t>(engine::Key::Count)> currentStates_;
        std::array<bool, static_cast<size_t>(engine::Key::Count)> previousStates_;
        std::string typedBuffer_;
        int scrollDelta_ = 0;
    };

} // namespace tests
