#pragma once
#include <cstdint>

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
    int potions = 3;
    int crystals = 0;
    Stats stats;

    static int RequiredExperience(int level);
    std::int64_t LevelStart() const;
    int ExperienceInLevel() const;
    bool Award(int amount);
    void Recalculate();
    bool Load();
    bool Save() const;
};
} // namespace Rpg
