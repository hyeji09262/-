#include <cassert>
#include "../SimpleGame/QuestCombat.h"

int main()
{
    {
        MainQuest::State state;
        state.active = true;
        state.step = MainQuest::Step::Shadows;
        MainQuest::Combat combat;
        combat.Enter(state);
        combat.enemies[0].position = {4, 0};
        combat.enemies[1].position = {0, 4};
        combat.Starfall({0, 0}, {1, 0}, state,
                        [](MainQuest::Point p)
                        {
                            return p.x < .5f;
                        });
        assert(combat.mana == 30 && combat.starfall == 0);
        const auto walkable = [](MainQuest::Point)
        {
            return true;
        };
        combat.Starfall({0, 0}, {1, 0}, state, walkable);
        assert(combat.mana == 22 && combat.starfall > 0);
        assert(combat.enemies[0].health < 24 && combat.enemies[1].health == 24);
        combat.Starfall({0, 0}, {1, 0}, state, walkable);
        assert(combat.mana == 22);
    }
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

    for (bool enraged : {false, true})
    {
        MainQuest::State state;
        state.active = true;
        state.step = MainQuest::Step::Boss;
        MainQuest::Combat combat;
        combat.Enter(state);
        combat.variedSkills = true;
        auto& boss = combat.enemies.front();
        boss.position = {0, 0};
        boss.pattern = 3;
        boss.enhancedBossMaxHealth = 250.f;
        boss.health = enraged ? 125.f : 250.f;
        MainQuest::Point player{6, 0};
        auto walkable = [](MainQuest::Point)
        {
            return true;
        };
        combat.Update(.05f, player, state, walkable);
        assert(boss.phase == 1 && boss.bossAttackEnraged == enraged);
        player = {0, 6};
        while (boss.phase == 1)
        {
            combat.Update(.05f, player, state, walkable);
        }
        assert(combat.hostileShots.size() == (enraged ? 7u : 5u));
        const auto& center = combat.hostileShots[combat.hostileShots.size() / 2];
        assert(center.direction.x == 1.f && center.direction.y == 0.f);
    }
}
