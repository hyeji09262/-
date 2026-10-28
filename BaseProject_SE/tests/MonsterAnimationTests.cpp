#include <cassert>
#include "../SimpleGame/MonsterAnimation.h"

int main()
{
    for (float dt : {.016f, .033f, .05f})
    {
        float remaining = MonsterAnimation::Duration;
        int impacts = 0;
        for (int frame = 0; frame < 100; ++frame)
        {
            impacts += MonsterAnimation::Advance(remaining, dt) ? 1 : 0;
        }
        assert(impacts == 1);
        assert(remaining == 0);
    }
    float timer = MonsterAnimation::Duration;
    assert(!MonsterAnimation::Advance(timer, .1f));
    assert(MonsterAnimation::Advance(timer, .3f));
    assert(!MonsterAnimation::Advance(timer, .1f));
    for (int kind = 0; kind < 11; ++kind)
    {
        const int start = int(Hunting::ModelId::Mob0Frame0) + kind * 10;
        assert(int(MonsterAnimation::Model(kind, 0, .8f)) == start + 4);
        assert(int(MonsterAnimation::Model(kind, 0, .45f)) == start + 7);
        assert(int(MonsterAnimation::Model(kind, 0, .001f)) == start + 9);
        for (int sector = 0; sector < 8; ++sector)
        {
            const float angle = sector * .7853981634f;
            for (float attack : {0.f, .8f, .45f, .001f})
            {
                const int frame = int(MonsterAnimation::Model(kind, 0, attack)) - start;
                const int id = int(MonsterAnimation::FacingModel(kind, 0, attack, std::cos(angle),
                                                                 std::sin(angle)));
                assert(id ==
                       int(Hunting::ModelId::MobFacingBegin) + (kind * 10 + frame) * 8 + sector);
                assert(id < int(Hunting::ModelId::Count));
            }
        }
    }
    for (int sector = 0; sector < 8; ++sector)
    {
        const float angle = sector * .7853981634f;
        const int id =
            int(MonsterAnimation::FacingModel(11, 0, .8f, std::cos(angle), std::sin(angle)));
        assert(id == int(Hunting::ModelId::MobFacingBegin) + (11 * 10 + 4) * 8 + sector);
        assert(id < int(Hunting::ModelId::Count));
    }
}
