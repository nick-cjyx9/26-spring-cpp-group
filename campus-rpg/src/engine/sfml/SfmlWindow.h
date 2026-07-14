#pragma once

#include "IWindow.h"
#include "SfmlInput.h"
#include "SfmlRenderer.h"

#include <SFML/Graphics/RenderWindow.hpp>

#include <memory>
#include <string>

namespace engine
{

    class SfmlWindow : public IWindow
    {
    public:
        SfmlWindow(int width, int height, const std::string &title);

        bool isOpen() const override;
        void close() override;
        void pollEvents(IInput &input) override;
        IRenderer &renderer() override;

    private:
        sf::RenderWindow window_;
        SfmlRenderer renderer_;
        SfmlInput *input_ = nullptr;
    };

} // namespace engine
