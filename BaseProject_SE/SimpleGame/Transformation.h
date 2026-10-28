#pragma once
#include "PrimitiveModels.h"
#include <algorithm>
#include <cmath>

namespace Transformation
{
constexpr float Duration = 3.f;

struct Sequence
{
    float remaining = 0;

    void Start()
    {
        remaining = Duration;
    }

    bool Active() const
    {
        return remaining > 0;
    }

    bool Update(float dt)
    {
        const bool wasActive = Active();
        remaining = (std::max)(0.f, remaining - dt);
        return wasActive && !Active();
    }
};

struct Pose
{
    Hunting::ModelId model;
    float x, y, lift, glow;
};

inline Pose Sample(float elapsed, float facingX, float facingY)
{
    const float t = (std::max)(0.f, (std::min)(1.f, elapsed / Duration));
    const float spin = 12.5663706f * t;
    const float angle = std::atan2(facingY, facingX) + spin;
    return {t < .45f ? Hunting::ModelId::StudentIdle : Hunting::ModelId::PlayerCast,
            std::cos(angle), std::sin(angle), std::sin(t * 3.14159265f) * 18.f,
            70.f + std::sin(t * 3.14159265f) * 110.f};
}
} // namespace Transformation
