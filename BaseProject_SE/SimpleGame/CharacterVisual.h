#pragma once
#include "Actor.h"
#include <algorithm>

namespace CharacterVisual
{
constexpr float WandX = 20.f;
constexpr float IdleHandY = -27.f;
constexpr float CastHandY = -40.f;
constexpr float WandHeadOffset = -26.f;
constexpr float CastDuration = .25f;
constexpr float WalkFramesPerSecond = 9.f;
constexpr float SpellSpeed = 9.f;
constexpr float SpellSize = 30.f;

inline int FrameIndex(float castRemaining, bool moving, float walkFrame)
{
    return castRemaining > 0 ? 5 : moving ? 1 + static_cast<int>(walkFrame) % 4 : 0;
}

inline Game::Transform WandSocket(bool casting, bool mirror)
{
    // Inverse quarter-view projection: 32 * (x - y) = model-space horizontal offset.
    float offset = (mirror ? -WandX : WandX) / 64.f;
    return {offset, -offset, -(casting ? CastHandY : IdleHandY) - WandHeadOffset, 1};
}

inline Game::Transform SpellPose(float x, float y, float age, bool mirror)
{
    auto socket = WandSocket(true, mirror);
    float blend = 1.f - (std::min)(1.f, age / .18f);
    return {x + socket.x * blend, y + socket.y * blend, 27.f + (socket.z - 27.f) * blend, 1};
}
} // namespace CharacterVisual
