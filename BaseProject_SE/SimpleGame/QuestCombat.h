#pragma once
#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>
#include "MainQuest.h"
#include "CharacterVisual.h"
#include "Profiler.h"
#include "PlayerActions.h"

namespace MainQuest
{
struct Point
{
    float x = 0, y = 0;
};

inline float Distance(Point a, Point b)
{
    return std::hypot(a.x - b.x, a.y - b.y);
}

inline Point Direction(Point a, Point b)
{
    const float length = (std::max)(.001f, Distance(a, b));
    return {(b.x - a.x) / length, (b.y - a.y) / length};
}

struct Enemy
{
    Point position;
    Point aim;
    float health = 24;
    float timer = 0;
    int phase = 0;
    int pattern = 0;
    bool boss = false;
};

struct Shot
{
    Point position, direction;
    float age = 0;
    float damage = 10;
    bool mirror = false;
    unsigned id = 0;
    float launchHeight = 0;
};

class Combat
{
  public:
    std::vector<Enemy> enemies;
    std::vector<Shot> shots;
    float health = 60, mana = 30;
    float hurt = 0, cast = 0, attackCooldown = 0;
    float burst = 0, burstCooldown = 0, comboTime = 0;
    Point burstOrigin;
    int combo = 0;
    unsigned nextShot = 1;
    bool castMirror = false;
    PlayerActions actions;

    bool Dead() const
    {
        return health <= 0;
    }

    void Enter(State& quest)
    {
        enemies.clear();
        actions = PlayerActions{};
        shots.clear();
        hurt = cast = attackCooldown = burst = burstCooldown = 0;
        combo = 0;
        comboTime = 0;
        health = float(quest.progress.stats.maxHealth);
        mana = float(quest.progress.stats.maxMana);
        if (quest.step == Step::Escape || quest.step == Step::Shadows)
        {
            quest.defeated = 0;
            const float start = quest.step == Step::Escape ? 18.f : 9.f;
            for (int i = 0; i < 3; ++i)
            {
                Enemy enemy;
                enemy.position = {start + i * 1.5f, 7.f + i};
                enemies.push_back(enemy);
            }
        }
        else if (quest.step == Step::Boss)
        {
            Enemy boss;
            boss.position = {15, 8};
            boss.health = 160;
            boss.boss = true;
            enemies.push_back(boss);
        }
    }

    void HitPlayer(float damage)
    {
        if (hurt <= 0 && !Dead())
        {
            health = (std::max)(0.f, health - damage);
            hurt = .7f;
        }
    }

    void Damage(Enemy& enemy, float damage, State& quest)
    {
        if (enemy.health <= 0)
        {
            return;
        }
        enemy.health -= damage;
        if (enemy.health <= 0)
        {
            quest.Emit(Event::Defeat);
            Performance::Profiler::Get().Count("quest.enemy_deaths");
        }
    }

    void Attack(Point player, bool mirror, State& quest, float height = 0)
    {
        if (!quest.CanFight() || Dead() || attackCooldown > 0 ||
            (quest.step != Step::Shadows && quest.step != Step::Boss))
        {
            return;
        }
        if (!actions.Fire(mana))
        {
            return;
        }
        Point aim = {player.x + (mirror ? -1.f : 1.f), player.y};
        float nearest = 9;
        for (const auto& enemy : enemies)
        {
            const float distance = Distance(player, enemy.position);
            if (enemy.health > 0 && distance < nearest)
            {
                aim = enemy.position;
                nearest = distance;
            }
        }
        combo = comboTime > 0 ? (combo + 1) % 3 : 0;
        comboTime = .9f;
        cast = CharacterVisual::CastDuration;
        castMirror = mirror;
        attackCooldown = .28f;
        shots.push_back({player, Direction(player, aim), 0,
                         quest.progress.stats.attack * (combo == 2 ? 1.5f : 1.f), mirror,
                         nextShot++, height});
    }

    bool UsePotion(State& quest)
    {
        if (!quest.active || Dead() || quest.progress.potions <= 0 ||
            health >= quest.progress.stats.maxHealth)
        {
            return false;
        }
        --quest.progress.potions;
        health = (std::min)(float(quest.progress.stats.maxHealth),
                            health + quest.progress.stats.maxHealth * .45f);
        Performance::Profiler::Get().Count("quest.potions_used");
        return true;
    }

    void Burst(Point player, State& quest)
    {
        if (!quest.CanFight() || Dead() || actions.reload > 0 || mana < 10 || burstCooldown > 0 ||
            (quest.step != Step::Shadows && quest.step != Step::Boss))
        {
            return;
        }
        mana -= 10;
        burst = .5f;
        burstCooldown = 2;
        burstOrigin = player;
        for (auto& enemy : enemies)
        {
            if (Distance(enemy.position, player) <= 3)
            {
                Damage(enemy, float(quest.progress.stats.attack * 3), quest);
            }
        }
    }

    using Collision = std::function<bool(Point)>;

    static void Move(Point& p, Point direction, float distance, const Collision& walkable)
    {
        const int steps = (std::max)(1, int(std::ceil(distance / .12f)));
        for (int i = 0; i < steps; ++i)
        {
            Point next = {p.x + direction.x * distance / steps,
                          p.y + direction.y * distance / steps};
            if (walkable({next.x, p.y}))
            {
                p.x = next.x;
            }
            if (walkable({p.x, next.y}))
            {
                p.y = next.y;
            }
        }
    }

    void Update(float dt, Point& player, State& quest, const Collision& walkable)
    {
        Performance::Scope scope("cpu.quest.combat_update_ms");
        if (Dead())
        {
            return;
        }
        hurt -= dt;
        actions.Update(dt, mana, float(quest.progress.stats.maxMana));
        cast -= dt;
        attackCooldown -= dt;
        burstCooldown -= dt;
        burst -= dt;
        comboTime -= dt;
        for (auto& shot : shots)
        {
            const int substeps =
                (std::max)(1, int(std::ceil(CharacterVisual::SpellSpeed * dt / .12f)));
            for (int i = 0; i < substeps && shot.age < 1; ++i)
            {
                shot.position.x += shot.direction.x * CharacterVisual::SpellSpeed * dt / substeps;
                shot.position.y += shot.direction.y * CharacterVisual::SpellSpeed * dt / substeps;
                shot.age += dt / substeps;
                if (!walkable(shot.position))
                {
                    shot.age = 1;
                    break;
                }
                for (auto& enemy : enemies)
                {
                    if (enemy.health > 0 &&
                        Distance(shot.position, enemy.position) < (enemy.boss ? 1.f : .6f))
                    {
                        Damage(enemy, shot.damage, quest);
                        shot.age = 1;
                        break;
                    }
                }
            }
        }
        shots.erase(std::remove_if(shots.begin(), shots.end(),
                                   [](const Shot& shot)
                                   {
                                       return shot.age >= 1;
                                   }),
                    shots.end());
        for (auto& enemy : enemies)
        {
            if (enemy.health <= 0 || Dead())
            {
                continue;
            }
            enemy.timer -= dt;
            const float distance = Distance(player, enemy.position);
            if (!enemy.boss)
            {
                Move(enemy.position, Direction(enemy.position, player), 2.1f * dt, walkable);
                if (distance < .8f)
                {
                    HitPlayer(6);
                }
                continue;
            }
            if (enemy.phase == 0)
            {
                if (distance > 4)
                {
                    Move(enemy.position, Direction(enemy.position, player), 1.7f * dt, walkable);
                }
                else if (enemy.timer <= 0)
                {
                    enemy.aim = player;
                    enemy.phase = 1;
                    enemy.timer = 1.1f;
                }
            }
            else if (enemy.phase == 1 && enemy.timer <= 0)
            {
                if (enemy.pattern == 1)
                {
                    enemy.phase = 2;
                    enemy.timer = .5f;
                }
                else
                {
                    if (enemy.pattern == 2 && walkable(enemy.aim))
                    {
                        enemy.position = enemy.aim;
                    }
                    if (Distance(player, enemy.pattern == 2 ? enemy.aim : enemy.position) <
                        (enemy.pattern == 2 ? 2.f : 2.4f))
                    {
                        HitPlayer(15);
                    }
                    enemy.phase = 3;
                    enemy.timer = 1.3f;
                }
            }
            else if (enemy.phase == 2)
            {
                Move(enemy.position, Direction(enemy.position, enemy.aim), 9 * dt, walkable);
                if (Distance(player, enemy.position) < 1.2f)
                {
                    HitPlayer(12);
                }
                if (enemy.timer <= 0)
                {
                    enemy.phase = 3;
                    enemy.timer = 1.3f;
                }
            }
            else if (enemy.phase == 3 && enemy.timer <= 0)
            {
                enemy.pattern = (enemy.pattern + 1) % 3;
                enemy.phase = 0;
            }
        }
        Performance::Profiler::Get().Gauge("quest.projectiles_alive", double(shots.size()));
    }
};
} // namespace MainQuest
