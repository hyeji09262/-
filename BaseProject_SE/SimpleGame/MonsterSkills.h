#pragma once
#include <algorithm>
#include <cmath>
#include "HuntingMap.h"

namespace MonsterSkills
{
enum class Style
{
    Melee,
    Bolt,
    Fan,
    Poison,
    Charge
};

struct Profile
{
    Style style;
    float range;
    float cooldown;
    const char* warning;
};

inline Profile Get(int kind)
{
    switch (kind % 10)
    {
    case 1:
    case 8:
        return {Style::Bolt, 6.f, 2.5f, "마법탄 준비"};
    case 4:
    case 7:
        return {Style::Fan, 5.5f, 3.2f, "확산탄 준비"};
    case 0:
    case 2:
    case 9:
        return {Style::Poison, 4.f, 4.f, "독안개 준비"};
    case 3:
    case 5:
        return {Style::Charge, 4.5f, 3.f, "돌진 준비"};
    default:
        return {Style::Melee, .9f, 1.4f, "집게 공격"};
    }
}

struct Projectile
{
    Hunting::Position position;
    Hunting::Position direction;
    int damage;
    unsigned id;
    float remaining = 1.5f;
};

// Substeps keep hostile shots from tunneling through walls or the player.
template <typename Blocked, typename Hit>
bool Advance(Projectile& shot, float dt, Blocked blocked, Hit hit)
{
    const float elapsed = (std::min)(dt, shot.remaining);
    shot.remaining -= elapsed;
    const int steps = (std::max)(1, int(std::ceil(elapsed * 4.5f / .1f)));
    for (int step = 0; step < steps; ++step)
    {
        shot.position.x += shot.direction.x * 4.5f * elapsed / steps;
        shot.position.y += shot.direction.y * 4.5f * elapsed / steps;
        if (blocked(shot.position) || hit(shot.position))
        {
            return false;
        }
    }
    return shot.remaining > 0;
}

struct Poison
{
    Hunting::Position position;
    int damage;
    unsigned id;
    float age = 0;
    float nextHit = .7f;
    static constexpr float Warning = .7f;
    static constexpr float Duration = 2.5f;
    static constexpr float Radius = 1.15f;

    bool Advance(float dt)
    {
        age += dt;
        if (age < nextHit || age >= Duration)
        {
            return false;
        }
        nextHit += .8f;
        return true;
    }
};
} // namespace MonsterSkills
