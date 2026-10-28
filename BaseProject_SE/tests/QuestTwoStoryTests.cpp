// Standalone logic tests; link RpgProgress.cpp and RuntimeFiles.cpp as for MainQuestTests.
#include <cassert>
#include <algorithm>
#include <queue>
#include "../SimpleGame/QuestTwoStory.h"
#include "../SimpleGame/SchoolLayout.h"

int main()
{
    using MainQuest::Event;
    using QuestTwo::Step;
    std::array<unsigned, 3> order{{0, 1, 2}};
    do
    {
        QuestTwo::StoryState state;
        state.progress.totalExperience = 120;
        state.progress.coins = 100;
        state.progress.Recalculate();
        assert(!state.Inspect(0));
        assert(!state.Advance(Event::Defeat));
        assert(!state.OpenSeal());
        assert(!state.TakeTreasure());
        assert(state.Advance(Event::Interact));
        assert(state.step == Step::Investigate);
        assert(!state.Advance(Event::Interact));
        assert(!state.Inspect(3));
        assert(state.Inspect(order[0]));
        assert(!state.Inspect(order[0]));
        assert(state.ClueCount() == 1);
        assert(state.Inspect(order[1]));
        assert(state.Inspect(order[2]));
        assert(state.ClueCount() == 3 && state.step == Step::Gym);
        assert(!state.ReadyToFight(1, false));
        assert(!state.ReadyToFight(3, false));
        assert(!state.ReadyToFight(0, true));
        assert(!state.ReadyToFight(0, false));
        assert(!state.Inspect(0));
        assert(!state.Advance(Event::Defeat));
        assert(state.step == Step::Gym && !state.Gym());
        assert(state.Advance(Event::Interact));
        assert(state.step == Step::Elite && state.Gym());
        assert(!state.ReadyToFight(0, false));
        assert(!state.ReadyToFight(2, true));
        assert(state.ReadyToFight(2, false));
        assert(state.Advance(Event::Defeat));
        assert(state.fragments == 5);
        assert(state.step == Step::Complete);
        assert(std::string(state.Current().id) == "MQ002_COMPLETE");
        assert(state.progress.totalExperience == 300 && state.progress.coins == 250);
        assert(state.progress.crystals == 0);
        assert(state.progress.potions == 7 && state.progress.manaPotions == 7);
        assert(!state.Advance(Event::Interact));
        assert(!state.Advance(Event::Defeat));
        assert(state.progress.coins == 250);
        assert(!state.OpenSeal());
        assert(!state.TakeTreasure());
        state.Restart();
        assert(state.step == Step::Reaction && state.clues == 0);
        assert(state.rewardClaimed && state.progress.coins == 250);
        assert(state.Advance(Event::Interact));
        for (unsigned i : order)
        {
            assert(state.Inspect(i));
        }
        assert(state.Advance(Event::Interact));
        assert(state.Advance(Event::Defeat));
        assert(state.progress.totalExperience == 300 && state.progress.coins == 250);
        assert(state.fragments == 5 && state.progress.potions == 7);
    } while (std::next_permutation(order.begin(), order.end()));
    for (const auto& site : QuestTwo::Investigations())
    {
        assert(SchoolLayout::Walkable(site.room, site.x, site.y));
    }
    {
        constexpr int width = 64, height = 52;
        std::array<bool, width * height> seen{};
        std::queue<std::pair<int, int>> pending;
        pending.push({32, 44});
        seen[44 * width + 32] = true;
        const int steps[][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        while (!pending.empty())
        {
            const auto p = pending.front();
            pending.pop();
            for (const auto& step : steps)
            {
                const int x = p.first + step[0], y = p.second + step[1];
                if (x < 0 || x >= width || y < 0 || y >= height || seen[y * width + x] ||
                    !SchoolLayout::Walkable(2, x * .5f, y * .5f))
                {
                    continue;
                }
                seen[y * width + x] = true;
                pending.push({x, y});
            }
        }
        assert(seen[16 * width + 30]); // Boss.
        assert(seen[49 * width + 32]); // Gym exit.
    }
}
