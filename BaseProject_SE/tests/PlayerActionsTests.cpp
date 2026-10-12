#include <cassert>
#include "../SimpleGame/PlayerActions.h"

int main()
{
    PlayerActions actions;
    actions.Reload();
    assert(actions.reload == 0);
    for (int i = 0; i < 6; ++i)
    {
        assert(actions.Fire());
    }
    assert(!actions.Fire());
    actions.Reload();
    assert(!actions.Fire());
    actions.Update(.6f);
    actions.Reload();
    assert(actions.reload <= .61f);
    actions.Update(.7f);
    assert(actions.charges == 6 && actions.reload == 0);
    actions.Jump();
    actions.Update(.05f);
    assert(actions.height > 0);
    const float velocity = actions.velocity;
    actions.Jump();
    assert(actions.velocity == velocity);
    for (int i = 0; i < 20; ++i)
    {
        actions.Update(.05f);
    }
    assert(actions.height == 0 && actions.velocity == 0);
}
