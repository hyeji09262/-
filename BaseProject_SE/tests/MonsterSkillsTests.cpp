#include <cassert>
#include "../SimpleGame/MonsterSkills.h"

int main()
{
    using namespace MonsterSkills;
    assert(Get(1).style == Style::Bolt);
    assert(Get(4).style == Style::Fan);
    assert(Get(2).style == Style::Poison);
    assert(Get(3).style == Style::Charge);
    assert(Get(6).style == Style::Melee);
    for (float dt : {.016f, .033f, .05f})
    {
        Projectile shot{{0, 0}, {1, 0}, 10, 1};
        bool hit = false;
        for (int i = 0; i < 200; ++i)
        {
            if (!Advance(
                    shot, dt,
                    [](Hunting::Position)
                    {
                        return false;
                    },
                    [&hit](Hunting::Position p)
                    {
                        hit = std::abs(p.x - 2.f) < .1f;
                        return hit;
                    }))
            {
                break;
            }
        }
        assert(hit);

        Projectile blocked{{0, 0}, {1, 0}, 10, 1};
        bool behindWallHit = false;
        for (int i = 0; i < 200; ++i)
        {
            if (!Advance(
                    blocked, dt,
                    [](Hunting::Position p)
                    {
                        return p.x >= 1;
                    },
                    [&behindWallHit](Hunting::Position p)
                    {
                        behindWallHit = p.x >= 2;
                        return behindWallHit;
                    }))
            {
                break;
            }
        }
        assert(blocked.position.x < 1.11f && !behindWallHit);

        Poison poison{{0, 0}, 10, 1};
        int ticks = 0;
        for (int i = 0; i < 300; ++i)
        {
            const bool damage = poison.Advance(dt);
            if (poison.age < Poison::Warning)
            {
                assert(!damage);
            }
            ticks += damage ? 1 : 0;
        }
        assert(ticks == 3);
    }
}
