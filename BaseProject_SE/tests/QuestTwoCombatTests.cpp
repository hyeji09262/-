#include <cassert>
#include "../SimpleGame/QuestCombat.h"

int main()
{
    {
        MainQuest::State state;
        state.active = true;
        state.step = MainQuest::Step::Boss;
        MainQuest::Combat combat;
        combat.Enter(state);
        auto& boss = combat.enemies.front();
        boss.position = {0, 0};
        boss.aim = {1, 0};
        boss.facing = {1, 0};
        boss.phase = 2;
        boss.timer = .5f;
        MainQuest::Point player{10, 10};
        for (int i = 0; i < 10; ++i)
        {
            combat.Update(.05f, player, state,
                          [](MainQuest::Point)
                          {
                              return true;
                          });
        }
        assert(boss.position.x > 4.f && boss.facing.x == 1);
    }
    for (float dt : {.016f, .033f, .05f})
    {
        for (int kind : {1, 4, 9})
        {
            MainQuest::State state;
            state.active = true;
            state.step = MainQuest::Step::Shadows;
            MainQuest::Combat combat;
            combat.Enter(state);
            combat.variedSkills = true;
            combat.enemies.resize(1);
            combat.enemies[0].kind = kind;
            combat.enemies[0].position = {3, 0};
            MainQuest::Point player{0, 0};
            const float initialHealth = combat.health;
            for (float time = 0; time < 2.4f; time += dt)
            {
                combat.Update(dt, player, state,
                              [](MainQuest::Point)
                              {
                                  return true;
                              });
            }
            assert(combat.health < initialHealth);
            combat.Enter(state);
            assert(combat.hostileShots.empty() && combat.poisonZones.empty());
        }

        MainQuest::State state;
        state.active = true;
        state.step = MainQuest::Step::Shadows;
        MainQuest::Combat combat;
        combat.Enter(state);
        combat.variedSkills = true;
        combat.enemies.resize(1);
        combat.enemies[0].kind = 9;
        combat.enemies[0].position = {3, 0};
        MainQuest::Point player{0, 0};
        for (float time = 0; time < .6f; time += dt)
        {
            combat.Update(dt, player, state,
                          [](MainQuest::Point)
                          {
                              return true;
                          });
        }
        assert(combat.poisonZones.size() == 1);
        player = {0, 6};
        const float initialHealth = combat.health;
        for (float time = 0; time < 1.8f; time += dt)
        {
            combat.Update(dt, player, state,
                          [](MainQuest::Point)
                          {
                              return true;
                          });
        }
        assert(combat.health == initialHealth); // Leaving the warning avoids the locked area.
    }
}
