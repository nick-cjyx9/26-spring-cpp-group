#include "SfmlInput.h"

#include <algorithm>

namespace engine
{

SfmlInput::SfmlInput()
{
    currentStates_.fill(false);
    previousStates_.fill(false);
    justPressed_.fill(false);
}

bool SfmlInput::isKeyPressed(Key key) const
{
    size_t idx = static_cast<size_t>(key);
    if (idx >= currentStates_.size())
        return false;
    return currentStates_[idx];
}

bool SfmlInput::wasKeyJustPressed(Key key)
{
    size_t idx = static_cast<size_t>(key);
    if (idx >= justPressed_.size())
        return false;
    return justPressed_[idx];
}

void SfmlInput::onSfmlKeyPressed(sf::Keyboard::Key key)
{
    for (size_t i = 1; i <= static_cast<size_t>(Key::Space); ++i)
    {
        if (toSfmlKey(static_cast<Key>(i)) == key && !currentStates_[i])
        {
            currentStates_[i] = true;
            justPressed_[i] = true;
        }
    }
}

void SfmlInput::onSfmlKeyReleased(sf::Keyboard::Key key)
{
    for (size_t i = 1; i <= static_cast<size_t>(Key::Space); ++i)
    {
        if (toSfmlKey(static_cast<Key>(i)) == key)
        {
            currentStates_[i] = false;
        }
    }
}

void SfmlInput::endFrame()
{
    std::copy(currentStates_.begin(), currentStates_.end(), previousStates_.begin());
    justPressed_.fill(false);
}

sf::Keyboard::Key SfmlInput::toSfmlKey(Key key)
{
    switch (key)
    {
    case Key::Up:
        return sf::Keyboard::Up;
    case Key::Down:
        return sf::Keyboard::Down;
    case Key::Left:
        return sf::Keyboard::Left;
    case Key::Right:
        return sf::Keyboard::Right;
    case Key::W:
        return sf::Keyboard::W;
    case Key::A:
        return sf::Keyboard::A;
    case Key::S:
        return sf::Keyboard::S;
    case Key::D:
        return sf::Keyboard::D;
    case Key::E:
        return sf::Keyboard::E;
    case Key::Enter:
        return sf::Keyboard::Enter;
    case Key::Escape:
        return sf::Keyboard::Escape;
    case Key::I:
        return sf::Keyboard::I;
    case Key::C:
        return sf::Keyboard::C;
    case Key::N:
        return sf::Keyboard::N;
    case Key::Space:
        return sf::Keyboard::Space;
    default:
        return sf::Keyboard::Unknown;
    }
}

} // namespace engine
