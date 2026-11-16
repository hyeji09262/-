#pragma once

#include "Renderer.h"

namespace MagicalTheme
{
constexpr float BackgroundR = .05f;
constexpr float BackgroundG = .06f;
constexpr float BackgroundB = .12f;
constexpr float TextR = .84f;
constexpr float TextG = .83f;
constexpr float TextB = .88f;

// Draw inside the owning HUD actor's existing screen-space render callback.
inline void Panel(Renderer& renderer, float x, float y, float w, float h, float scale)
{
    const auto rect = [&](float left, float top, float width, float height,
                          float r, float g, float b)
    {
        renderer.Triangle(left, top, left + width, top, left + width, top + height, r, g, b, 1);
        renderer.Triangle(left, top, left + width, top + height, left, top + height, r, g, b, 1);
    };
    rect(x, y, w, h, .28f, .23f, .36f);
    rect(x + 2 * scale, y + 2 * scale, w - 4 * scale, h - 4 * scale, BackgroundR, BackgroundG, BackgroundB);
    rect(x + 5 * scale, y + 5 * scale, w - 10 * scale, 3 * scale, .38f, .26f, .36f);
    if (w < 120 * scale || h < 60 * scale)
    {
        return;
    }

    const float cx = x + w - 17 * scale;
    const float cy = y + 14 * scale;
    renderer.Triangle(cx, cy, cx - 11 * scale, cy - 6 * scale,
                      cx - 10 * scale, cy + 6 * scale, .50f, .32f, .43f, 1);
    renderer.Triangle(cx, cy, cx + 11 * scale, cy - 6 * scale,
                      cx + 10 * scale, cy + 6 * scale, .50f, .32f, .43f, 1);
    renderer.Triangle(cx, cy - 7 * scale, cx + 3 * scale, cy,
                      cx, cy + 7 * scale, .66f, .57f, .42f, 1);
    renderer.Triangle(cx, cy - 7 * scale, cx - 3 * scale, cy,
                      cx, cy + 7 * scale, .66f, .57f, .42f, 1);
}
} // namespace MagicalTheme
