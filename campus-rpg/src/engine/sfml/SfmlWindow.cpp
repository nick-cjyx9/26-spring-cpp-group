#include "SfmlWindow.h"

#include <SFML/Window/Event.hpp>

namespace engine
{

SfmlWindow::SfmlWindow(int width, int height, const std::string &title)
    : window_(sf::VideoMode(static_cast<unsigned int>(width), static_cast<unsigned int>(height)), title),
      renderer_(window_)
{
    window_.setFramerateLimit(60);
}

bool SfmlWindow::isOpen() const
{
    return window_.isOpen();
}

void SfmlWindow::close()
{
    window_.close();
}

void SfmlWindow::pollEvents(IInput &input)
{
    SfmlInput *sfmlInput = dynamic_cast<SfmlInput *>(&input);
    if (sfmlInput)
        input_ = sfmlInput;

    sf::Event event;
    while (window_.pollEvent(event))
    {
        if (event.type == sf::Event::Closed)
        {
            window_.close();
        }
        else if (input_)
        {
            if (event.type == sf::Event::KeyPressed)
                input_->onSfmlKeyPressed(event.key.code);
            else if (event.type == sf::Event::KeyReleased)
                input_->onSfmlKeyReleased(event.key.code);
            else if (event.type == sf::Event::TextEntered)
                input_->onSfmlTextEntered(event.text.unicode);
            else if (event.type == sf::Event::MouseWheelScrolled)
                input_->onSfmlMouseWheelScrolled(static_cast<int>(event.mouseWheelScroll.delta));
        }
    }
}

IRenderer &SfmlWindow::renderer()
{
    return renderer_;
}

} // namespace engine
