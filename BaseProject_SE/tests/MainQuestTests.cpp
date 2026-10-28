// Standalone tests: link RpgProgress.cpp and RuntimeFiles.cpp, without the game's main.
#include <cassert>
#include <cstring>
#include "../SimpleGame/QuestCombat.h"

int main()
{
    using namespace MainQuest;
    State quest;
    assert(!quest.FinishEpilogue());
    assert(!quest.Emit(Event::Interact));
    quest.active = true;
    assert(!quest.Emit(Event::Defeat));
    while (quest.step != Step::Shadows)
    {
        assert(!quest.CanFight());
        assert(quest.Emit(quest.Current().event));
    }
    assert(quest.fragments == 1 && quest.CanFight());
    Combat combat;
    combat.Enter(quest);
    assert(combat.enemies.size() == 3);
    combat.Damage(combat.enemies[0], 100, quest);
    combat.Damage(combat.enemies[0], 100, quest);
    assert(quest.defeated == 1 && quest.step == Step::Shadows);
    combat.Damage(combat.enemies[1], 100, quest);
    combat.Damage(combat.enemies[2], 100, quest);
    assert(quest.step == Step::Boss);
    combat.Enter(quest);
    assert(combat.enemies.size() == 1 && combat.enemies[0].boss);
    combat.Damage(combat.enemies[0], 200, quest);
    assert(quest.step == Step::CheckYuna);
    assert(quest.Emit(Event::Interact));
    assert(quest.Emit(Event::Interact));
    assert(quest.step == Step::Complete && quest.rewardClaimed);
    assert(quest.fragments == 4 && quest.progress.totalExperience == 120 &&
           quest.progress.coins == 100);
    assert(std::strcmp(quest.NextQuestId(), "MQ_002_SOMETHING_IN_SCHOOL") == 0);
    assert(!quest.Emit(Event::Interact) && !quest.Emit(Event::Defeat));
    assert(quest.progress.coins == 100);
    assert(quest.FinishEpilogue());
    assert(!quest.FinishEpilogue());
    assert(std::strcmp(quest.ActiveQuestId(), "MQ_002_SOMETHING_IN_SCHOOL") == 0);
    assert(quest.progress.coins == 100 && quest.fragments == 4);

    State escape;
    escape.active = true;
    escape.step = Step::Escape;
    combat.Enter(escape);
    combat.Attack({4, 8}, {.70710678f, -.70710678f}, escape);
    combat.Burst({4, 8}, escape);
    assert(combat.shots.empty() && combat.mana == 30);
    assert(!combat.UsePotion(escape));
    assert(escape.progress.potions == 5);
    combat.health = 0;
    assert(!combat.UsePotion(escape));
    escape.step = Step::Shadows;
    combat.Attack({4, 8}, {.70710678f, -.70710678f}, escape);
    assert(combat.shots.empty());
    combat.Enter(escape);
    combat.HitPlayer(20);
    assert(combat.health == 40);
    assert(combat.UsePotion(escape));
    assert(combat.health == 60 && escape.progress.potions == 4);
    assert(!combat.UsePotion(escape));
    combat.health = 10;
    assert(combat.UsePotion(escape));
    assert(combat.health == 37 && escape.progress.potions == 3);
    escape.progress.potions = 0;
    assert(!combat.UsePotion(escape));
    combat.Enter(escape);
    assert(escape.progress.potions == 0);

    const auto socket = CharacterVisual::WandSocket(true, false, 40);
    assert(std::abs((socket.x - socket.y) * 40 - CharacterVisual::WandX) < .001f);
    Point position{2, 8};
    Combat::Move(position, {-1, 0}, 20,
                 [](Point p)
                 {
                     return p.x >= 1;
                 });
    assert(position.x >= 1);
}
