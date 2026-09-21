#pragma once
#include "Actor.h"

namespace CharacterVisual
{
constexpr float WandX = 20.f;
constexpr float IdleHandY = -27.f;
constexpr float CastHandY = -40.f;
constexpr float WandHeadOffset = -26.f;

inline Game::Transform WandSocket(bool casting, bool mirror)
{
    // Inverse quarter-view projection: 32 * (x - y) = model-space horizontal offset.
    float offset = (mirror ? -WandX : WandX) / 64.f;
    return {offset, -offset, -(casting ? CastHandY : IdleHandY) - WandHeadOffset, 1};
}
} // namespace CharacterVisual
