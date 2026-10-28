#pragma once
#include <array>

namespace BuildingVisuals
{
// World-space footprint/entrance and pixel-space label/shadow sizes from the source model.
struct Style
{
    float halfWidth = 2.25f;
    float halfDepth = 1.85f;
    float entranceX = 0;
    float entranceY = 3;
    float labelHeight = 220;
    float shadowRadius = 54;
};

inline std::array<Style, 9>& Styles()
{
    static auto styles = []()
    {
        std::array<Style, 9> result{};
        result[4].halfWidth = 3.9f;
        result[4].labelHeight = 270;
        result[4].shadowRadius = 92;
        return result;
    }();
    return styles;
}

inline const Style& Get(int style)
{
    return Styles()[style];
}

inline void Set(const std::array<Style, 9>& styles)
{
    Styles() = styles;
}
} // namespace BuildingVisuals
