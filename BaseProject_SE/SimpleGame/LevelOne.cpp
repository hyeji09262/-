#include "stdafx.h"
#include "MagicalTheme.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "GameInput.h"
#include "CompanionFollow.h"
#include "CombatEffects.h"
#include "CombatKeys.h"
#include "Prologue.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <random>
#include <string>
#include <vector>
#include "LevelOne.h"
#include "HuntingMap.h"
#include "PrimitiveModels.h"
#include "RpgProgress.h"
#include "SceneGraph.h"
#include "CharacterVisual.h"
#include "PlayerActions.h"
#include "NatureVisuals.h"
#include "BuildingVisuals.h"
#include "MonsterAnimation.h"
#include "MonsterSkills.h"
#include "AutoTarget.h"

namespace LevelOne
{
namespace
{
using Hunting::Distance;
using Hunting::ModelId;
using Hunting::Position;

struct HuntingRoom
{
    int level;
    int kind;
    const char* name;
    Position portal;
};

constexpr std::array<HuntingRoom, 6> Rooms{{
    {1, 6, "산호 게", {23.5f, 18.5f}},
    {5, 3, "그림자 토끼", {30.5f, 18.5f}},
    {9, 2, "독버섯", {37.5f, 18.5f}},
    {13, 1, "유령 마법사", {23.5f, 28.5f}},
    {17, 4, "마도서", {30.5f, 28.5f}},
    {21, 8, "등불 도깨비", {37.5f, 28.5f}},
}};
int activeRoom = -1;
constexpr float PortalRange = 1.65f;
constexpr float ExitRange = 1.8f;
constexpr float BasicAttackRange = 10.f;
constexpr float BasicAttackHitRadius = .95f;

struct Color
{
    float r, g, b, a;
};

struct Enemy
{
    Position position;
    Position home;
    Position wanderTarget;
    int kind = 0;
    int level = 1;
    int health = 24;
    int maxHealth = 24;
    float attackTimer = 0;
    float respawnTimer = 0;
    float wanderTimer = 0;
    float animation = 0;
    float attackMotion = 0;
    bool nextAttackIsAlternate = false;
    MonsterSkills::Profile pendingSkill{};
    Position facing = {.70710678f, -.70710678f};
    Position skillTarget;
    float chargeRemaining = 0;
    bool chargeHit = false;
};

unsigned int nextVisualId = 0;

struct Drop
{
    Position position;
    int kind;
    int quantity;
    unsigned int visualId = nextVisualId++;
};

struct Projectile
{
    Position position;
    Position direction;
    int damage;
    float lifetime;
    float age = 0;
    unsigned int visualId = nextVisualId++;
    float launchHeight = 0;
    Position wandFacing{};
};

struct FloatingText
{
    Position position;
    std::string label;
    float lifetime;
    Color color;
};

Renderer* graphics = nullptr;
Hunting::Map map;
Hunting::PrimitiveModels models;
Game::SceneGraph scene;
bool castMirror = false;
Rpg::Progress progress;
std::vector<Enemy> enemies;
std::vector<Drop> drops;
std::array<std::vector<Drop>, Rooms.size()> roomDrops;
std::vector<Projectile> projectiles;
std::vector<MonsterSkills::Projectile> hostileShots;
std::vector<MonsterSkills::Poison> poisonZones;
std::vector<FloatingText> floating;
std::array<int, Hunting::Map::Count> navigation;
std::mt19937 random;
Position player;
Position facing = {0.707f, -0.707f};
Position castDirection = {.707f, -.707f};
std::array<bool, 256> keys = {};
int screenWidth = 1280;
int screenHeight = 800;
float health = 60;
float mana = 30;
float time = 0;
float attackTimer = 0;
struct HuntingSpells
{
    float burst = 0, starfall = 0;
    float burstCooldown = 0, starfallCooldown = 0;
    Position burstOrigin{}, starfallOrigin{};
};
HuntingSpells magic;
float damageTimer = 0;
float navigationTimer = 0;
float walkFrame = 0;
float castTimer = 0;
constexpr float LevelUpDuration = 1.6f;
float levelUpTimer = 0;
PlayerActions actions;
float noticeTimer = 0;
bool paused = false;
bool moving = false;
bool initialized = false;
bool worldRequested = false;
std::string notice;
const Color panel = {.045f, .055f, .085f, .95f};
const Color gold = {1.f, .77f, .38f, 1.f};
const Color teal = {.35f, .95f, .85f, 1.f};
const Color white = {.86f, .88f, .91f, 1.f};

Position Project(Position world, float elevation = 0)
{
    return {640 + (world.x - world.y - player.x + player.y) * 32,
            420 + (world.x + world.y - player.x - player.y) * 16 - elevation};
}

bool Visible(Position p, float margin = 100)
{
    return p.x > -margin && p.x < 1280 + margin && p.y > -margin && p.y < 800 + margin;
}

void Triangle(Position a, Position b, Position c, Color color)
{
    graphics->Triangle(a.x * screenWidth / 1280, a.y * screenHeight / 800, b.x * screenWidth / 1280,
                       b.y * screenHeight / 800, c.x * screenWidth / 1280, c.y * screenHeight / 800,
                       color.r, color.g, color.b, color.a);
}

void Box(float x, float y, float width, float height, Color color)
{
    if (color.r < .12f && color.g < .12f && color.b < .2f && color.a > .9f &&
        width > 100 && height > 30)
    {
        MagicalTheme::Panel(*graphics, x * screenWidth / 1280, y * screenHeight / 800,
                            width * screenWidth / 1280, height * screenHeight / 800,
                            (std::min)(screenWidth / 1280.f, screenHeight / 800.f));
        return;
    }

    Triangle({x, y}, {x + width, y}, {x + width, y + height}, color);
    Triangle({x, y}, {x + width, y + height}, {x, y + height}, color);
}

void Diamond(Position p, float width, float height, Color color)
{
    Triangle({p.x - width, p.y}, {p.x, p.y - height}, {p.x + width, p.y}, color);
    Triangle({p.x - width, p.y}, {p.x + width, p.y}, {p.x, p.y + height}, color);
}

void Text(float x, float y, const std::string& label, Color color = white, float size = 18)
{
    color = {MagicalTheme::TextR, MagicalTheme::TextG, MagicalTheme::TextB, color.a};
    graphics->Text(x * screenWidth / 1280, y * screenHeight / 800, label, size * screenHeight / 800,
                   color.r, color.g, color.b, color.a);
}

void Model(ModelId id, Position world, float scale = 1, float elevation = 0, bool mirror = false)
{
    Position screen = Project(world, elevation);
    if (Visible(screen, 140 * scale))
    {
        const Position look = castTimer > 0 ? castDirection : facing;
        const auto& mesh = id == ModelId::Lumi ? models.GetFacing(id, look.x, look.y)
                                               : models.Get(id);
        graphics->DrawModel(
            mesh, screen.x * screenWidth / 1280, screen.y * screenHeight / 800,
            scale * screenWidth / 1280 * (mirror ? -1.f : 1.f), scale * screenHeight / 800);
    }
}

void Effect(Position world, float width, float height, int kind, float elevation = 0,
            float phase = 0)
{
    Position screen = Project(world, elevation);
    if (!Visible(screen))
    {
        return;
    }
    graphics->Effect((screen.x - width * .5f) * screenWidth / 1280,
                     (screen.y - height) * screenHeight / 800, width * screenWidth / 1280,
                     height * screenHeight / 800, time, kind, phase);
}

void Notify(const std::string& text)
{
    notice = text;
    noticeTimer = 5;
}

void FloatText(Position p, const std::string& text, Color color)
{
    if (floating.size() < 40)
    {
        floating.push_back({p, text, 1.2f, color});
    }
}

void ScaleEnemy(Enemy& enemy)
{
    enemy.level = Rooms[activeRoom].level;
    enemy.maxHealth = 24 + (enemy.level - 1) * 8;
}

void NewMap(int room = -1)
{
    Performance::Scope profileScope("cpu.map.generate_level_ms");
    const bool changingRoom = room != activeRoom;
    scene.Clear();
    if (changingRoom && activeRoom >= 0)
    {
        roomDrops[activeRoom].swap(drops);
    }
    activeRoom = room;
    map.GenerateHuntingRoom(random(), room);
    player = map.camp;
    enemies.clear();
    drops.clear();
    if (changingRoom && activeRoom >= 0)
    {
        drops.swap(roomDrops[activeRoom]);
    }
    projectiles.clear();
    hostileShots.clear();
    poisonZones.clear();
    floating.clear();
    if (!changingRoom || health <= 0)
    {
        health = static_cast<float>(progress.stats.maxHealth);
        mana = static_cast<float>(progress.stats.maxMana);
    }
    attackTimer = damageTimer = castTimer = navigationTimer = 0;
    magic = HuntingSpells{};
    actions = PlayerActions{};
    keys.fill(false);

    if (activeRoom < 0)
    {
        Notify("별빛 숲 캠프 · 노란 포탈 앞에서 E를 누르면 사냥터에 입장합니다.");
        return;
    }
    auto candidates = map.SpawnLocations();
    std::shuffle(candidates.begin(), candidates.end(), random);
    for (const Position& p : candidates)
    {
        bool separated = true;
        for (const auto& enemy : enemies)
        {
            if (Distance(enemy.home, p) < 6.f)
            {
                separated = false;
            }
        }
        if (!separated)
        {
            continue;
        }

        Enemy enemy;
        enemy.position = enemy.home = enemy.wanderTarget = p;
        enemy.kind = Rooms[activeRoom].kind;
        ScaleEnemy(enemy);
        enemy.health = enemy.maxHealth;
        enemies.push_back(enemy);
        if (enemies.size() == 8)
        {
            break;
        }
    }
    navigation = map.Distances(player);
    Notify(std::string(Rooms[activeRoom].name) + " 사냥터 · 몬스터 처치로 경험치를 얻습니다.");
}

bool LineOfSight(Position a, Position b)
{
    int steps = (std::max)(1, static_cast<int>(std::ceil(Distance(a, b) / .12f)));
    for (int i = 1; i <= steps; ++i)
    {
        float t = i / static_cast<float>(steps);
        if (!map.Walkable({a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t}, .06f))
        {
            return false;
        }
    }
    return true;
}

void Attack()
{
    if (attackTimer > 0 || !actions.Fire(mana))
    {
        return;
    }

    castDirection = facing;
    AutoTarget::Selection aim(player.x, player.y, facing.x, facing.y, BasicAttackRange,
                              AutoTarget::Mode::Nearest);
    {
        Performance::Scope scope("cpu.combat.auto_target_ms");
        for (const auto& enemy : enemies)
        {
            if (enemy.health > 0 && Distance(player, enemy.position) < aim.distance)
            {
                aim.Consider(enemy.position.x, enemy.position.y, true,
                             LineOfSight(player, enemy.position));
            }
        }
    }

    attackTimer = .5f;
    castTimer = CharacterVisual::CastDuration;
    castMirror = castDirection.x - castDirection.y < 0;
    projectiles.push_back(
        {player, {aim.directionX, aim.directionY}, progress.stats.attack,
         BasicAttackRange / CharacterVisual::SpellSpeed});
    projectiles.back().launchHeight = actions.height;
    projectiles.back().wandFacing = castDirection;
}

void Kill(Enemy& enemy)
{
    enemy.health = 0;
    enemy.respawnTimer = 10;
    const int reward = 7 + enemy.level * 2;
    const auto previousStats = progress.stats;
    bool leveled = progress.Award(reward);
    FloatText(enemy.position, "+" + std::to_string(reward) + " 경험치", teal);

    constexpr size_t DropLimit = 300;
    std::uniform_int_distribution<int> roll(0, 99);
    if (drops.size() < DropLimit && roll(random) < 2)
    {
        drops.push_back({enemy.position, 0, 2 + enemy.level});
    }
    if (drops.size() < DropLimit && roll(random) < 5)
    {
        drops.push_back({enemy.position, 1, 1});
    }
    if (drops.size() < DropLimit && roll(random) < 5)
    {
        drops.push_back({enemy.position, 3, 1});
    }

    if (leveled)
    {
        health += progress.stats.maxHealth - previousStats.maxHealth;
        mana += progress.stats.maxMana - previousStats.maxMana;
        levelUpTimer = LevelUpDuration;
        Notify("레벨 " + std::to_string(progress.stats.level) +
               " 달성! 최대 체력·마나·공격력·방어력이 상승했습니다.");
    }
}

void CastSpecial(bool starfall)
{
    float& cooldown = starfall ? magic.starfallCooldown : magic.burstCooldown;
    const float cost = starfall ? 8.f : 10.f;
    if (activeRoom < 0 || cooldown > 0 || actions.reload > 0 || mana < cost)
    {
        return;
    }

    Position center = player;
    if (starfall)
    {
        for (int tile = 1; tile <= 4; ++tile)
        {
            const Position next{player.x + facing.x * tile, player.y + facing.y * tile};
            if (!LineOfSight(center, next))
            {
                break;
            }
            center = next;
        }
        if (Distance(player, center) < .5f)
        {
            return;
        }
        magic.starfall = .65f;
        magic.starfallOrigin = center;
    }
    else
    {
        magic.burst = .65f;
        magic.burstOrigin = center;
    }

    mana -= cost;
    cooldown = starfall ? 3.f : 2.f;
    const float radius = starfall ? 1.8f : 3.f;
    const int damage = progress.stats.attack * (starfall ? 2 : 3);
    for (auto& enemy : enemies)
    {
        if (enemy.health <= 0 || Distance(enemy.position, center) > radius ||
            !LineOfSight(center, enemy.position))
        {
            continue;
        }
        enemy.health -= damage;
        FloatText(enemy.position, "-" + std::to_string(damage), gold);
        if (enemy.health <= 0)
        {
            Kill(enemy);
        }
    }
    Performance::Profiler::Get().Count(starfall ? "combat.starfall_count" : "combat.star_burst_count");
}

void UpdateProjectiles(float dt)
{
    Performance::Scope profileScope("cpu.gameplay.projectiles_ms");
    for (auto& shot : projectiles)
    {
        const float elapsed = (std::min)(dt, shot.lifetime);
        shot.age += elapsed;
        int steps =
            (std::max)(1, static_cast<int>(std::ceil(CharacterVisual::SpellSpeed * elapsed / .10f)));
        for (int step = 0; step < steps && shot.lifetime > 0; ++step)
        {
            Position next = {
                shot.position.x + shot.direction.x * CharacterVisual::SpellSpeed * elapsed / steps,
                shot.position.y + shot.direction.y * CharacterVisual::SpellSpeed * elapsed / steps};
            if (!map.Walkable(next, .06f))
            {
                shot.lifetime = 0;
                break;
            }
            shot.position = next;
            for (auto& enemy : enemies)
            {
                if (enemy.health > 0 && Distance(shot.position, enemy.position) < BasicAttackHitRadius)
                {
                    enemy.health -= shot.damage;
                    FloatText(enemy.position, "-" + std::to_string(shot.damage), gold);
                    shot.lifetime = 0;
                    if (enemy.health <= 0)
                    {
                        Kill(enemy);
                    }
                    break;
                }
            }
        }
        shot.lifetime -= elapsed;
    }
    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
                                     [](const Projectile& shot)
                                     {
                                         return shot.lifetime <= 0;
                                     }),
                      projectiles.end());
}

void RetryFight()
{
    player = map.camp;
    health = float(progress.stats.maxHealth);
    mana = float(progress.stats.maxMana);
    navigationTimer = 0;
    projectiles.clear();
    hostileShots.clear();
    poisonZones.clear();
    actions = PlayerActions{};
    damageTimer = castTimer = attackTimer = 0;
    magic = HuntingSpells{};
    paused = false;
    keys.fill(false);
    Notify("별빛 숲 캠프에서 다시 도전합니다.");
}

void ReceiveDamage(int attack)
{
    if (Defeated() || damageTimer > 0 || map.Safe(player))
    {
        return;
    }
    int damage = (std::max)(1, attack - progress.stats.defense);
    health = (std::max)(0.f, health - damage);
    damageTimer = .8f;
    FloatText(player, "-" + std::to_string(damage), {1, .3f, .4f, 1});
    if (Defeated())
    {
        keys.fill(false);
    }
}

void UpdateEnemies(float dt)
{
    Performance::Scope profileScope("cpu.gameplay.enemy_ai_ms");
    navigationTimer -= dt;
    if (navigationTimer <= 0)
    {
        navigation = map.Distances(player);
        navigationTimer = .25f;
    }

    const int dx[] = {1, -1, 0, 0};
    const int dy[] = {0, 0, 1, -1};
    for (auto& enemy : enemies)
    {
        if (enemy.health <= 0)
        {
            enemy.respawnTimer -= dt;
            if (enemy.respawnTimer <= 0 && Distance(player, enemy.home) > 4)
            {
                enemy.position = enemy.home;
                ScaleEnemy(enemy);
                enemy.health = enemy.maxHealth;
                enemy.attackTimer = 1;
                enemy.attackMotion = 0;
                enemy.nextAttackIsAlternate = false;
                enemy.chargeRemaining = 0;
                enemy.wanderTimer = 0;
            }
            continue;
        }

        enemy.attackTimer -= dt;
        const auto mainSkill = MonsterSkills::Get(enemy.kind);
        const auto alternate = mainSkill.style == MonsterSkills::Style::Melee ||
                                       mainSkill.style == MonsterSkills::Style::Charge ||
                                       mainSkill.style == MonsterSkills::Style::Poison
                                   ? MonsterSkills::Profile{MonsterSkills::Style::Bolt, 5.f, 2.2f,
                                                            "마법탄 준비"}
                                   : MonsterSkills::Profile{MonsterSkills::Style::Poison, 6.f, 3.f,
                                                            "위험 장판 준비"};
        const auto nextSkill = enemy.nextAttackIsAlternate ? alternate : mainSkill;
        if (MonsterAnimation::Advance(enemy.attackMotion, dt) && !map.Safe(player))
        {
            const auto skill = enemy.pendingSkill;
            const int damage = 9 + enemy.level * 2;
            Performance::Profiler::Get().Count("combat.enemy_skills_cast_count");
            if (skill.style == MonsterSkills::Style::Melee)
            {
                if (Distance(enemy.position, player) < 1.1f && LineOfSight(enemy.position, player))
                {
                    ReceiveDamage(damage);
                }
            }
            else if (skill.style == MonsterSkills::Style::Charge)
            {
                enemy.chargeRemaining = .5f;
                enemy.chargeHit = false;
            }
            else if (skill.style == MonsterSkills::Style::Poison)
            {
                if (poisonZones.size() < 24 && map.Walkable(enemy.skillTarget) &&
                    !map.Safe(enemy.skillTarget) && LineOfSight(enemy.position, enemy.skillTarget))
                {
                    poisonZones.push_back({enemy.skillTarget, damage, nextVisualId++});
                }
            }
            else if (hostileShots.size() < 62)
            {
                const int count = skill.style == MonsterSkills::Style::Fan ? 5
                                  : enemy.kind == 8                        ? 2
                                                                           : 1;
                for (int i = 0; i < count; ++i)
                {
                    const float spacing = enemy.kind == 8 ? .38f : .21f;
                    const float angle = (i - (count - 1) * .5f) * spacing;
                    const float c = std::cos(angle), s = std::sin(angle);
                    hostileShots.push_back({enemy.position,
                                            {enemy.facing.x * c - enemy.facing.y * s,
                                             enemy.facing.x * s + enemy.facing.y * c},
                                            damage,
                                            nextVisualId++});
                }
            }
        }
        if (enemy.chargeRemaining > 0)
        {
            const float step = (std::min)(dt, enemy.chargeRemaining);
            enemy.chargeRemaining -= step;
            const Position before = enemy.position;
            const Position next = {before.x + enemy.facing.x * 7.f * step,
                                   before.y + enemy.facing.y * 7.f * step};
            if (!map.Walkable(next) || map.Safe(next))
            {
                enemy.chargeRemaining = 0;
            }
            else
            {
                enemy.position = next;
                enemy.animation += Distance(before, next) * 7;
                if (!enemy.chargeHit && Distance(next, player) < .7f)
                {
                    enemy.chargeHit = true;
                    ReceiveDamage(11 + enemy.level * 2);
                }
            }
            continue;
        }
        enemy.wanderTimer -= dt;
        int x = static_cast<int>(enemy.position.x);
        int y = static_cast<int>(enemy.position.y);
        int cell = Hunting::Map::Index(x, y);
        bool chase = !map.Safe(player) && navigation[cell] >= 0 && navigation[cell] < 12;
        Position target = enemy.wanderTarget;
        if (chase)
        {
            int best = navigation[cell];
            target = player;
            for (int d = 0; d < 4; ++d)
            {
                if (!map.Floor(x + dx[d], y + dy[d]))
                {
                    continue;
                }
                int next = Hunting::Map::Index(x + dx[d], y + dy[d]);
                if (navigation[next] >= 0 && navigation[next] < best)
                {
                    best = navigation[next];
                    target = {x + dx[d] + .5f, y + dy[d] + .5f};
                }
            }
        }
        else if (enemy.wanderTimer <= 0)
        {
            enemy.wanderTimer = 1.5f;
            int d = static_cast<int>(random() % 4);
            Position candidate = {x + dx[d] + .5f, y + dy[d] + .5f};
            if (map.Walkable(candidate) && !map.Safe(candidate))
            {
                enemy.wanderTarget = target = candidate;
            }
        }

        float distance = Distance(enemy.position, target);
        const bool hasSight = chase && LineOfSight(enemy.position, player);
        const bool holdRange =
            hasSight && nextSkill.range > 1 &&
            Distance(enemy.position, player) < nextSkill.range * .8f;
        if (!holdRange && enemy.attackMotion <= 0 && distance > .08f &&
            (!chase || Distance(enemy.position, player) > .65f))
        {
            float speed = chase ? 1.6f + (enemy.kind % 2) * .35f : .65f;
            Position before = enemy.position;
            map.Move(enemy.position, {(target.x - enemy.position.x) / distance * speed * dt,
                                      (target.y - enemy.position.y) / distance * speed * dt});
            if (map.Safe(enemy.position))
            {
                enemy.position = before;
            }
            const float traveled = Distance(enemy.position, before);
            if (traveled > .001f)
            {
                enemy.facing = {(enemy.position.x - before.x) / traveled,
                                (enemy.position.y - before.y) / traveled};
            }
            enemy.animation += traveled * 7;
        }
        if (hasSight && Distance(enemy.position, player) < nextSkill.range &&
            enemy.attackTimer <= 0 && enemy.attackMotion <= 0)
        {
            enemy.pendingSkill = nextSkill;
            enemy.nextAttackIsAlternate = !enemy.nextAttackIsAlternate;
            enemy.attackTimer = nextSkill.cooldown;
            enemy.attackMotion = MonsterAnimation::Duration;
            enemy.skillTarget = player;
            const float distanceToPlayer = Distance(enemy.position, player);
            if (distanceToPlayer > .001f)
            {
                enemy.facing = {(player.x - enemy.position.x) / distanceToPlayer,
                                (player.y - enemy.position.y) / distanceToPlayer};
            }
        }
    }
}

void UpdateHostileSkills(float dt)
{
    Performance::Scope scope("cpu.combat.enemy_skills_update_ms");
    hostileShots.erase(std::remove_if(hostileShots.begin(), hostileShots.end(),
                                      [dt](MonsterSkills::Projectile& shot)
                                      {
                                          return !MonsterSkills::Advance(
                                              shot, dt,
                                              [](Position p)
                                              {
                                                  return !map.Walkable(p, .06f) || map.Safe(p);
                                              },
                                              [&shot](Position p)
                                              {
                                                  if (Distance(p, player) < .4f)
                                                  {
                                                      ReceiveDamage(shot.damage);
                                                      return true;
                                                  }
                                                  return false;
                                              });
                                      }),
                       hostileShots.end());
    for (auto& poison : poisonZones)
    {
        if (poison.Advance(dt) &&
            Distance(player, poison.position) < MonsterSkills::Poison::Radius &&
            LineOfSight(poison.position, player))
        {
            ReceiveDamage(poison.damage);
        }
    }
    poisonZones.erase(std::remove_if(poisonZones.begin(), poisonZones.end(),
                                     [](const MonsterSkills::Poison& poison)
                                     {
                                         return poison.age >= MonsterSkills::Poison::Duration;
                                     }),
                      poisonZones.end());
    Performance::Profiler::Get().Gauge("combat.enemy_projectiles_count",
                                       double(hostileShots.size()));
    Performance::Profiler::Get().Gauge("combat.enemy_poison_zones_count",
                                       double(poisonZones.size()));
}

void PickUp()
{
    Performance::Scope profileScope("cpu.gameplay.loot_ms");
    int collected = 0;
    for (auto it = drops.begin(); it != drops.end();)
    {
        if (Distance(player, it->position) > 2.f || !LineOfSight(player, it->position))
        {
            ++it;
            continue;
        }
        if ((it->kind == 1 && progress.potions >= 99) ||
            (it->kind == 3 && progress.manaPotions >= 99))
        {
            ++it;
            continue;
        }
        if (it->kind == 0)
        {
            progress.coins = (std::min)(1000000, progress.coins + it->quantity);
        }
        else if (it->kind == 1)
        {
            progress.potions += it->quantity;
        }
        else if (it->kind == 3)
        {
            progress.manaPotions = (std::min)(99, progress.manaPotions + it->quantity);
            FloatText(player, "마나 물약 +" + std::to_string(it->quantity), teal);
        }
        ++collected;
        it = drops.erase(it);
    }
    if (collected > 0)
    {
        Notify("전리품 획득! 골드·체력/마력 물약을 획득했습니다.");
    }
    else
    {
        Notify("가까운 전리품이 없습니다. 물약은 최대 99개까지 소지할 수 있습니다.");
    }
}

void PlaceModel(const std::string& name, ModelId model, Game::Transform pose, Game::Actor* parent,
                bool mirror = false, Game::RenderLayer layer = Game::RenderLayer::World)
{
    scene.Place(
        name, pose, layer,
        [model, mirror](const Game::Actor&, const Game::Transform& world)
        {
            Model(model, {world.x, world.y}, world.scale, world.z, mirror);
        },
        parent);
}

void PlaceEffect(const std::string& name, Game::Transform pose, float width, float height, int kind,
                 Game::Actor* parent)
{
    scene.Place(
        name, pose, Game::RenderLayer::World,
        [width, height, kind](const Game::Actor&, const Game::Transform& world)
        {
            // Effect() takes the bottom edge; Actor.z describes the effect center.
            Effect({world.x, world.y}, width * world.scale, height * world.scale, kind,
                   world.z - height * world.scale * .5f);
        },
        parent);
}

void DrawMap()
{
    if (scene.Find("terrain"))
    {
        return;
    }
    Performance::Scope scope("cpu.scene.static_build_ms");
    auto& terrain = scene.Place("terrain", {}, Game::RenderLayer::Ground);
    auto& scenery = scene.Place("scenery", {}, Game::RenderLayer::World);
    for (int y = 0; y < Hunting::Map::Height; ++y)
    {
        for (int x = 0; x < Hunting::Map::Width; ++x)
        {
            auto tile = map.tiles[Hunting::Map::Index(x, y)];
            if (tile == Hunting::Tile::Void)
            {
                continue;
            }
            Position p = {x + .5f, y + .5f};
            const bool stone = map.Safe(p);
            std::string key = std::to_string(Hunting::Map::Index(x, y));
            scene.Place(
                "tile/" + key, {p.x, p.y}, Game::RenderLayer::Ground,
                [tile, stone, x, y](const Game::Actor&, const Game::Transform& world)
                {
                    Position screen = Project({world.x, world.y}, world.z);
                    if (!Visible(screen, 36 * world.scale))
                    {
                        return;
                    }
                    if (tile == Hunting::Tile::Water)
                    {
                        Effect({world.x, world.y}, 65 * world.scale, 33 * world.scale, 0,
                               world.z - 16.5f * world.scale, x * .63f + y * .91f);
                    }
                    else
                    {
                        Model(stone ? ModelId::FloorStone : ModelId::TerrainGrass,
                              {world.x, world.y}, world.scale * 1.01f, world.z);
                    }
                },
                &terrain);
            if (tile == Hunting::Tile::Tree || tile == Hunting::Tile::Rock)
            {
                std::string prop = "prop/" + key;
                PlaceModel(prop,
                           tile == Hunting::Tile::Tree ? NatureVisuals::Tree(p.x, p.y)
                                                       : NatureVisuals::Rock(p.x, p.y),
                           {p.x, p.y, 0, NatureVisuals::Scale(p.x, p.y)}, &scenery);
                PlaceModel(prop + "/shadow", ModelId::Shadow,
                           {0, 0, 0, tile == Hunting::Tile::Tree ? 1.4f : 1.f}, scene.Find(prop),
                           false, Game::RenderLayer::Shadow);
            }
        }
    }
    const Position exit = map.VillageExit();
    auto& marker = scene.Place(
        "return/ground", {exit.x, exit.y}, Game::RenderLayer::Shadow,
        [](const Game::Actor&, const Game::Transform& world)
        {
            const Position p = Project({world.x, world.y});
            const bool ready = Distance(player, {world.x, world.y}) < ExitRange;
            const Color border{.22f, .17f, .25f, 1.f};
            Diamond(p, 24, 14, border);
            Diamond(p, 20, 10, ready ? Color{1.f, .9f, .61f, 1.f} : gold);
            Diamond(p, 5, 3, border);
        },
        &scenery);
    scene.Place(
        "return/label", {0, 0, 48}, Game::RenderLayer::Overlay,
        [](const Game::Actor&, const Game::Transform& world)
        {
            if (Distance(player, {world.x, world.y}) > 5.5f)
            {
                return;
            }
            Position screen = Project({world.x, world.y}, world.z);
            if (Visible(screen, 0))
            {
                const bool ready = Distance(player, {world.x, world.y}) < ExitRange;
                const char* label = activeRoom < 0 ? (ready ? "별빛 마을로 [E]" : "마을 연결 입구")
                                                    : (ready ? "캠프로 돌아가기 [E]" : "입구 · 캠프행");
                graphics->Nameplate(screen.x * screenWidth / 1280, screen.y * screenHeight / 800,
                                    label, 17.f * screenHeight / 800, 0, 0, 0, true);
            }
        },
        &marker);
    if (activeRoom < 0)
    {
        for (size_t i = 0; i < Rooms.size(); ++i)
        {
            const auto& room = Rooms[i];
            const std::string id = "room/portal/" + std::to_string(i);
            auto& portal = scene.Place(id, {room.portal.x, room.portal.y}, Game::RenderLayer::Shadow,
                                       [i](const Game::Actor&, const Game::Transform& world)
                                       {
                                           const bool unlocked = progress.stats.level >= Rooms[i].level;
                                           Diamond(Project({world.x, world.y}), 19, 11,
                                                   unlocked ? gold : Color{.48f, .50f, .56f, 1.f});
                                       },
                                       &scenery);
            scene.Place(id + "/label", {0, 0, 38}, Game::RenderLayer::Overlay,
                        [i](const Game::Actor&, const Game::Transform& world)
                        {
                            const auto& area = Rooms[i];
                            const auto p = Project({world.x, world.y}, world.z);
                            const bool unlocked = progress.stats.level >= area.level;
                            const std::string label = "Lv." + std::to_string(area.level) + " " +
                                                      area.name + (unlocked ? " [E]" : " · 잠김");
                            graphics->Nameplate(p.x * screenWidth / 1280, p.y * screenHeight / 800,
                                                label, 14.f * screenHeight / 800,
                                                unlocked ? 1.f : .7f, unlocked ? .85f : .7f, .6f);
                        },
                        &portal);
        }
    }
    PlaceModel("beacon", ModelId::Campfire, {map.camp.x + 1, map.camp.y - 1}, &scenery);
    PlaceEffect("beacon/flame", {0, 0, 31.5f}, 42, 63, 1, scene.Find("beacon"));
    scene.RetainSubtree(terrain);
    scene.RetainSubtree(scenery);
}

void SkillDisc(Position center, float radius, Color color)
{
    static const auto ring = []()
    {
        std::array<Position, 25> points{};
        for (size_t i = 0; i < points.size(); ++i)
        {
            const float angle = float(i) / 24 * 6.2831853f;
            points[i] = {std::cos(angle), std::sin(angle)};
        }
        return points;
    }();
    const Position p = Project(center);
    for (size_t i = 1; i < ring.size(); ++i)
    {
        Triangle(p, Project({center.x + ring[i - 1].x * radius, center.y + ring[i - 1].y * radius}),
                 Project({center.x + ring[i].x * radius, center.y + ring[i].y * radius}), color);
    }
}

void DrawHostileSkills()
{
    Performance::Scope scope("cpu.render.enemy_skills_sync_ms");
    auto& root = scene.Place("hostile/skills", {}, Game::RenderLayer::World);
    for (const auto& shot : hostileShots)
    {
        auto& actor = scene.Place(
            "hostile/shot/" + std::to_string(shot.id), {shot.position.x, shot.position.y},
            Game::RenderLayer::World,
            [](const Game::Actor&, const Game::Transform& world)
            {
                const Position p = Project({world.x, world.y}, 27);
                Diamond(p, 9, 12, {.84f, .21f, .54f, 1});
                Diamond(p, 4, 6, {1, .74f, .91f, 1});
            },
            &root);
        PlaceModel("hostile/shot/shadow/" + std::to_string(shot.id), ModelId::Shadow,
                   {0, 0, 0, .45f}, &actor, false, Game::RenderLayer::Shadow);
    }
    for (const auto& poison : poisonZones)
    {
        scene.Place(
            "hostile/poison/" + std::to_string(poison.id), {poison.position.x, poison.position.y},
            Game::RenderLayer::Ground,
            [poison](const Game::Actor&, const Game::Transform& world)
            {
                const Position p = {world.x, world.y};
                const bool warning = poison.age < MonsterSkills::Poison::Warning;
                SkillDisc(p, MonsterSkills::Poison::Radius,
                          warning ? Color{1, .34f, .38f, .6f} : Color{.3f, .79f, .22f, .5f});
                SkillDisc(p, MonsterSkills::Poison::Radius * .85f,
                          warning ? Color{.35f, .1f, .15f, .7f} : Color{.15f, .42f, .24f, .6f});
            },
            &root);
    }
}

void DrawActors()
{
    auto& dynamic = scene.Place("characters", {}, Game::RenderLayer::World);
    if (Prologue::TransformationUnlocked())
    {
        const Position look = castTimer > 0 ? castDirection : facing;
        const Position position = CompanionFollow::RightOf(player, look);
        PlaceModel("lumi", ModelId::Lumi, {position.x, position.y, 26 + 4 * std::sin(time * 3)},
                   &dynamic);
        scene.Place("lumi/name", {0, 0, 60}, Game::RenderLayer::Overlay,
                    [](const Game::Actor&, const Game::Transform& world)
                    {
                        const Position screen = Project({world.x, world.y}, world.z);
                        graphics->Nameplate(screen.x * screenWidth / 1280,
                                            screen.y * screenHeight / 800,
                                            "루미 [F]", 16.f * screenHeight / 800);
                    },
                    scene.Find("lumi"));
    }
    ModelId playerModel =
        static_cast<ModelId>(CharacterVisual::FrameIndex(castTimer, moving, walkFrame));
    bool mirror = castTimer > 0 ? castMirror : facing.x - facing.y < 0;
    // Root remains visible during damage flicker, so its shadow/socket are not lost.
    auto& heroine =
        scene.Place("player", {player.x, player.y}, Game::RenderLayer::World, {}, &dynamic);
    const Position look = castTimer > 0 ? castDirection : facing;
    scene.Place(
        "player/body", {0, 0, actions.height}, Game::RenderLayer::World,
        [playerModel, look, mirror](const Game::Actor&, const Game::Transform& world)
        {
            const Position p = Project({world.x, world.y}, world.z);
            graphics->DrawModel(models.GetFacing(playerModel, look.x, look.y),
                                p.x * screenWidth / 1280, p.y * screenHeight / 800,
                                float(screenWidth) / 1280 *
                                    (models.loadedFromResource ? 1.f
                                     : mirror                  ? -1.f
                                                               : 1.f),
                                float(screenHeight) / 800);
        },
        &heroine);
    scene.Find("player/body")->visible = damageTimer <= 0 || static_cast<int>(time * 14) % 2 == 0;
    PlaceModel("player/shadow", ModelId::Shadow, {}, &heroine, false, Game::RenderLayer::Shadow);
    if (levelUpTimer > 0)
    {
        scene.Place("player/level-up", {0, 0, actions.height + 120.f}, Game::RenderLayer::Overlay,
                    [](const Game::Actor&, const Game::Transform& world)
                    {
                        const float age = 1.f - levelUpTimer / LevelUpDuration;
                        const float alpha = (std::min)(1.f, levelUpTimer / .4f);
                        const Position p = Project({world.x, world.y}, world.z + age * 30.f);
                        Text(p.x - 89, p.y + 2, "LEVEL UP!", {.13f, .08f, .17f, alpha}, 34);
                        Text(p.x - 91, p.y, "LEVEL UP!", {1.f, .86f, .42f, alpha}, 34);
                    },
                    &heroine);
    }
    CombatEffects::Recovery(
        scene, *graphics, heroine, actions, actions.height,
        (std::min)(screenWidth / 1280.f, screenHeight / 800.f),
        [](const Game::Transform& world)
        {
            const Position p = Project({world.x, world.y}, world.z);
            return Position{p.x * screenWidth / 1280, p.y * screenHeight / 800};
        });
    auto& socket =
        scene.Place("player/wand", CharacterVisual::FacingSocket(castTimer > 0, look.x, look.y),
                    Game::RenderLayer::World, {}, &heroine);
    socket.local.z += actions.height;
    if (castTimer > 0)
    {
        PlaceEffect("player/wand/flash", {}, 24, 24, 2, &socket);
    }
    for (size_t i = 0; i < enemies.size(); ++i)
    {
        const auto& enemy = enemies[i];
        if (enemy.health <= 0)
        {
            continue;
        }
        std::string name = "enemy/" + std::to_string(i);
        PlaceModel(name,
                   MonsterAnimation::FacingModel(enemy.kind, enemy.animation * .25f,
                                                 enemy.attackMotion, enemy.facing.x,
                                                 enemy.facing.y),
                   {enemy.position.x, enemy.position.y}, &dynamic);
        PlaceModel(name + "/shadow", ModelId::Shadow, {}, scene.Find(name), false,
                   Game::RenderLayer::Shadow);
        if (enemy.attackMotion > MonsterAnimation::ImpactRemaining)
        {
            const auto skill = enemy.pendingSkill;
            scene.Place(
                name + "/warning", {0, 0, 95}, Game::RenderLayer::Overlay,
                [skill](const Game::Actor&, const Game::Transform& world)
                {
                    const Position p = Project({world.x, world.y}, world.z);
                    if (Visible(p))
                    {
                        Text(p.x - 32, p.y, skill.warning, gold, 12);
                    }
                },
                scene.Find(name));
            if (skill.style == MonsterSkills::Style::Melee ||
                skill.style == MonsterSkills::Style::Poison)
            {
                const bool melee = skill.style == MonsterSkills::Style::Melee;
                const Position center = melee ? enemy.position : enemy.skillTarget;
                scene.Place(name + "/danger", {center.x, center.y}, Game::RenderLayer::Shadow,
                            [melee](const Game::Actor&, const Game::Transform& world)
                            {
                                SkillDisc({world.x, world.y},
                                          melee ? 1.1f : MonsterSkills::Poison::Radius,
                                          {1.f, .28f, .36f, .40f});
                            });
            }
            else
            {
                const bool charge = skill.style == MonsterSkills::Style::Charge;
                scene.Place(
                    name + "/attack-path", {}, Game::RenderLayer::Shadow,
                    [enemy, charge](const Game::Actor&, const Game::Transform& world)
                    {
                        const int steps = charge ? 7 : 12;
                        for (int i = 1; i <= steps; ++i)
                        {
                            const Position p = Project({world.x + enemy.facing.x * i * .5f,
                                                        world.y + enemy.facing.y * i * .5f});
                            Diamond(p, charge ? 9 : 5, 4, {1, .36f, .4f, .75f});
                        }
                    },
                    scene.Find(name));
            }
        }
    }
    auto& loot = scene.Place("loot", {}, Game::RenderLayer::World);
    for (const auto& drop : drops)
    {
        ModelId model = drop.kind == 0   ? ModelId::Coin
                        : drop.kind == 1 ? ModelId::HealthPotion
                                         : ModelId::ManaPotion;
        PlaceModel(
            "drop/" + std::to_string(drop.visualId), model,
            {drop.position.x, drop.position.y, 0, drop.kind == 1 || drop.kind == 3 ? .75f : 1.f},
            &loot);
        if ((drop.kind == 1 || drop.kind == 3) && Distance(player, drop.position) < 3.f)
        {
            scene.Place(
                "drop/mana_label/" + std::to_string(drop.visualId),
                {drop.position.x, drop.position.y, 28}, Game::RenderLayer::World,
                [drop](const Game::Actor&, const Game::Transform& world)
                {
                    Position screen = Project({world.x, world.y}, world.z);
                    if (Visible(screen))
                    {
                        graphics->Nameplate(
                            screen.x * screenWidth / 1280, screen.y * screenHeight / 800,
                            drop.kind == 1 ? "체력물약[E]" : "마나물약[E]",
                            12.f * screenHeight / 800, drop.kind == 1 ? 1.f : .52f,
                            drop.kind == 1 ? .52f : .75f, drop.kind == 1 ? .62f : 1.f);
                    }
                },
                &loot);
        }
    }
    auto& spells = scene.Place("spells", {}, Game::RenderLayer::World);
    for (const auto& shot : projectiles)
    {
        auto pose = CharacterVisual::FacingSpellPose(shot.position.x, shot.position.y, shot.age,
                                                     shot.wandFacing.x, shot.wandFacing.y);
        pose.z += shot.launchHeight * (1 - (std::min)(1.f, shot.age / .18f));
        const std::string id = "shot/" + std::to_string(shot.visualId);
        PlaceEffect(id, pose, 38, 38, 2, &spells);
        scene.Place(id + "/ground", {shot.position.x, shot.position.y}, Game::RenderLayer::Shadow,
                    [](const Game::Actor&, const Game::Transform& world)
                    {
                        Diamond(Project({world.x, world.y}), 11, 5, {.48f, .96f, .9f, .8f});
                    },
                    &spells);
    }
    DrawHostileSkills();
    CombatEffects::Spells(scene, *graphics, magic,
                          (std::min)(screenWidth / 1280.f, screenHeight / 800.f),
                          [](const Game::Transform& world)
                          {
                              const Position p = Project({world.x, world.y}, world.z);
                              return Position{p.x * screenWidth / 1280, p.y * screenHeight / 800};
                          });
}

void DrawHud()
{
    Box(18, 18, 820, 136, panel);
    Text(35, 48, activeRoom < 0 ? "별빛 숲 · 사냥터 포탈"
                                : std::string(Rooms[activeRoom].name) + " 사냥터",
         gold, 25);
    char line[320];
    sprintf_s(line, "레벨 %d   공격력 %d   방어력 %d", progress.stats.level, progress.stats.attack,
              progress.stats.defense);
    Text(35, 80, line);
    const float healthRatio = (std::max)(0.f, (std::min)(1.f, health / progress.stats.maxHealth));
    const float manaRatio = (std::max)(0.f, (std::min)(1.f, mana / progress.stats.maxMana));
    Box(22, 212, 152, 354, panel);
    Text(41, 231, "체력", white, 18);
    Text(105, 231, "마나", white, 18);
    Box(48, 243, 22, 280, {.20f, .22f, .30f, 1});
    Box(48, 243 + 280 * (1 - healthRatio), 22, 280 * healthRatio, {.95f, .40f, .58f, 1});
    Box(112, 243, 22, 280, {.20f, .22f, .30f, 1});
    Box(112, 243 + 280 * (1 - manaRatio), 22, 280 * manaRatio, {.52f, .65f, .98f, 1});
    sprintf_s(line, "%.0f/%d", health, progress.stats.maxHealth);
    Text(34, 542, line, white, 14);
    sprintf_s(line, "%.0f/%d", mana, progress.stats.maxMana);
    Text(98, 542, line, white, 14);
    Text(34, 560, "[U] " + std::to_string(progress.potions) + "개", white, 15);
    Text(98, 560, "[I] " + std::to_string(progress.manaPotions) + "개", white, 15);

    float ratio = progress.stats.level == Rpg::LevelCap
                      ? 1.f
                      : float(progress.ExperienceInLevel()) /
                            Rpg::Progress::RequiredExperience(progress.stats.level);
    Box(35, 95, 775, 9, {.10f, .15f, .19f, 1});
    Box(35, 95, 775 * ratio, 9, teal);
    if (progress.stats.level == Rpg::LevelCap)
    {
        sprintf_s(line, "최고 레벨 달성   누적 경험치 %lld",
                  static_cast<long long>(progress.totalExperience));
    }
    else
    {
        sprintf_s(
            line,
            "경험치 %d / %d   누적 %lld   다음 레벨: 체력 +12 · 마나 +5 · 공격력 +3 · 방어력 +1",
            progress.ExperienceInLevel(), Rpg::Progress::RequiredExperience(progress.stats.level),
            static_cast<long long>(progress.totalExperience));
    }
    Text(35, 135, line, teal, 16);

    Box(18, 166, 600, 40, panel);
    sprintf_s(line, "골드 %d    체력 물약 %d [U]    마력 물약 %d [I]    %s", progress.coins,
              progress.potions, progress.manaPotions,
              map.Safe(player) ? "모닥불: 회복 중 · 표지판 E로 마을 이동"
                               : "숲 사냥터 · 악몽 출현");
    Text(35, 193, line, gold, 17);

    Box(1030, 20, 230, 170, panel);
    const float unit = 2.8f;
    for (int y = 0; y < Hunting::Map::Height; ++y)
    {
        for (int x = 0; x < Hunting::Map::Width; ++x)
        {
            Color color = map.Floor(x, y) ? Color{.24f, .38f, .29f, 1} : Color{.11f, .18f, .24f, 1};
            Box(1045 + x * unit, 31 + y * unit, unit, unit, color);
        }
    }
    for (const auto& enemy : enemies)
    {
        if (enemy.health > 0)
        {
            Box(1045 + enemy.position.x * unit, 31 + enemy.position.y * unit, 3, 3,
                {.95f, .35f, .36f, 1});
        }
    }
    Diamond({1045 + map.camp.x * unit, 31 + map.camp.y * unit}, 4, 4, gold);
    const Position exit = map.VillageExit();
    Diamond({1045 + exit.x * unit, 31 + exit.y * unit}, 4, 4, white);
    if (activeRoom < 0)
    {
        for (const auto& room : Rooms)
        {
            Diamond({1045 + room.portal.x * unit, 31 + room.portal.y * unit}, 3, 3,
                    progress.stats.level >= room.level ? gold : Color{.48f, .50f, .56f, 1.f});
        }
    }
    Diamond({1045 + player.x * unit, 31 + player.y * unit}, 4, 4, teal);
    sprintf_s(line, "맵 시드: %u", map.seed);
    Text(1045, 179, line, white, 14);

    for (const auto& enemy : enemies)
    {
        Position s = Project(enemy.position, 50);
        if (enemy.health > 0 && Distance(player, enemy.position) < 7 && Visible(s))
        {
            sprintf_s(line, "레벨 %d %s", enemy.level, MonsterAnimation::Name(enemy.kind));
            Text(s.x - 45, s.y, line, white, 13);
            Box(s.x - 25, s.y + 5, 50, 4, panel);
            Box(s.x - 25, s.y + 5, 50.f * enemy.health / enemy.maxHealth, 4, {.9f, .27f, .35f, 1});
        }
    }
    for (const auto& text : floating)
    {
        Position s = Project(text.position, 65 + (1.2f - text.lifetime) * 25);
        Text(s.x - 20, s.y, text.label, text.color, 18);
    }
    for (const auto& drop : drops)
    {
        if (drop.kind == 0 && Distance(player, drop.position) < 1.5f)
        {
            Text(606, 492, "골드[E]", gold, 12);
            break;
        }
    }
    Box(18, 710, 1244, 74, panel);
    Text(35, 738, "WASD 이동 · J 자동조준 · K 별빛 폭발 · L 별빛 낙하 · U/I 물약 · E 획득",
         gold, 17);
    std::string cache = models.loadedFromFile ? "모델: 파일 캐시 로딩"
                        : models.storedToFile ? "모델: 최초 생성·캐시 저장 완료"
                                              : "모델: 캐시 쓰기 실패";
    Text(35, 769, cache, white, 15);
    if (noticeTimer > 0)
    {
        Box(250, 655, 1012, 45, panel);
        Text(267, 685, notice, teal, 18);
    }
    if (Defeated())
    {
        Box(270, 275, 740, 195, panel);
        Text(307, 320, "쓰러졌습니다", gold, 26);
        Text(307, 370, "[N] 다시 도전", white, 21);
        Text(307, 416, "[M] 마을로 돌아가기 · 물약 정비", white, 21);
    }
    else if (paused)
    {
        Box(0, 0, 1280, 800, {.02f, .025f, .04f, .78f});
        Box(270, 250, 740, 275, panel);
        Text(307, 298, "별빛 숲 메뉴", gold, 26);
        Text(307, 347, "ESC: 계속하기   |   N: 새로운 랜덤 맵");
        Text(307, 391, "새 맵에서도 경험치와 소지품은 유지됩니다.", teal);
        Text(307, 427, "단, 바닥에 남은 전리품과 몬스터 배치는 초기화됩니다.", white, 16);
        Text(307, 481, "목표: 몬스터 처치 → 경험치 획득 → 자동 스탯 성장", gold, 18);
    }
}
} // namespace

void Initialize(Renderer* renderer)
{
    if (initialized)
    {
        return;
    }
    graphics = renderer;
    models.Initialize();
    progress.Recalculate();
    random.seed(std::random_device{}());
    initialized = true;
    NewMap();
}

WorldMonitor::Scene MonitorSnapshot()
{
    WorldMonitor::Scene snapshot;
    snapshot.id = "hunting";
    snapshot.name = activeRoom < 0 ? "별빛 숲 · 사냥터 포탈"
                                   : std::string(Rooms[activeRoom].name) + " 사냥터";
    snapshot.paused = paused;
    snapshot.objective = activeRoom < 0 ? "레벨에 맞는 포탈 선택"
                                        : "몬스터 사냥 · 경험치 획득";
    for (const auto& enemy : enemies)
    {
        enemy.health > 0 ? ++snapshot.mobsAlive : ++snapshot.mobsDead;
    }
    snapshot.towns.push_back(
        {"hunting_camp", "별빛 숲 사냥 지역", "안전 거점과 야생 구역 공존", {}});
    return snapshot;
}

void Resize(int width, int height)
{
    screenWidth = (std::max)(1, width);
    screenHeight = (std::max)(1, height);
}

void Pause()
{
    paused = true;
    keys.fill(false);
}

void Resume(bool fromVillage)
{
    paused = false;
    worldRequested = false;
    keys.fill(false);
    if (fromVillage)
    {
        if (activeRoom >= 0)
        {
            NewMap();
        }
        const Position entrance = map.VillageExit();
        player = {entrance.x, entrance.y - 1.f};
        actions.height = actions.velocity = 0;
    }
}

bool ConsumeWorldRequest()
{
    const bool requested = worldRequested;
    worldRequested = false;
    return requested;
}

bool Defeated()
{
    return health <= 0;
}

const Rpg::Progress& Progress()
{
    return progress;
}

void SyncGrowth(const Rpg::Progress& source)
{
    const auto previous = progress.stats;
    progress.SyncGrowth(source);
    if (!Defeated())
    {
        health += progress.stats.maxHealth - previous.maxHealth;
        mana += progress.stats.maxMana - previous.maxMana;
    }
}

void Restock(const Rpg::Progress& supplies)
{
    progress.potions = supplies.potions;
    progress.manaPotions = supplies.manaPotions;
    progress.coins = supplies.coins;
    RetryFight();
}

void Key(unsigned char key, bool down)
{
    if (key >= 'A' && key <= 'Z')
    {
        key = key - 'A' + 'a';
    }
    bool fresh = down && !keys[key];
    keys[key] = down;
    if (!fresh)
    {
        return;
    }
    if (Defeated())
    {
        if (key == CombatKeys::Retry)
        {
            RetryFight();
        }
        else if (key == CombatKeys::ReturnToTown)
        {
            worldRequested = true;
            keys.fill(false);
        }
        return;
    }
    if (key == 27)
    {
        paused = !paused;
        keys.fill(false);
        return;
    }
    if (paused)
    {
        if (key == 'n')
        {
            NewMap(activeRoom);
            paused = false;
        }
        return;
    }
    if (key == 'f' && Prologue::TransformationUnlocked())
    {
        Notify("루미: 체력이 부족하면 캠프 모닥불 근처에서 쉬어 가자!");
        return;
    }
    if (key == 'e')
    {
        if (activeRoom < 0)
        {
            for (size_t i = 0; i < Rooms.size(); ++i)
            {
                const auto& room = Rooms[i];
                if (Distance(player, room.portal) >= PortalRange)
                {
                    continue;
                }
                if (progress.stats.level < room.level)
                {
                    Notify("레벨 " + std::to_string(room.level) + "부터 입장할 수 있습니다.");
                    return;
                }
                NewMap(static_cast<int>(i));
                player = map.VillageExit();
                navigation = map.Distances(player);
                return;
            }
        }
        if (Distance(player, map.VillageExit()) < ExitRange)
        {
            if (activeRoom >= 0)
            {
                const Position entrance = Rooms[activeRoom].portal;
                NewMap();
                const float inward = entrance.y < map.camp.y ? 1.2f : -1.2f;
                player = {entrance.x, entrance.y + inward};
            }
            else
            {
                worldRequested = true;
            }
            return;
        }
        PickUp();
    }
    if (key == ' ')
    {
        actions.Jump();
    }
    if (key == CombatKeys::Burst || key == CombatKeys::Starfall)
    {
        CastSpecial(key == CombatKeys::Starfall);
    }
    if (key == CombatKeys::ManaPotion && health > 0)
    {
        if (actions.UseManaPotion(mana, float(progress.stats.maxMana), progress.manaPotions))
        {
            FloatText(player, "마나 회복", teal);
        }
    }
    if (key == CombatKeys::HealthPotion && progress.potions > 0 && health < progress.stats.maxHealth)
    {
        --progress.potions;
        health =
            (std::min)(float(progress.stats.maxHealth), health + progress.stats.maxHealth * .45f);
        actions.MarkRecovery(false);
        FloatText(player, "체력 회복", teal);
    }
}

void Update(float dt)
{
    Performance::Scope profileScope("cpu.gameplay.level_update_ms");
    if (!initialized || paused || Defeated())
    {
        return;
    }
    time += dt;
    actions.Update(dt, mana, float(progress.stats.maxMana));
    attackTimer -= dt;
    magic.burst -= dt;
    magic.starfall -= dt;
    magic.burstCooldown -= dt;
    magic.starfallCooldown -= dt;
    damageTimer -= dt;
    castTimer -= dt;
    levelUpTimer = (std::max)(0.f, levelUpTimer - dt);
    noticeTimer -= dt;
    if (map.Safe(player))
    {
        health = (std::min)(float(progress.stats.maxHealth), health + dt * 10);
    }

    float sx = float(keys['d']) - float(keys['a']);
    float sy = float(keys['s']) - float(keys['w']);
    float dx = sx + sy, dy = sy - sx;
    float length = std::sqrt(dx * dx + dy * dy);
    Position before = player;
    if (length > 0)
    {
        facing = {dx / length, dy / length};
        const bool running = GameInput::SprintHeld();
        const float speed = running ? 6.f : 3.5f;
        map.Move(player, {facing.x * speed * dt, facing.y * speed * dt});
    }
    moving = Distance(before, player) > .001f;
    if (moving)
    {
        walkFrame += Distance(before, player) / 3.5f * CharacterVisual::WalkFramesPerSecond;
    }
    if (keys[CombatKeys::Attack])
    {
        Attack();
    }

    UpdateProjectiles(dt);
    UpdateEnemies(dt);
    UpdateHostileSkills(dt);
    for (auto& text : floating)
    {
        text.lifetime -= dt;
    }
    floating.erase(std::remove_if(floating.begin(), floating.end(),
                                  [](const FloatingText& text)
                                  {
                                      return text.lifetime <= 0;
                                  }),
                   floating.end());
}

void Draw()
{
    auto syncStart = Performance::Clock::now();
    scene.BeginSync();
    DrawMap();
    DrawActors();
    scene.Place("ui", {}, Game::RenderLayer::UserInterface,
                [](const Game::Actor&, const Game::Transform&)
                {
                    DrawHud();
                });
    scene.Place(
        "ui/draw-calls", {}, Game::RenderLayer::UserInterface,
        [](const Game::Actor&, const Game::Transform&)
        {
            // Performance counters remain available in logs and the browser monitor.
        },
        scene.Find("ui"));
    scene.EndSync();
    Performance::Profiler::Get().Sample("cpu.scene.sync_ms", Performance::Milliseconds(syncStart));

    graphics->BeginWorld();
    scene.Render(Game::RenderLayer::Background, Game::RenderLayer::Overlay,
                 [](const Game::Transform& world)
                 {
                     return Visible(Project({world.x, world.y}, world.z), 240 * world.scale);
                 });
    graphics->EndWorld(time);
    scene.Render(Game::RenderLayer::UserInterface, Game::RenderLayer::UserInterface);
    graphics->Flush();
}
} // namespace LevelOne
