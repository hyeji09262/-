#pragma once
#include <algorithm>

// Shared controls: six wand charges, manual 1.2-second reload, non-evasive jump.
struct PlayerActions
{
    int charges = 6;
    float reload = 0;
    float height = 0;
    float velocity = 0;

    void Jump()
    {
        if (height == 0 && velocity == 0)
        {
            velocity = 180;
        }
    }

    void Reload()
    {
        if (charges < 6 && reload <= 0)
        {
            reload = 1.2f;
        }
    }

    bool Fire()
    {
        if (reload > 0 || charges <= 0)
        {
            return false;
        }
        --charges;
        return true;
    }

    void Update(float dt)
    {
        if (reload > 0)
        {
            reload = (std::max)(0.f, reload - dt);
            if (reload == 0)
            {
                charges = 6;
            }
        }
        if (height > 0 || velocity != 0)
        {
            velocity -= 480 * dt;
            height = (std::max)(0.f, height + velocity * dt);
            if (height == 0)
            {
                velocity = 0;
            }
        }
    }
};
