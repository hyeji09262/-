#pragma once
#include <cstdint>
#include "PrimitiveModels.h"

namespace NatureVisuals
{
inline std::uint32_t Seed(float x, float y)
{
    auto value = std::uint32_t(int(x * 4)) * 73856093u ^ std::uint32_t(int(y * 4)) * 19349663u;
    value ^= value >> 13;
    return value * 1274126177u;
}

inline float Scale(float x, float y)
{
    return .85f + float(Seed(x, y) % 31) * .01f;
}

inline Hunting::ModelId Tree(float x, float y)
{
    const Hunting::ModelId ids[] = {Hunting::ModelId::Tree, Hunting::ModelId::TreeB,
                                    Hunting::ModelId::TreeC};
    return ids[(Seed(x, y) >> 8) % 3];
}

inline Hunting::ModelId Rock(float x, float y)
{
    const Hunting::ModelId ids[] = {Hunting::ModelId::Rock, Hunting::ModelId::RockB,
                                    Hunting::ModelId::RockC};
    return ids[(Seed(x, y) >> 8) % 3];
}

inline Hunting::ModelId Grass(float x, float y)
{
    const Hunting::ModelId ids[] = {Hunting::ModelId::GrassA, Hunting::ModelId::GrassB,
                                    Hunting::ModelId::GrassC};
    return ids[(Seed(x, y) >> 8) % 3];
}
} // namespace NatureVisuals
