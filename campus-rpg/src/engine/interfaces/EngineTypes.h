#pragma once

#include <cstdint>
#include <string>

namespace engine
{

struct Vec2
{
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}

    Vec2 operator+(const Vec2 &other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(const Vec2 &other) const { return {x - other.x, y - other.y}; }
    Vec2 &operator+=(const Vec2 &other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
};

struct Rect
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    Rect() = default;
    Rect(float x_, float y_, float w_, float h_) : x(x_), y(y_), width(w_), height(h_) {}

    float left() const { return x; }
    float right() const { return x + width; }
    float top() const { return y; }
    float bottom() const { return y + height; }

    bool intersects(const Rect &other) const
    {
        return left() < other.right() && right() > other.left() &&
               top() < other.bottom() && bottom() > other.top();
    }
};

struct Color
{
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;

    Color() = default;
    Color(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255)
        : r(r_), g(g_), b(b_), a(a_) {}

    static Color white() { return {255, 255, 255}; }
    static Color black() { return {0, 0, 0}; }
    static Color red() { return {255, 0, 0}; }
    static Color green() { return {0, 255, 0}; }
    static Color blue() { return {0, 0, 255}; }
    static Color yellow() { return {255, 255, 0}; }
    static Color cyan() { return {0, 255, 255}; }
    static Color gray() { return {128, 128, 128}; }
};

enum class Key
{
    Unknown,
    Up,
    Down,
    Left,
    Right,
    W,
    A,
    S,
    D,
    E,
    Enter,
    Escape,
    I,
    C,
    N,
    Space,
    F5,
    Count
};

inline std::string keyName(Key key)
{
    switch (key)
    {
    case Key::Up:
        return "Up";
    case Key::Down:
        return "Down";
    case Key::Left:
        return "Left";
    case Key::Right:
        return "Right";
    case Key::W:
        return "W";
    case Key::A:
        return "A";
    case Key::S:
        return "S";
    case Key::D:
        return "D";
    case Key::E:
        return "E";
    case Key::Enter:
        return "Enter";
    case Key::Escape:
        return "Escape";
    case Key::I:
        return "I";
    case Key::C:
        return "C";
    case Key::N:
        return "N";
    case Key::Space:
        return "Space";
    case Key::F5:
        return "F5";
    default:
        return "Unknown";
    }
}

} // namespace engine
