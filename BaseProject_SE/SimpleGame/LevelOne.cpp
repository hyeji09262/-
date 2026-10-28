#include "stdafx.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
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
#include "AutoTarget.h"
#include "NatureVisuals.h"
#include "BuildingVisuals.h"
#include "MonsterAnimation.h"
#include "MonsterSkills.h"

namespace LevelOne
{
namespace
{
using Hunting::Distance;
using Hunting::ModelId;
using Hunting::Position;

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
    Position muzzleOffset;
    float age = 0;
    unsigned int visualId = nextVisualId++;
    float launchHeight = 0;
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
float damageTimer = 0;
float navigationTimer = 0;
float walkFrame = 0;
float castTimer = 0;
PlayerActions actions;
float noticeTimer = 0;
bool paused = false;
bool moving = false;
bool initialized = false;
bool saveSucceeded = true;
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
    graphics->Text(x * screenWidth / 1280, y * screenHeight / 800, label, size * screenHeight / 800,
                   color.r, color.g, color.b, color.a);
}

void Model(ModelId id, Position world, float scale = 1, float elevation = 0, bool mirror = false)
{
    Position screen = Project(world, elevation);
    if (Visible(screen, 140 * scale))
    {
        graphics->DrawModel(
            models.Get(id), screen.x * screenWidth / 1280, screen.y * screenHeight / 800,
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
    const int areaBonus = (std::min)(2, int(Distance(enemy.home, map.camp) / 12));
    const float ratio = enemy.maxHealth > 0 ? float(enemy.health) / enemy.maxHealth : 1.f;
    enemy.level = (std::min)(Rpg::LevelCap, progress.stats.level + areaBonus);
    enemy.maxHealth = 18 + enemy.level * 6;
    if (enemy.health > 0)
    {
        enemy.health = (std::max)(1, int(std::ceil(enemy.maxHealth * ratio)));
    }
}

void NewMap()
{
    Performance::Scope profileScope("cpu.map.generate_level_ms");
    scene.Clear();
    map.Generate(random());
    player = map.camp;
    enemies.clear();
    drops.clear();
    projectiles.clear();
    hostileShots.clear();
    poisonZones.clear();
    floating.clear();
    health = static_cast<float>(progress.stats.maxHealth);
    mana = static_cast<float>(progress.stats.maxMana);
    attackTimer = damageTimer = castTimer = navigationTimer = 0;
    actions = PlayerActions{};
    keys.fill(false);

    auto candidates = map.SpawnLocations();
    std::shuffle(candidates.begin(), candidates.end(), random);
    for (const Position& p : candidates)
    {
        bool separated = true;
        for (const auto& enemy : enemies)
        {
            if (Distance(enemy.home, p) < 3)
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
        enemy.kind = static_cast<int>(random() % MonsterAnimation::WildKinds);
        ScaleEnemy(enemy);
        enemy.health = enemy.maxHealth;
        enemies.push_back(enemy);
        if (enemies.size() == 24)
        {
            break;
        }
    }
    navigation = map.Distances(player);
    Notify("마법소녀의 첫 임무! Q 공격 / Space 점프 / X 마나 물약 / E 획득");
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

    AutoTarget::Selection aim(player.x, player.y, facing.x, facing.y, 7.f);
    {
        Performance::Scope scope("cpu.combat.auto_target_ms");
        for (const auto& enemy : enemies)
        {
            const bool candidate =
                enemy.health > 0 && Distance(player, enemy.position) < aim.distance;
            if (candidate)
            {
                aim.Consider(enemy.position.x, enemy.position.y, true,
                             LineOfSight(player, enemy.position));
            }
        }
    }
    Position direction = {aim.directionX, aim.directionY};
    castDirection = direction;

    attackTimer = .5f;
    castTimer = CharacterVisual::CastDuration;
    castMirror = direction.x - direction.y < 0;
    auto socket = CharacterVisual::FacingSocket(true, direction.x, direction.y);
    // Keep the original collision path/range; only the initial visual segment uses the socket.
    projectiles.push_back({player, direction, progress.stats.attack, .85f, {socket.x, socket.y}});
    projectiles.back().launchHeight = actions.height;
}

void Kill(Enemy& enemy)
{
    enemy.health = 0;
    enemy.respawnTimer = 10;
    int reward = 12 + enemy.level * 6;
    bool leveled = progress.Award(reward);
    FloatText(enemy.position, "+" + std::to_string(reward) + " 경험치", teal);

    // Guaranteed coins plus either a health potion or a mana potion.
    if (drops.size() <= 298)
    {
        drops.push_back({enemy.position, 0, 2 + enemy.level});
        drops.push_back({enemy.position, random() % 2 == 0 ? 1 : 3, 1});
    }

    if (leveled)
    {
        Performance::Scope scope("cpu.gameplay.enemy_level_scale_ms");
        for (auto& other : enemies)
        {
            ScaleEnemy(other);
        }
        health = static_cast<float>(progress.stats.maxHealth);
        mana = static_cast<float>(progress.stats.maxMana);
        FloatText(player, "레벨 상승!", gold);
        Notify("레벨 " + std::to_string(progress.stats.level) +
               " 달성! 최대 체력·마나·공격력·방어력이 상승했습니다.");
    }
    Save();
}

void UpdateProjectiles(float dt)
{
    Performance::Scope profileScope("cpu.gameplay.projectiles_ms");
    for (auto& shot : projectiles)
    {
        shot.lifetime -= dt;
        shot.age += dt;
        int steps =
            (std::max)(1, static_cast<int>(std::ceil(CharacterVisual::SpellSpeed * dt / .10f)));
        for (int step = 0; step < steps && shot.lifetime > 0; ++step)
        {
            Position next = {
                shot.position.x + shot.direction.x * CharacterVisual::SpellSpeed * dt / steps,
                shot.position.y + shot.direction.y * CharacterVisual::SpellSpeed * dt / steps};
            if (!map.Walkable(next, .06f))
            {
                shot.lifetime = 0;
                break;
            }
            shot.position = next;
            for (auto& enemy : enemies)
            {
                if (enemy.health > 0 && Distance(shot.position, enemy.position) < .8f)
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
    }
    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
                                     [](const Projectile& shot)
                                     {
                                         return shot.lifetime <= 0;
                                     }),
                      projectiles.end());
}

void ReceiveDamage(int attack)
{
    if (damageTimer > 0 || map.Safe(player))
    {
        return;
    }
    int damage = (std::max)(1, attack - progress.stats.defense);
    health -= damage;
    damageTimer = .8f;
    FloatText(player, "-" + std::to_string(damage), {1, .3f, .4f, 1});
    if (health <= 0)
    {
        player = map.camp;
        health = static_cast<float>(progress.stats.maxHealth);
        mana = static_cast<float>(progress.stats.maxMana);
        navigationTimer = 0;
        projectiles.clear();
        Notify("학교에서 회복했습니다. 경험치와 소지품은 잃지 않습니다.");
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
                enemy.chargeRemaining = 0;
                enemy.wanderTimer = 0;
            }
            continue;
        }

        enemy.attackTimer -= dt;
        const auto skill = MonsterSkills::Get(enemy.kind);
        if (MonsterAnimation::Advance(enemy.attackMotion, dt) && !map.Safe(player))
        {
            const int damage = 4 + enemy.level * 2;
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
                const int count = skill.style == MonsterSkills::Style::Fan ? 3 : 1;
                for (int i = 0; i < count; ++i)
                {
                    const float angle = count == 1 ? 0.f : (i - 1) * .24f;
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
                    ReceiveDamage(6 + enemy.level * 2);
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
            hasSight && skill.range > 1 && Distance(enemy.position, player) < skill.range * .8f;
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
        if (chase && Distance(enemy.position, player) < skill.range && enemy.attackTimer <= 0 &&
            hasSight)
        {
            enemy.attackTimer = skill.cooldown;
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
        Notify("전리품 획득! 골드·체력/마력 물약을 저장했습니다.");
        Save();
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
            Position p = {x + .5f, y + .5f};
            Color ground = map.Safe(p) ? Color{.67f, .60f, .64f, 1} : Color{.34f, .48f, .40f, 1};
            std::string key = std::to_string(Hunting::Map::Index(x, y));
            scene.Place(
                "tile/" + key, {p.x, p.y}, Game::RenderLayer::Ground,
                [tile, ground, x, y](const Game::Actor&, const Game::Transform& world)
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
                        Model(map.Safe({world.x, world.y}) ? ModelId::FloorStone
                                                           : ModelId::TerrainGrass,
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
    auto& sign = scene.Place(
        "return/sign", {map.camp.x - 2, map.camp.y - 1}, Game::RenderLayer::World,
        [](const Game::Actor&, const Game::Transform& world)
        {
            Position p = Project({world.x, world.y});
            Box(p.x - 3, p.y - 42, 6, 42, {.38f, .26f, .21f, 1});
            Box(p.x - 23, p.y - 53, 46, 22, {.57f, .39f, .29f, 1});
            Text(p.x - 17, p.y - 51, "마을", gold, 15);
        },
        &scenery);
    scene.Place(
        "return/label", {0, 0, 83}, Game::RenderLayer::Overlay,
        [](const Game::Actor&, const Game::Transform& world)
        {
            if (Distance(player, {world.x, world.y}) > 5.5f)
            {
                return;
            }
            Position screen = Project({world.x, world.y}, world.z);
            if (Visible(screen, 0))
            {
                graphics->Nameplate(screen.x * screenWidth / 1280, screen.y * screenHeight / 800,
                                    "[E] 마을로 돌아가기", 17.f * screenHeight / 800);
            }
        },
        &sign);
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
            const auto skill = MonsterSkills::Get(enemy.kind);
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
            if (skill.style == MonsterSkills::Style::Charge)
            {
                scene.Place(
                    name + "/charge-path", {}, Game::RenderLayer::Ground,
                    [enemy](const Game::Actor&, const Game::Transform& world)
                    {
                        for (int i = 1; i <= 7; ++i)
                        {
                            const Position p = Project({world.x + enemy.facing.x * i * .5f,
                                                        world.y + enemy.facing.y * i * .5f});
                            Diamond(p, 7, 4, {1, .36f, .4f, .75f});
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
                                                     shot.direction.x, shot.direction.y);
        pose.z += shot.launchHeight * (1 - (std::min)(1.f, shot.age / .18f));
        PlaceEffect("shot/" + std::to_string(shot.visualId), pose, CharacterVisual::SpellSize,
                    CharacterVisual::SpellSize, 2, &spells);
    }
    DrawHostileSkills();
}

void DrawHud()
{
    Box(18, 18, 820, 136, panel);
    Text(35, 48, "마법소녀 · 방과 후 첫 임무", gold, 25);
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
    Text(34, 560, "[Z] " + std::to_string(progress.potions) + "개", white, 15);
    Text(98, 560, "[X] " + std::to_string(progress.manaPotions) + "개", white, 15);

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
    sprintf_s(line, "골드 %d    체력 물약 %d [Z]    마력 물약 %d [X]    %s", progress.coins,
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
    Text(35, 738, "WASD 이동 · Space 점프 · Q 공격 · X 마나 물약 · E 획득 · Z 체력 물약 · ESC 메뉴",
         gold, 17);
    std::string cache = models.loadedFromFile ? "모델: 파일 캐시 로딩"
                        : models.storedToFile ? "모델: 최초 생성·캐시 저장 완료"
                                              : "모델: 캐시 쓰기 실패";
    Text(35, 769, cache + (saveSucceeded ? "   |   성장 자동 저장" : "   |   성장 저장 실패"),
         white, 15);
    if (noticeTimer > 0)
    {
        Box(250, 655, 1012, 45, panel);
        Text(267, 685, notice, teal, 18);
    }
    if (paused)
    {
        Box(0, 0, 1280, 800, {.02f, .025f, .04f, .78f});
        Box(270, 250, 740, 275, panel);
        Text(307, 298, "학교 주변 메뉴", gold, 26);
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
    bool loaded = progress.Load();
    progress.Recalculate();
    random.seed(std::random_device{}());
    initialized = true;
    NewMap();
    if (loaded)
    {
        Notify("저장된 경험치와 소지품을 불러왔습니다. 새로운 사냥을 시작하세요.");
    }
}

WorldMonitor::Scene MonitorSnapshot()
{
    WorldMonitor::Scene snapshot;
    snapshot.id = "hunting";
    snapshot.name = "사냥터 · 학교 주변";
    snapshot.paused = paused;
    snapshot.objective = "몬스터 사냥 · 경험치 획득";
    for (const auto& enemy : enemies)
    {
        enemy.health > 0 ? ++snapshot.mobsAlive : ++snapshot.mobsDead;
    }
    snapshot.towns.push_back(
        {"hunting_camp", "학교 주변 사냥 지역", "안전 거점과 야생 구역 공존", {}});
    return snapshot;
}

void Resize(int width, int height)
{
    screenWidth = (std::max)(1, width);
    screenHeight = (std::max)(1, height);
}

void Save()
{
    Performance::Scope profileScope("cpu.storage.save_progress_ms");
    if (initialized)
    {
        saveSucceeded = progress.Save();
    }
}

void Pause()
{
    paused = true;
    keys.fill(false);
    Save();
}

bool ConsumeWorldRequest()
{
    const bool requested = worldRequested;
    worldRequested = false;
    return requested;
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
    if (key == 27)
    {
        paused = !paused;
        keys.fill(false);
        Save();
        return;
    }
    if (paused)
    {
        if (key == 'n')
        {
            NewMap();
            paused = false;
        }
        return;
    }
    if (key == 'e')
    {
        if (Distance(player, {map.camp.x - 2, map.camp.y - 1}) < 1.8f)
        {
            worldRequested = true;
            return;
        }
        PickUp();
    }
    if (key == ' ')
    {
        actions.Jump();
    }
    if (key == 'x' && health > 0)
    {
        if (actions.UseManaPotion(mana, float(progress.stats.maxMana), progress.manaPotions))
        {
            FloatText(player, "마나 회복", teal);
            Save();
        }
    }
    if (key == 'z' && progress.potions > 0 && health < progress.stats.maxHealth)
    {
        --progress.potions;
        health =
            (std::min)(float(progress.stats.maxHealth), health + progress.stats.maxHealth * .45f);
        FloatText(player, "체력 회복", teal);
        Save();
    }
}

void Update(float dt)
{
    Performance::Scope profileScope("cpu.gameplay.level_update_ms");
    if (!initialized || paused)
    {
        return;
    }
    time += dt;
    actions.Update(dt, mana, float(progress.stats.maxMana));
    attackTimer -= dt;
    damageTimer -= dt;
    castTimer -= dt;
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
        const bool running =
            GetActiveWindow() == GetForegroundWindow() && (GetAsyncKeyState(VK_SHIFT) & 0x8000);
        const float speed = running ? 6.f : 3.5f;
        map.Move(player, {facing.x * speed * dt, facing.y * speed * dt});
    }
    moving = Distance(before, player) > .001f;
    if (moving)
    {
        walkFrame += Distance(before, player) / 3.5f * CharacterVisual::WalkFramesPerSecond;
    }
    if (keys['q'])
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
