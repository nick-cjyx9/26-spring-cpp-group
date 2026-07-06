#include "SfmlRenderer.h"

#include <SFML/Graphics/Sprite.hpp>

#include <filesystem>
#include <iostream>

namespace engine
{

SfmlRenderer::SfmlRenderer(sf::RenderWindow &window)
    : window_(window)
{
    rectShape_.setOutlineThickness(0.0f);
    loadDefaultResources();
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
    {
        std::string path = "resources/textures/" + textureId + ".png";
        if (!loadTexture(textureId, path))
            return;
        it = textures_.find(textureId);
        if (it == textures_.end())
            return;
    }
    sf::Sprite sprite(*it->second);
    sprite.setPosition(pos.x, pos.y);
    window_.draw(sprite);
}

void SfmlRenderer::drawTexture(const std::string &textureId, const Rect &dstRect)
{
    auto it = textures_.find(textureId);
    if (it == textures_.end() || !it->second)
    {
        std::string path = "resources/textures/" + textureId + ".png";
        if (!loadTexture(textureId, path))
            return;
        it = textures_.find(textureId);
        if (it == textures_.end())
            return;
    }
    sf::Sprite sprite(*it->second);
    sprite.setPosition(dstRect.x, dstRect.y);
    sprite.setScale(dstRect.width / sprite.getLocalBounds().width,
                    dstRect.height / sprite.getLocalBounds().height);
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
    {
        std::cerr << "Failed to load font: " << path << "\n";
        return false;
    }
    fonts_[id] = std::move(font);
    return true;
}

bool SfmlRenderer::loadTexture(const std::string &id, const std::string &path)
{
    auto texture = std::make_unique<sf::Texture>();
    if (!texture->loadFromFile(path))
    {
        std::cerr << "Failed to load texture: " << path << "\n";
        return false;
    }
    textures_[id] = std::move(texture);
    return true;
}

void SfmlRenderer::loadDefaultResources()
{
    std::filesystem::path exeDir = std::filesystem::current_path();
    std::filesystem::path texturesDir = exeDir / "resources" / "textures";
    std::filesystem::path fontsDir = exeDir / "resources" / "fonts";

    if (!std::filesystem::exists(texturesDir))
        texturesDir = exeDir / ".." / "resources" / "textures";
    if (!std::filesystem::exists(fontsDir))
        fontsDir = exeDir / ".." / "resources" / "fonts";

    loadFont("default", (fontsDir / "default.ttf").string());

    if (std::filesystem::exists(texturesDir))
    {
        for (const auto &entry : std::filesystem::directory_iterator(texturesDir))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".png")
            {
                std::string id = entry.path().stem().string();
                loadTexture(id, entry.path().string());
            }
        }
    }
}

} // namespace engine
