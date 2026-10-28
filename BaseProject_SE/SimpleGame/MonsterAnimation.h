#pragma once
#include <algorithm>
#include "PrimitiveModels.h"
#include "CharacterVisual.h"

namespace MonsterAnimation
{
constexpr float Duration = .8f;
constexpr float ImpactRemaining = .45f;
constexpr int WildKinds = 10;

inline bool Advance(float& remaining, float dt)
{
    const float before = remaining;
    remaining = (std::max)(0.f, remaining - dt);
    return before > ImpactRemaining && remaining <= ImpactRemaining;
}

inline Hunting::ModelId Model(int kind, float clock, float attack)
{
    const int attackFrame =
        attack > ImpactRemaining
            ? (std::min)(2, int((Duration - attack) / (Duration - ImpactRemaining) * 3))
            : 3 + (std::min)(2, int((ImpactRemaining - attack) / ImpactRemaining * 3));
    const int frame = attack > 0 ? 4 + attackFrame : int(clock * 4) % 4;
    return static_cast<Hunting::ModelId>(int(Hunting::ModelId::Mob0Frame0) + (kind % 11) * 10 +
                                         frame);
}

inline Hunting::ModelId FacingModel(int kind, float clock, float attack, float x, float y)
{
    const int frame =
        int(Model(kind, clock, attack)) - int(Hunting::ModelId::Mob0Frame0) - (kind % 11) * 10;
    return static_cast<Hunting::ModelId>(int(Hunting::ModelId::MobFacingBegin) +
                                         ((kind % 12) * 10 + frame) * 8 +
                                         CharacterVisual::FacingSector(x, y));
}

inline float BossMotion(int phase, float timer)
{
    if (phase == 1)
        return .8f - (1.1f - timer) / 1.1f * .35f;
    if (phase == 2)
        return .45f;
    if (phase == 3)
        return (std::max)(.001f, timer / 1.3f * .4f);
    return 0;
}

inline const char* Name(int kind)
{
    static const char* names[] = {"슬라임",      "유령 마법사", "독버섯",  "그림자 토끼",
                                  "마도서",      "악몽 박쥐",   "산호 게", "미믹 상자",
                                  "등불 도깨비", "가시 씨앗"};
    return names[kind % WildKinds];
}
} // namespace MonsterAnimation
