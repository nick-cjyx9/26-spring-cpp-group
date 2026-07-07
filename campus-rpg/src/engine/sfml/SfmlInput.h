#pragma once

#include "IInput.h"

#include <SFML/Window/Keyboard.hpp>

#include <array>
#include <cstdint>
#include <string>

namespace engine
{

class SfmlInput : public IInput
{
public:
    SfmlInput();

    bool isKeyPressed(Key key) const override;
    bool wasKeyJustPressed(Key key) override;
    std::string consumeTypedText() override;

    // Called by SfmlWindow on each SFML event.
    void onSfmlKeyPressed(sf::Keyboard::Key key);
    void onSfmlKeyReleased(sf::Keyboard::Key key);
    void onSfmlTextEntered(std::uint32_t unicode);
    void endFrame();

private:
    static sf::Keyboard::Key toSfmlKey(Key key);

    std::array<bool, static_cast<size_t>(Key::Count)> currentStates_;
    std::array<bool, static_cast<size_t>(Key::Count)> previousStates_;
    std::array<bool, static_cast<size_t>(Key::Count)> justPressed_;
    std::string typedBuffer_;
};

} // namespace engine
