#pragma once
#include <cstdint>
#include <algorithm>

namespace Rpg
{
constexpr int LevelCap = 30;
constexpr std::int64_t ExperienceLimit = 1000000000;

struct Stats
{
    int level = 1;
    int maxHealth = 60;
    int maxMana = 30;
    int attack = 10;
    int defense = 1;
};

class Progress
{
  public:
    std::int64_t totalExperience = 0;
    int coins = 0;
    int potions = 5;
    int manaPotions = 5;
    int crystals = 0;
    Stats stats;

    void EnsureStorySupplies()
    {
        potions = (std::max)(5, potions);
        manaPotions = (std::max)(5, manaPotions);
    }

    bool BuyPotion(bool magic)
    {
        const int price = magic ? 25 : 20;
        int& quantity = magic ? manaPotions : potions;
        if (coins < price || quantity >= 99)
        {
            return false;
        }
        coins -= price;
        ++quantity;
        return true;
    }

    static int RequiredExperience(int level);
    std::int64_t LevelStart() const;
    int ExperienceInLevel() const;
    bool Award(int amount);
    void Recalculate();
    bool Load();
    bool Save() const;
};
} // namespace Rpg
