// Standalone logic tests; link RpgProgress.cpp and RuntimeFiles.cpp as for MainQuestTests.
#include <cassert>
#include <algorithm>
#include "../SimpleGame/QuestTwoStory.h"

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
        assert(state.ClueCount() == 3 && state.step == Step::Shadows);
        assert(!state.Inspect(0));
        assert(!state.Advance(Event::Interact));
        assert(state.Advance(Event::Defeat));
        assert(state.step == Step::Gym && !state.Gym());
        assert(state.Advance(Event::Interact));
        assert(state.step == Step::Elite && state.Gym());
        assert(state.Advance(Event::Defeat));
        assert(state.Advance(Event::Interact));
        assert(state.fragments == 5 && state.step == Step::Report);
        assert(state.Advance(Event::Interact));
        assert(state.step == Step::Complete);
        assert(state.progress.totalExperience == 300 && state.progress.coins == 250);
        assert(state.progress.crystals == 1);
        assert(!state.Advance(Event::Interact));
        assert(!state.Advance(Event::Defeat));
        assert(state.progress.coins == 250);
        assert(state.OpenSeal());
        assert(!state.OpenSeal());
        state.progress.manaPotions = 98;
        assert(state.TakeTreasure());
        assert(!state.TakeTreasure());
        assert(state.progress.crystals == 3 && state.progress.manaPotions == 99);
    } while (std::next_permutation(order.begin(), order.end()));
}
