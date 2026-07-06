#pragma once

#include "IInput.h"

#include <SFML/Window/Keyboard.hpp>

#include <array>

namespace engine
{

class SfmlInput : public IInput
{
public:
    SfmlInput();

    bool isKeyPressed(Key key) const override;
    bool wasKeyJustPressed(Key key) override;

    // Called by SfmlWindow on each SFML event.
    void onSfmlKeyPressed(sf::Keyboard::Key key);
    void onSfmlKeyReleased(sf::Keyboard::Key key);
    void endFrame();

private:
    static sf::Keyboard::Key toSfmlKey(Key key);

    std::array<bool, static_cast<size_t>(Key::Space) + 1> currentStates_;
    std::array<bool, static_cast<size_t>(Key::Space) + 1> previousStates_;
    std::array<bool, static_cast<size_t>(Key::Space) + 1> justPressed_;
};

} // namespace engine
