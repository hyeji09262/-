#pragma once
#include "Actor.h"
#include <algorithm>
#include <array>
#include <cmath>

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

inline std::array<float, 4>& ResourceSockets()
{
    static std::array<float, 4> sockets{
        {WandX, IdleHandY + WandHeadOffset, WandX, CastHandY + WandHeadOffset}};
    return sockets;
}

inline void SetResourceSockets(const std::array<float, 4>& sockets)
{
    ResourceSockets() = sockets;
}

inline int FacingSector(float x, float y)
{
    const int sector = int(std::floor(std::atan2(y, x) / .7853981634f + .5f));
    return (sector % 8 + 8) % 8;
}

inline std::array<float, 32>& DirectionSockets()
{
    static std::array<float, 32> sockets{};
    return sockets;
}

inline bool& HasDirectionSockets()
{
    static bool loaded = false;
    return loaded;
}

inline void SetDirectionSockets(const std::array<float, 32>& sockets)
{
    DirectionSockets() = sockets;
    HasDirectionSockets() = true;
}

inline int FrameIndex(float castRemaining, bool moving, float walkFrame)
{
    return castRemaining > 0 ? 5 : moving ? 1 + static_cast<int>(walkFrame) % 4 : 0;
}

inline Game::Transform WandSocket(bool casting, bool mirror, float halfTileWidth = 32.f)
{
    // Inverse quarter-view projection: 32 * (x - y) = model-space horizontal offset.
    const auto& sockets = ResourceSockets();
    const int index = casting ? 2 : 0;
    float offset = (mirror ? -sockets[index] : sockets[index]) / (2 * halfTileWidth);
    return {offset, -offset, -sockets[index + 1], 1};
}

inline Game::Transform SpellPose(float x, float y, float age, bool mirror,
                                 float halfTileWidth = 32.f)
{
    auto socket = WandSocket(true, mirror, halfTileWidth);
    float blend = 1.f - (std::min)(1.f, age / .18f);
    return {x + socket.x * blend, y + socket.y * blend, 27.f + (socket.z - 27.f) * blend, 1};
}

inline Game::Transform FacingSocket(bool casting, float x, float y, float halfTileWidth = 32.f)
{
    if (!HasDirectionSockets())
    {
        return WandSocket(casting, x - y < 0, halfTileWidth);
    }
    const int index = FacingSector(x, y) * 4 + (casting ? 2 : 0);
    const auto& sockets = DirectionSockets();
    const float offset = sockets[index] / (2 * halfTileWidth);
    return {offset, -offset, -sockets[index + 1], 1};
}

inline Game::Transform FacingSpellPose(float x, float y, float age, float dx, float dy,
                                       float halfTileWidth = 32.f)
{
    const auto socket = FacingSocket(true, dx, dy, halfTileWidth);
    // A fixed socket offset makes the visual follow the same straight direction as the shot.
    return {x + socket.x, y + socket.y, socket.z, 1};
}
} // namespace CharacterVisual
