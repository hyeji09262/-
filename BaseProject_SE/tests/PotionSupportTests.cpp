#include <cassert>
#include "../SimpleGame/QuestCombat.h"

int main()
{
    Rpg::Progress progress;
    assert(progress.potions == 5 && progress.manaPotions == 5);
    progress.potions = 0;
    progress.manaPotions = 2;
    progress.EnsureStorySupplies();
    assert(progress.potions == 5 && progress.manaPotions == 5);
    progress.potions = 8;
    progress.EnsureStorySupplies();
    assert(progress.potions == 8);
    progress.coins = 44;
    assert(progress.BuyPotion(false));
    assert(progress.coins == 24 && progress.potions == 9);
    assert(!progress.BuyPotion(true));
    assert(progress.coins == 24 && progress.manaPotions == 5);
    progress.coins = 25;
    assert(progress.BuyPotion(true));
    assert(progress.coins == 0 && progress.manaPotions == 6);
    progress.coins = 100;
    progress.potions = 99;
    assert(!progress.BuyPotion(false));
    assert(progress.coins == 100 && progress.potions == 99);

    MainQuest::State state;
    state.active = true;
    state.step = MainQuest::Step::Shadows;
    MainQuest::Combat combat;
    combat.Enter(state);
    combat.enemies.clear();
    MainQuest::Point player{0, 0};
    assert(combat.Attack(player, {1, 0}, state));
    MainQuest::Enemy enemy;
    enemy.position = {2, .75f};
    enemy.attackMotion = 100;
    combat.enemies.push_back(enemy);
    for (int i = 0; i < 10; ++i)
    {
        combat.Update(.05f, player, state,
                      [](MainQuest::Point)
                      {
                          return true;
                      });
    }
    assert(combat.enemies.front().health < 24); // A glancing shot now connects.

    combat.Enter(state);
    combat.enemies.resize(1);
    auto& blockedEnemy = combat.enemies.front();
    blockedEnemy.position = {2, .5f};
    blockedEnemy.attackMotion = 100;
    const float blockedHealth = blockedEnemy.health;
    assert(combat.Attack(player, {1, 0}, state));
    assert(combat.castDirection.x == 1 && combat.castDirection.y == 0);
    for (int i = 0; i < 10; ++i)
    {
        combat.Update(.05f, player, state,
                      [](MainQuest::Point p)
                      {
                          return p.x < 1;
                      });
    }
    assert(combat.shots.empty() && blockedEnemy.health == blockedHealth);
}
