#include "SfmlRenderer.h"

#include <SFML/Graphics/Sprite.hpp>

namespace engine
{

SfmlRenderer::SfmlRenderer(sf::RenderWindow &window)
    : window_(window)
{
    rectShape_.setOutlineThickness(0.0f);
}

void SfmlRenderer::clear()
{
    window_.clear(sf::Color::Black);
}

void SfmlRenderer::present()
{
    window_.display();
}

void SfmlRenderer::drawRect(const Rect &rect, Color color)
{
    rectShape_.setPosition(rect.x, rect.y);
    rectShape_.setSize({rect.width, rect.height});
    rectShape_.setFillColor(sf::Color(color.r, color.g, color.b, color.a));
    window_.draw(rectShape_);
}

void SfmlRenderer::drawTexture(const std::string &textureId, const Vec2 &pos)
{
    auto it = textures_.find(textureId);
    if (it == textures_.end() || !it->second)
        return;
    sf::Sprite sprite(*it->second);
    sprite.setPosition(pos.x, pos.y);
    window_.draw(sprite);
}

void SfmlRenderer::drawText(const std::string &text, const Vec2 &pos, int size, Color color)
{
    auto it = fonts_.find("default");
    if (it == fonts_.end() || !it->second)
        return;
    textShape_.setFont(*it->second);
    textShape_.setString(text);
    textShape_.setCharacterSize(static_cast<unsigned int>(size));
    textShape_.setFillColor(sf::Color(color.r, color.g, color.b, color.a));
    textShape_.setPosition(pos.x, pos.y);
    window_.draw(textShape_);
}

bool SfmlRenderer::loadFont(const std::string &id, const std::string &path)
{
    auto font = std::make_unique<sf::Font>();
    if (!font->loadFromFile(path))
        return false;
    fonts_[id] = std::move(font);
    return true;
}

bool SfmlRenderer::loadTexture(const std::string &id, const std::string &path)
{
    auto texture = std::make_unique<sf::Texture>();
    if (!texture->loadFromFile(path))
        return false;
    textures_[id] = std::move(texture);
    return true;
}

} // namespace engine
