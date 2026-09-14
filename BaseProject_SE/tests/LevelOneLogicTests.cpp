// Optional standalone logic checks. Not part of the game executable.
#include "../SimpleGame/HuntingMap.h"
#include "../SimpleGame/RpgProgress.h"
#include <iostream>
#include <stdexcept>

void Require(bool condition, const char* label)
{
    if (!condition)
    {
        throw std::runtime_error(label);
    }
}

int main()
{
    Rpg::Progress progress;
    Require(!progress.Award(39), "39 XP remains level 1");
    Require(progress.Award(1), "40 XP reaches level 2");
    Require(progress.stats.level == 2 && progress.stats.attack == 13 &&
                progress.stats.maxHealth == 72 && progress.stats.maxMana == 35 &&
                progress.stats.defense == 2,
            "level 2 stats");
    Require(progress.ExperienceInLevel() == 0, "level boundary remainder");
    progress.Award(140);
    Require(progress.stats.level == 4 && progress.totalExperience == 180,
            "multi-level reward and cumulative XP");
    progress.Award(-10);
    Require(progress.totalExperience == 180, "negative reward ignored");
    progress.Award(1000000000);
    Require(progress.stats.level == Rpg::LevelCap &&
                progress.totalExperience == Rpg::ExperienceLimit,
            "level and XP caps");

    for (unsigned seed = 0; seed < 256; ++seed)
    {
        Hunting::Map map;
        map.Generate(seed);
        Require(map.AllFloorConnected(), "all floor cells connected");
        Require(map.Walkable(map.camp), "camp clearance");
        Require(!map.Walkable({0, 0}), "outer boundary blocked");
        Require(map.SpawnLocations().size() >= 24, "enough spawn candidates");
        for (auto position : map.SpawnLocations())
        {
            Require(map.Walkable(position) && !map.Safe(position), "spawn clearance");
        }

        // Verify cardinal connections with the same finite-radius movement used by the player.
        for (int y = 1; y < Hunting::Map::Height - 1; ++y)
        {
            for (int x = 1; x < Hunting::Map::Width - 1; ++x)
            {
                if (!map.Floor(x, y))
                {
                    continue;
                }
                for (auto delta : {Hunting::Position{1, 0}, Hunting::Position{0, 1}})
                {
                    if (!map.Floor(x + static_cast<int>(delta.x), y + static_cast<int>(delta.y)))
                    {
                        continue;
                    }
                    Hunting::Position start = {x + .5f, y + .5f};
                    Hunting::Position expected = {start.x + delta.x, start.y + delta.y};
                    map.Move(start, delta);
                    Require(Hunting::Distance(start, expected) < .01f,
                            "connected passage supports player radius");
                }
            }
        }
    }
    std::cout << "Level-one map and progression checks passed.\n";
}
