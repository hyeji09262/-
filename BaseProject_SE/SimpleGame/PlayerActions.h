#pragma once
#include <algorithm>

// Shared controls: mana-powered attacks, manual 1.2-second recovery, non-evasive jump.
struct PlayerActions
{
    static constexpr float AttackManaCost = 4.f;
    float reload = 0;
    float height = 0;
    float velocity = 0;

    bool UseManaPotion(float& mana, float maximum, int& potions)
    {
        if (potions <= 0 || mana >= maximum)
        {
            return false;
        }
        --potions;
        mana = maximum;
        reload = 0;
        return true;
    }

    void Jump()
    {
        if (height == 0 && velocity == 0)
        {
            velocity = 180;
        }
    }

    void Reload(float mana, float maximum)
    {
        if (mana < maximum && reload <= 0)
        {
            reload = 1.2f;
        }
    }

    bool Fire(float& mana)
    {
        if (reload > 0 || mana < AttackManaCost)
        {
            return false;
        }
        mana -= AttackManaCost;
        return true;
    }

    void Update(float dt, float& mana, float maximum)
    {
        if (reload > 0)
        {
            reload = (std::max)(0.f, reload - dt);
            if (reload == 0)
            {
                mana = maximum;
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
