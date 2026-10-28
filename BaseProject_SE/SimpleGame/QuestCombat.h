#pragma once
#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>
#include "MainQuest.h"
#include "CharacterVisual.h"
#include "Profiler.h"
#include "PlayerActions.h"
#include "AutoTarget.h"
#include "MonsterSkills.h"
#include "MonsterAnimation.h"

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
    float attackMotion = 0;
    int kind = 1;
    Point facing{.70710678f, -.70710678f};
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
    Point castDirection{.70710678f, -.70710678f};
    bool variedSkills = false;
    std::vector<MonsterSkills::Projectile> hostileShots;
    std::vector<MonsterSkills::Poison> poisonZones;
    PlayerActions actions;

    bool Dead() const
    {
        return health <= 0;
    }

    void Enter(State& quest)
    {
        enemies.clear();
        hostileShots.clear();
        poisonZones.clear();
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

    bool Attack(Point player, Point facing, State& quest, float height = 0,
                const std::function<bool(Point)>& walkable = {})
    {
        if (!quest.CanFight() || Dead() || attackCooldown > 0 ||
            (quest.step != Step::Shadows && quest.step != Step::Boss))
        {
            return false;
        }
        if (!actions.Fire(mana))
        {
            return false;
        }
        AutoTarget::Selection aim(player.x, player.y, facing.x, facing.y, 9.f);
        {
            Performance::Scope scope("cpu.combat.auto_target_ms");
            for (const auto& enemy : enemies)
            {
                aim.Consider(enemy.position.x, enemy.position.y, enemy.health > 0,
                             !walkable || ClearPath(player, enemy.position, walkable));
            }
        }
        castDirection = {aim.directionX, aim.directionY};
        combo = comboTime > 0 ? (combo + 1) % 3 : 0;
        comboTime = .9f;
        cast = CharacterVisual::CastDuration;
        castMirror = castDirection.x - castDirection.y < 0;
        attackCooldown = .28f;
        shots.push_back({player, castDirection, 0,
                         quest.progress.stats.attack * (combo == 2 ? 1.5f : 1.f), castMirror,
                         nextShot++, height});
        Performance::Profiler::Get().Count("combat.basic_attacks_count");
        return true;
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

    static bool ClearPath(Point from, Point to, const Collision& walkable)
    {
        const int steps = (std::max)(1, int(std::ceil(Distance(from, to) / .2f)));
        for (int i = 1; i <= steps; ++i)
        {
            const float t = float(i) / steps;
            if (!walkable({from.x + (to.x - from.x) * t, from.y + (to.y - from.y) * t}))
            {
                return false;
            }
        }
        return true;
    }

    void EnemySpell(const Enemy& enemy, int level, int defense)
    {
        const auto skill = MonsterSkills::Get(enemy.kind);
        const int damage = (std::max)(1, 4 + level * 2 - defense);
        if (skill.style == MonsterSkills::Style::Poison)
        {
            if (poisonZones.size() < 12)
            {
                poisonZones.push_back({{enemy.aim.x, enemy.aim.y}, damage, nextShot++});
            }
        }
        else if (hostileShots.size() < 46)
        {
            const int count = skill.style == MonsterSkills::Style::Fan ? 3 : 1;
            for (int i = 0; i < count; ++i)
            {
                const float angle = count == 1 ? 0.f : (i - 1) * .24f;
                const float c = std::cos(angle), s = std::sin(angle);
                hostileShots.push_back({{enemy.position.x, enemy.position.y},
                                        {enemy.facing.x * c - enemy.facing.y * s,
                                         enemy.facing.x * s + enemy.facing.y * c},
                                        damage,
                                        nextShot++});
            }
        }
        Performance::Profiler::Get().Count("quest.enemy_skills_cast_count");
    }

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
                        Distance(shot.position, enemy.position) < (enemy.boss ? 1.35f : .85f))
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
                if (variedSkills)
                {
                    const auto skill = MonsterSkills::Get(enemy.kind);
                    const bool sight = ClearPath(enemy.position, player, walkable);
                    if (MonsterAnimation::Advance(enemy.attackMotion, dt))
                    {
                        EnemySpell(enemy, quest.progress.stats.level, quest.progress.stats.defense);
                    }
                    if (enemy.attackMotion <= 0)
                    {
                        if (distance > skill.range * .8f || !sight)
                        {
                            const Point before = enemy.position;
                            Move(enemy.position, Direction(enemy.position, player), 1.7f * dt,
                                 walkable);
                            if (Distance(before, enemy.position) > .001f)
                            {
                                enemy.facing = Direction(before, enemy.position);
                            }
                        }
                        if (distance < skill.range && enemy.timer <= 0 && sight)
                        {
                            enemy.aim = player;
                            enemy.facing = Direction(enemy.position, player);
                            enemy.attackMotion = MonsterAnimation::Duration;
                            enemy.timer = skill.cooldown;
                        }
                    }
                    continue;
                }
                if (MonsterAnimation::Advance(enemy.attackMotion, dt) && distance < 1.1f)
                {
                    HitPlayer(6);
                }
                if (enemy.attackMotion <= 0)
                {
                    const Point before = enemy.position;
                    Move(enemy.position, Direction(enemy.position, player), 2.1f * dt, walkable);
                    if (Distance(before, enemy.position) > .001f)
                    {
                        enemy.facing = Direction(before, enemy.position);
                    }
                    if (distance < .9f && enemy.timer <= 0)
                    {
                        enemy.facing = Direction(enemy.position, player);
                        enemy.attackMotion = MonsterAnimation::Duration;
                        enemy.timer = 1.4f;
                    }
                }
                continue;
            }
            if (enemy.phase == 0)
            {
                if (distance > 4)
                {
                    enemy.facing = Direction(enemy.position, player);
                    Move(enemy.position, Direction(enemy.position, player), 1.7f * dt, walkable);
                }
                else if (enemy.timer <= 0)
                {
                    enemy.aim = player;
                    enemy.facing = Direction(enemy.position, player);
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
                // Keep the windup direction after passing the target; never reverse mid-charge.
                Move(enemy.position, enemy.facing, 9 * dt, walkable);
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
        if (variedSkills)
        {
            Performance::Scope skillScope("cpu.quest.enemy_skills_update_ms");
            hostileShots.erase(std::remove_if(hostileShots.begin(), hostileShots.end(),
                                              [&](MonsterSkills::Projectile& shot)
                                              {
                                                  return !MonsterSkills::Advance(
                                                      shot, dt,
                                                      [&](Hunting::Position p)
                                                      {
                                                          return !walkable({p.x, p.y});
                                                      },
                                                      [&](Hunting::Position p)
                                                      {
                                                          if (Distance({p.x, p.y}, player) < .4f)
                                                          {
                                                              HitPlayer(float(shot.damage));
                                                              return true;
                                                          }
                                                          return false;
                                                      });
                                              }),
                               hostileShots.end());
            for (auto& poison : poisonZones)
            {
                if (poison.Advance(dt) &&
                    Distance({poison.position.x, poison.position.y}, player) <
                        MonsterSkills::Poison::Radius &&
                    ClearPath({poison.position.x, poison.position.y}, player, walkable))
                {
                    HitPlayer(float(poison.damage));
                }
            }
            poisonZones.erase(std::remove_if(poisonZones.begin(), poisonZones.end(),
                                             [](const MonsterSkills::Poison& poison)
                                             {
                                                 return poison.age >=
                                                        MonsterSkills::Poison::Duration;
                                             }),
                              poisonZones.end());
            Performance::Profiler::Get().Gauge("quest.enemy_projectiles_count",
                                               double(hostileShots.size()));
            Performance::Profiler::Get().Gauge("quest.enemy_poison_zones_count",
                                               double(poisonZones.size()));
        }
        Performance::Profiler::Get().Gauge("quest.projectiles_alive", double(shots.size()));
    }
};
} // namespace MainQuest
