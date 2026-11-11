#pragma once
#include <algorithm>

// Shared controls: mana-powered attacks, manual 1.2-second recovery, non-evasive jump.
struct PlayerActions
{
    static constexpr float AttackManaCost = 4.f;
    static constexpr float RecoveryDuration = .65f;
    float reload = 0;
    float height = 0;
    float velocity = 0;
    float recoveryFlash = 0;
    bool manaRecovery = false;

    void MarkRecovery(bool magic)
    {
        recoveryFlash = RecoveryDuration;
        manaRecovery = magic;
    }

    bool UseManaPotion(float& mana, float maximum, int& potions)
    {
        if (potions <= 0 || mana >= maximum)
        {
            return false;
        }
        --potions;
        mana = maximum;
        reload = 0;
        MarkRecovery(true);
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
        recoveryFlash = (std::max)(0.f, recoveryFlash - dt);
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
