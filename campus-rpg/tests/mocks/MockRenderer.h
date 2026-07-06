#pragma once

#include "IRenderer.h"

#include <vector>

namespace tests
{

struct DrawRectCall
{
    engine::Rect rect;
    engine::Color color;
};

struct DrawTextCall
{
    std::string text;
    engine::Vec2 pos;
    int size;
    engine::Color color;
};

struct DrawTextureCall
{
    std::string textureId;
    engine::Vec2 pos;
};

class MockRenderer : public engine::IRenderer
{
public:
    void clear() override { ++clearCount_; }
    void present() override { ++presentCount_; }

    void drawRect(const engine::Rect &rect, engine::Color color) override
    {
        rectCalls_.push_back({rect, color});
    }

    void drawTexture(const std::string &textureId, const engine::Vec2 &pos) override
    {
        textureCalls_.push_back({textureId, pos});
    }

    void drawText(const std::string &text, const engine::Vec2 &pos, int size, engine::Color color) override
    {
        textCalls_.push_back({text, pos, size, color});
    }

    int clearCount() const { return clearCount_; }
    int presentCount() const { return presentCount_; }
    const std::vector<DrawRectCall> &rectCalls() const { return rectCalls_; }
    const std::vector<DrawTextCall> &textCalls() const { return textCalls_; }
    const std::vector<DrawTextureCall> &textureCalls() const { return textureCalls_; }

    void reset()
    {
        clearCount_ = 0;
        presentCount_ = 0;
        rectCalls_.clear();
        textCalls_.clear();
        textureCalls_.clear();
    }

private:
    int clearCount_ = 0;
    int presentCount_ = 0;
    std::vector<DrawRectCall> rectCalls_;
    std::vector<DrawTextCall> textCalls_;
    std::vector<DrawTextureCall> textureCalls_;
};

} // namespace tests
