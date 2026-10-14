#include <cassert>
#include "../SimpleGame/PlayerActions.h"

int main()
{
    PlayerActions actions;
    float potionMana = 10;
    int manaPotions = 3;
    assert(actions.UseManaPotion(potionMana, 30, manaPotions));
    assert(potionMana == 30 && manaPotions == 2);
    assert(!actions.UseManaPotion(potionMana, 30, manaPotions));
    assert(manaPotions == 2);
    potionMana = 0;
    manaPotions = 0;
    assert(!actions.UseManaPotion(potionMana, 30, manaPotions));
    assert(potionMana == 0);
    float mana = 30;
    actions.Reload(mana, 30);
    assert(actions.reload == 0);
    for (int i = 0; i < 7; ++i)
    {
        assert(actions.Fire(mana));
    }
    assert(mana == 2);
    assert(!actions.Fire(mana));
    actions.Reload(mana, 30);
    assert(!actions.Fire(mana));
    actions.Update(.6f, mana, 30);
    assert(mana == 2);
    actions.Reload(mana, 30);
    assert(actions.reload <= .61f);
    actions.Update(.7f, mana, 30);
    assert(mana == 30 && actions.reload == 0);
    assert(actions.Fire(mana));
    actions.Reload(mana, 45);
    actions.Update(1.3f, mana, 45);
    assert(mana == 45);
    actions.Jump();
    actions.Update(.05f, mana, 45);
    assert(actions.height > 0);
    const float velocity = actions.velocity;
    actions.Jump();
    assert(actions.velocity == velocity);
    for (int i = 0; i < 20; ++i)
    {
        actions.Update(.05f, mana, 45);
    }
    assert(actions.height == 0 && actions.velocity == 0);
}
