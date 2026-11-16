// Optional standalone logic checks. Not part of the game executable.
#include "../SimpleGame/HuntingMap.h"
#include "../SimpleGame/RpgProgress.h"
#include <algorithm>
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
    Rpg::Progress story;
    story.Award(Rpg::AlleyQuestExperience);
    story.Award(Rpg::SchoolQuestExperience);
    story.Award(Rpg::MiaQuestExperience);
    Require(story.stats.level == 1, "story before the park stays at level 1");
    story.Award(Rpg::ParkQuestExperience);
    story.coins = 430;
    Rpg::Progress hunting;
    hunting.SyncGrowth(story);
    Require(hunting.stats.level == 2 && hunting.totalExperience == 45 && hunting.coins == 430,
            "park completion carries level 2 and gold into hunting");

    Rpg::Progress progress;
    Require(!progress.Award(39), "39 XP remains level 1");
    Require(progress.Award(1), "40 XP reaches level 2");
    Require(progress.stats.level == 2 && progress.stats.attack == 13 &&
                progress.stats.maxHealth == 66 && progress.stats.maxMana == 33 &&
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
        Require(map.Safe(map.VillageExit()) && map.Walkable(map.VillageExit()),
                "village exit is in the safe clearing");
        for (int y = static_cast<int>(map.VillageExit().y); y <= 41; ++y)
        {
            const int x = static_cast<int>(map.camp.x);
            Require(map.VillageTrail(x, y) && map.Floor(x, y), "village trail is clear");
        }
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
    for (int room = -1; room < 6; ++room)
    {
        Hunting::Map map;
        map.GenerateHuntingRoom(42, room);
        const auto floorCount = std::count(map.tiles.begin(), map.tiles.end(), Hunting::Tile::Ground);
        Require(floorCount == (room < 0 ? 440 : 1520), "fourfold walkable area");
        Require(map.AllFloorConnected(), "hunting room is connected");
        Require(map.Walkable(map.VillageExit()), "hunting room exit is reachable");
        if (room >= 0)
        {
            Require(map.SpawnLocations().size() >= 8, "hunting room has spawn space");
        }
    }
    std::cout << "Level-one map and progression checks passed.\n";
}
