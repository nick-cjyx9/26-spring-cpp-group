#pragma once

#include "IRenderer.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Font.hpp>

#include <map>
#include <memory>
#include <string>

namespace engine
{

class SfmlRenderer : public IRenderer
{
public:
    explicit SfmlRenderer(sf::RenderWindow &window);

    void clear() override;
    void present() override;

    void drawRect(const Rect &rect, Color color) override;
    void drawTexture(const std::string &textureId, const Vec2 &pos) override;
    void drawTexture(const std::string &textureId, const Rect &dstRect) override;
    void drawText(const std::string &text, const Vec2 &pos, int size, Color color) override;
    float textWidth(const std::string &text, int size) const override;

    // Resource management
    bool loadFont(const std::string &id, const std::string &path);
    bool loadTexture(const std::string &id, const std::string &path);

    // Load all textures from resources/textures/*.png and font from resources/fonts/default.ttf
    void loadDefaultResources();

private:
    sf::RenderWindow &window_;
    sf::RectangleShape rectShape_;
    sf::Text textShape_;
    std::map<std::string, std::unique_ptr<sf::Font>> fonts_;
    std::map<std::string, std::unique_ptr<sf::Texture>> textures_;
};

} // namespace engine
