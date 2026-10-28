#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include "Renderer.h"
#include "PrimitiveModels.h"
#include "SceneGraph.h"
#include "CharacterVisual.h"
#include "PlayerActions.h"
#include "WorldMonitor.h"
#include "VillageWorld.h"
#include "SchoolScene.h"
#include "AutoTarget.h"
#include "NatureVisuals.h"
#include "Prologue.h"
#include "QuestTwo.h"
#include "VillageInterior.h"
#include "TownPopulation.h"
#include "MonsterAnimation.h"
#include "Dependencies/freeglut.h"

namespace Tutorial
{
struct Vec
{
    float x, y;
};

struct Color
{
    float r, g, b, a;
};

struct Prop
{
    Vec p;
    int kind;
    float radius;
};

struct NPC
{
    Vec p;
    const char* name;
    const char* line;
};

struct Monster
{
    Vec p, home;
    int hp, kind;
    float cooldown, phase;
    float attackMotion = 0;
    float respawn = 0;
    Vec facing{.70710678f, -.70710678f};
};

struct Particle
{
    Vec p, velocity;
    float z, vz, life, total;
    Color color;
};

struct SpellVisual
{
    Vec origin, direction;
    float age, lifetime;
    bool mirror;
    unsigned int id;
    float launchHeight = 0;
};

static std::vector<SpellVisual> spells;
static unsigned int nextSpellId = 0;

static const int MapWidth = CoastalWorld::Width, MapHeight = CoastalWorld::Height,
                 LandWidth = CoastalWorld::LandWidth;
static std::array<NPC, 30> villagers = []()
{
    std::array<NPC, 30> result{};
    for (size_t i = 0; i < result.size(); ++i)
    {
        const auto& npc = CoastalWorld::Residents()[i];
        const auto& location = TownPopulation::ResidentPositions[i % 10];
        static const char* names[] = {"주민 소미", "주민 서진", "주민 루아", "주민 무영",
                                      "주민 은",   "주민 보리", "주민 별",   "주민 아린",
                                      "주민 누리", "주민 다온"};
        result[i] = {{location[0], CoastalWorld::TownCenterY(int(i / 10)) + location[1]},
                     names[i % 10],
                     npc.line};
    }
    return result;
}();
static constexpr int NPCCount = 30;
static Renderer* renderer = nullptr;
static Game::SceneGraph scene;
static bool visualActive = false;
static Game::Transform visualSource, visualWorld;
static bool castMirror = false;
static int width = 1280, height = 800, stage = 0, hp = 5, bossHP = 10, upgrade = 0, kills = 0;
static Vec player = {31, 26}, boss = {30, 16}, shrine = {43, 26}, beamTarget = {31, 26};
static Vec facingDirection = {.70710678f, -.70710678f};
static Vec castDirection = {.70710678f, -.70710678f};
static bool keys[256] = {}, paused = false, moving = false;
static PlayerActions actions;
static int potions = 5;
static int manaPotions = 5;
static bool shopOpen = false;
static float elapsed = 0, clockTime = 0, attackCD = 0, hurtCD = 0, pulse = 0, messageTime = 0,
             finishTime = 0;
static float walkCycle = 0, healTimer = 0, castPose = 0;
static std::string message;
static std::vector<Prop> props;
static constexpr int CollisionColumns = (MapWidth + 3) / 4;
static std::array<std::vector<size_t>, CollisionColumns*((MapHeight + 3) / 4)> propCells;
static unsigned collisionTests = 0;
static std::vector<Monster> monsters;
static std::vector<Particle> particles;
static const Color ink = {.055f, .058f, .10f, 1}, gold = {1, .76f, .39f, 1},
                   teal = {.35f, .95f, .84f, 1};

static float mana = 30.f;
static bool exploration = false;
static bool regionsUnlocked = false;
static bool storyMagicUnlocked = false;
static int interior = -1;
static int interiorRoom = 0;
static Vec outdoorReturn = {31, 26};
static std::array<int, 3> townKills{};
static unsigned spawnCursor = 0;
static std::vector<Vec> spawnSites;
static int sceneRequest = 0; // 1: current story, 2: school quest.

inline bool MagicUnlocked()
{
    return exploration ? storyMagicUnlocked : stage >= 1;
}

inline float Dist(Vec a, Vec b)
{
    float x = a.x - b.x, y = a.y - b.y;
    return std::sqrt(x * x + y * y);
}

inline WorldMonitor::Scene MonitorSnapshot();

inline Vec Project(Vec p, float z = 0)
{
    return {640 + (p.x - p.y - player.x + player.y) * 32,
            430 + (p.x + p.y - player.x - player.y) * 16 - z};
}

inline Vec VisualPoint(Vec p)
{
    if (!visualActive)
    {
        return p;
    }
    Vec source = Project({visualSource.x, visualSource.y}, visualSource.z);
    Vec target = Project({visualWorld.x, visualWorld.y}, visualWorld.z);
    return {target.x + (p.x - source.x) * visualWorld.scale,
            target.y + (p.y - source.y) * visualWorld.scale};
}

inline bool Visible(Vec s, float margin = 130)
{
    s = VisualPoint(s);
    return s.x > -margin && s.x < 1280 + margin && s.y > -margin && s.y < 800 + margin;
}

inline void Tri(Vec a, Vec b, Vec c, Color col)
{
    a = VisualPoint(a);
    b = VisualPoint(b);
    c = VisualPoint(c);
    renderer->Triangle(a.x * width / 1280, a.y * height / 800, b.x * width / 1280,
                       b.y * height / 800, c.x * width / 1280, c.y * height / 800, col.r, col.g,
                       col.b, col.a);
}

inline void Box(float x, float y, float w, float h, Color c)
{
    Tri({x, y}, {x + w, y}, {x + w, y + h}, c);
    Tri({x, y}, {x + w, y + h}, {x, y + h}, c);
}

inline void Quad(Vec a, Vec b, Vec c, Vec d, Color col)
{
    Tri(a, b, c, col);
    Tri(a, c, d, col);
}

inline void Diamond(Vec p, float w, float h, Color c)
{
    Tri({p.x - w, p.y}, {p.x, p.y - h}, {p.x + w, p.y}, c);
    Tri({p.x - w, p.y}, {p.x + w, p.y}, {p.x, p.y + h}, c);
}

inline void Ellipse(Vec p, float w, float h, Color c, int segments = 18)
{
    for (int i = 0; i < segments; ++i)
    {
        float a = i * 6.283185f / segments, b = (i + 1) * 6.283185f / segments;
        Tri(p, {p.x + std::cos(a) * w, p.y + std::sin(a) * h},
            {p.x + std::cos(b) * w, p.y + std::sin(b) * h}, c);
    }
}

inline void Line(Vec a, Vec b, float w, Color c)
{
    float dx = b.x - a.x, dy = b.y - a.y, len = std::sqrt(dx * dx + dy * dy);
    if (len < .01f)
        return;
    Vec n = {-dy / len * w * .5f, dx / len * w * .5f};
    Quad({a.x + n.x, a.y + n.y}, {b.x + n.x, b.y + n.y}, {b.x - n.x, b.y - n.y},
         {a.x - n.x, a.y - n.y}, c);
}

inline void Glow(Vec p, float w, float h, Color c)
{
    for (int i = 5; i >= 1; --i)
    {
        float t = i / 5.f;
        Color layer = c;
        layer.a = c.a * .14f;
        Ellipse(p, w * t, h * t, layer, 16);
    }
}

inline void Shadow(Vec p, float w, float h, float length)
{
    // Cast shadows are all drawn in a ground pass, before any upright objects.
    Vec s = Project(p);
    for (int i = 4; i >= 0; --i)
    {
        float spread = 1 + i * .13f;
        Ellipse({s.x - length * .55f, s.y + length * .21f}, w * spread + length * .45f, h * spread,
                {.015f, .024f, .047f, .035f}, 16);
    }
    Ellipse(s, w * .6f, h * .65f, {.012f, .02f, .03f, .18f}, 14);
}

inline void Text(float x, float y, const std::string& text, Color c = gold, float size = 18)
{
    Vec screen = VisualPoint({x, y});
    float scale = visualActive ? visualWorld.scale : 1.f;
    renderer->Text(screen.x * width / 1280, screen.y * height / 800, text,
                   size * scale * height / 800, c.r, c.g, c.b, c.a);
}

inline void Say(const std::string& line)
{
    message = line;
    messageTime = 8;
}

inline bool Path(int x, int y)
{
    return CoastalWorld::Paved(x, y) || (std::abs(x - 19) <= 1 && y >= 9 && y <= 37) ||
           (std::abs(y - 36) <= 1 && x >= 18 && x <= 35);
}

inline bool Safe(Vec p)
{
    return interior >= 0 || CoastalWorld::Safe(p.x, p.y) || CoastalWorld::SchoolGround(p.x, p.y);
}

inline bool Walkable(Vec p)
{
    if (interior >= 0)
    {
        if (CoastalWorld::Buildings()[interior].interior == 1)
        {
            return SchoolLayout::Walkable(interiorRoom, p.x, p.y);
        }
        return VillageInterior::Walkable(CoastalWorld::Buildings()[interior].interior, p.x, p.y);
    }
    if (p.x < .7f || p.x > LandWidth - .5f || p.y < .7f || p.y > MapHeight - 1.5f)
        return false;
    if (exploration && !regionsUnlocked && p.y > 60)
    {
        return false;
    }
    for (const auto& building : CoastalWorld::Buildings())
    {
        if (CoastalWorld::Blocks(building, p.x, p.y))
        {
            return false;
        }
    }
    const int cellX = int(p.x) / 4;
    const int cellY = int(p.y) / 4;
    for (int y = (std::max)(0, cellY - 1); y <= (std::min)((MapHeight - 1) / 4, cellY + 1); ++y)
    {
        for (int x = (std::max)(0, cellX - 1); x <= (std::min)(CollisionColumns - 1, cellX + 1);
             ++x)
        {
            for (size_t index : propCells[y * CollisionColumns + x])
            {
                ++collisionTests;
                const auto& prop = props[index];
                if (Dist(p, prop.p) < prop.radius + .22f)
                {
                    return false;
                }
            }
        }
    }
    return true;
}

inline void Move(Vec& p, Vec delta)
{
    Vec q = {p.x + delta.x, p.y};
    if (Walkable(q))
        p = q;
    q = {p.x, p.y + delta.y};
    if (Walkable(q))
        p = q;
}

#include "VillageRuntime.h"

inline void Burst(Vec p, int amount, Color color, float z = 22)
{
    for (int i = 0; i < amount && particles.size() < 320; ++i)
    {
        float angle = i * 2.399963f + clockTime, life = .35f + (i % 7) * .08f;
        particles.push_back({p,
                             {std::cos(angle) * 1.7f, std::sin(angle) * 1.7f},
                             z,
                             16.f + (i % 5) * 7,
                             life,
                             life,
                             color});
    }
}

inline void Reset()
{
    interior = -1;
    interiorRoom = 0;
    sceneRequest = 0;
    townKills.fill(0);
    spawnCursor = 0;
    actions = PlayerActions{};
    mana = 30.f;
    potions = 5;
    manaPotions = 5;
    scene.Clear();
    player = {31, 26};
    facingDirection = {.70710678f, -.70710678f};
    castDirection = facingDirection;
    stage = 0;
    hp = 5;
    bossHP = 10;
    upgrade = kills = 0;
    paused = moving = false;
    elapsed = clockTime = attackCD = hurtCD = pulse = finishTime = walkCycle = healTimer =
        castPose = 0;
    std::fill(keys, keys + 256, false);
    props.clear();
    monsters.clear();
    particles.clear();
    spells.clear();
    Say("별빛 마을에 도착했습니다. 선생님과 대화하면 마법소녀 연습을 시작합니다. F6은 스토리 마을 탐험입니다.");
    for (int town = 0; town < CoastalWorld::TownCount; ++town)
    {
        const float y = CoastalWorld::TownCenterY(town) + 6;
        for (float side : {-1.f, 1.f})
        {
            for (float offset : {-5.f, 0.f, 5.f})
            {
                props.push_back({{49 + side * 6, y + offset}, 0, .6f});
            }
            props.push_back({{49 + side * 4, y - 2}, 6, .7f});
            props.push_back({{49 + side * 4, y + 3}, 6, .7f});
            props.push_back({{49 + side * 2, y + 6}, 7, .2f});
        }
    }
    // Keep residents and the tutorial corridor free of decorative collision.
    props.erase(std::remove_if(props.begin(), props.end(),
                               [](const Prop& p)
                               {
                                   for (const auto& npc : villagers)
                                       if (Dist(npc.p, p.p) < p.radius + .5f)
                                           return true;
                                   return false;
                               }),
                props.end());
    for (int y = 2; y < MapHeight - 2; ++y)
        for (int x = 2; x < LandWidth - 2; ++x)
        {
            if ((x * 17 + y * 31) % 9 != 0 || Path(x, y))
                continue;
            Vec p = {float(x), float(y)};
            if (CoastalWorld::Safe(p.x, p.y) || CoastalWorld::SchoolGround(p.x, p.y) ||
                Dist(p, boss) < 4 || Dist(p, shrine) < 3)
                continue;
            bool reserved = false;
            for (const auto& building : CoastalWorld::Buildings())
                if (Dist(p, {building.x, building.y}) < 4)
                    reserved = true;
            if (!reserved)
                props.push_back({p, (x + y) % 7 == 0 ? 9 : 0, .42f});
        }
    for (auto& cell : propCells)
    {
        cell.clear();
    }
    for (size_t i = 0; i < props.size(); ++i)
    {
        propCells[int(props[i].p.y) / 4 * CollisionColumns + int(props[i].p.x) / 4].push_back(i);
    }
    BuildSpawnSites();
}

inline int NearestNPC()
{
    if (interior >= 0)
    {
        return -1;
    }
    int result = -1;
    float distance = 1.65f;
    for (int i = 0; i < NPCCount; ++i)
        if (Dist(player, villagers[i].p) < distance)
        {
            result = i;
            distance = Dist(player, villagers[i].p);
        }
    return result;
}

inline void Interact()
{
    if (exploration && interior < 0)
    {
        float x, y;
        int person;
        std::string objective;
        if (Prologue::WorldObjective(x, y, person, objective) && person != 3 &&
            Dist(player, {x, y}) < 1.8f)
        {
            Say(Prologue::InteractWorld());
            if (!Prologue::WorldObjective(x, y, person, objective))
            {
                sceneRequest = 1;
            }
            return;
        }
    }
    if (!exploration && (stage == 2 || stage == 5))
        return;
    if (BuildingInteraction())
    {
        return;
    }
    if (exploration)
    {
        const Vec guard = {TownPopulation::GuardX, TownPopulation::GuardY + CurrentTown() * 70.f};
        if (interior < 0 && Dist(player, guard) < 1.8f)
        {
            Say("경비원 단: 학교 앞 길을 살피고 있습니다.");
            return;
        }
        const int npc = NearestNPC();
        if (npc >= 0)
        {
            Say(npc == 0
                    ? "집과 학교를 둘러보세요. 별빛 학교 로비에서 친구들의 이야기를 이어갈 수 있어요."
                    : villagers[npc].line);
        }
        return;
    }
    if (Dist(player, shrine) < (upgrade == 1 ? 4.f : 2.1f))
    {
        if (stage == 3)
        {
            stage = 4;
            Burst(shrine, 45, gold, 45);
            Say("악몽 봉인이 풀렸습니다! 별빛 조각을 되찾았어요. 선생님에게 돌아가세요.");
        }
        else if (stage < 3)
            Say("악몽 봉인입니다. 악몽 교감을 물리치고 마법을 먼저 배워야 합니다.");
        else
            Say("별빛 조각은 주머니에 안전하게 있습니다. 선생님에게 돌아가세요.");
        return;
    }
    int i = NearestNPC();
    if (i < 0)
        return;
    if (i == 0 && stage == 0)
    {
        stage = 1;
        Say(villagers[0].line);
    }
    else if (i == 0 && stage == 4)
    {
        stage = 5;
        finishTime = elapsed;
        Say("모라: 학교를 구했군요! 숙제 면제는 아니지만요. 고마워요, 마법소녀!");
    }
    else if (i == 0 && stage == 3)
        Say("동쪽 해안 별빛 장치에 별빛 마법을 써서 별빛 조각을 찾아주세요.");
    else
        Say(villagers[i].line);
}

inline void Key(unsigned char key, bool down)
{
    if (key >= 'A' && key <= 'Z')
        key = key - 'A' + 'a';
    bool fresh = down && !keys[key];
    keys[key] = down;
    if (!fresh)
        return;
    if (shopOpen)
    {
        if (key == 27 || key == 'e')
        {
            shopOpen = false;
        }
        else if (key == '1' || key == '2')
        {
            std::string message;
            QuestTwo::BuyPotion(key == '2', message);
            Rpg::Progress inventory;
            if (QuestTwo::ShopProgress(inventory))
            {
                potions = inventory.potions;
                manaPotions = inventory.manaPotions;
            }
            Say(message);
        }
        return;
    }
    if (key == 27)
    {
        paused = !paused;
        return;
    }
    if (paused)
        return;
    if (!exploration && key == 'n' && stage == 5)
    {
        Reset();
        return;
    }
    if (exploration || (stage != 2 && stage != 5))
    {
        if (key == ' ')
        {
            actions.Jump();
        }
        if (key == 'x' && hp > 0)
        {
            if (actions.UseManaPotion(mana, 30.f, manaPotions))
            {
                Say("마나 물약으로 마나를 회복했습니다.");
                QuestTwo::StoreWorldPotions(potions, manaPotions);
            }
        }
        if (key == 'z' && potions > 0 && hp > 0 && hp < 5)
        {
            --potions;
            hp = (std::min)(5, hp + 2);
            Say("물약으로 체력을 회복했습니다.");
            QuestTwo::StoreWorldPotions(potions, manaPotions);
        }
    }
    if (!exploration && stage == 2 && (key == '1' || key == '2'))
    {
        upgrade = key - '0';
        stage = 3;
        Burst(player, 30, teal, 30);
        Say(upgrade == 1
                ? "먼 손길을 배웠습니다! 해안 별빛 장치를 4타일 거리에서 열 수 있어요."
                : "가벼운 발을 배웠습니다! 이동 속도가 올랐어요. 해안 별빛 장치로 가세요.");
    }
    if (key == 'e')
        Interact();
}

inline void Damage()
{
    if (hurtCD > 0)
        return;
    --hp;
    hurtCD = 1.2f;
    Burst(player, 14, {1, .3f, .35f, 1});
    if (hp <= 0)
    {
        player = {31, 26};
        hp = 5;
        bossHP = 10;
        pulse = 0;
        healTimer = 0;
        for (auto& m : monsters)
        {
            m.p = m.home;
            m.cooldown = 1;
        }
        Say("주민들이 구해 주었습니다. 퀘스트는 유지됩니다. 붉은 공격 범위를 피하세요.");
    }
}

inline void Update(float dt)
{
    Performance::Scope scope(exploration ? "cpu.world.update_ms"
                                         : "cpu.gameplay.tutorial_update_ms");
    collisionTests = 0;
    if (paused || shopOpen || (!exploration && (stage == 2 || stage == 5)))
        return;
    if (exploration && interior >= 0)
    {
        const auto& building = CoastalWorld::Buildings()[interior];
        int room;
        float x, y;
        std::string label;
        if (building.interior == 1 && building.town == 0 &&
            QuestTwo::SchoolObjective(interiorRoom, player.x, player.y, room, x, y, label) &&
            room == interiorRoom)
        {
            sceneRequest = 3;
            return;
        }
    }
    SyncWildlife();
    elapsed += dt;
    actions.Update(dt, mana, 30.f);
    clockTime += dt;
    attackCD -= dt;
    hurtCD -= dt;
    for (auto& spell : spells)
    {
        spell.age += dt;
    }
    spells.erase(std::remove_if(spells.begin(), spells.end(),
                                [](const SpellVisual& spell)
                                {
                                    return spell.age >= spell.lifetime;
                                }),
                 spells.end());
    castPose -= dt;
    messageTime -= dt;
    float sx = float(keys['d']) - float(keys['a']), sy = float(keys['s']) - float(keys['w']);
    float dx = sx + sy, dy = sy - sx, len = std::sqrt(dx * dx + dy * dy);
    Vec old = player;
    if (len > 0)
    {
        float speed = exploration ? CoastalWorld::WalkSpeed : upgrade == 2 ? 4.5f : 3.5f;
        if (exploration && GetActiveWindow() == GetForegroundWindow() &&
            (GetAsyncKeyState(VK_SHIFT) & 0x8000))
        {
            speed = 5.f;
        }
        Move(player, {dx / len * speed * dt, dy / len * speed * dt});
        facingDirection = {dx / len, dy / len};
    }
    moving = Dist(old, player) > .001f;
    if (moving)
        walkCycle += dt * CharacterVisual::WalkFramesPerSecond * 1.570796f;
    if (Safe(player) && hp < 5)
    {
        healTimer += dt;
        if (healTimer > 2)
        {
            ++hp;
            healTimer = 0;
            Burst(player, 8, teal);
        }
    }
    else
        healTimer = 0;
    for (auto& m : monsters)
    {
        if (interior >= 0)
        {
            break;
        }
        if (m.hp <= 0)
        {
            m.respawn -= dt;
            if (m.respawn <= 0)
            {
                Respawn(m, false);
            }
            continue;
        }
        m.cooldown -= dt;
        if (MonsterAnimation::Advance(m.attackMotion, dt) && !Safe(player) &&
            Dist(player, m.p) < 1.1f)
        {
            Damage();
        }
        m.phase += dt;
        bool chase = !Safe(player) && Dist(player, m.p) < 5.5f && Dist(player, m.home) < 8;
        Vec goal = chase ? player
                         : Vec{m.home.x + std::sin(m.phase * .7f) * 1.1f,
                               m.home.y + std::cos(m.phase * .5f) * 1.1f};
        float distance = Dist(m.p, goal);
        if (distance > .45f && m.attackMotion <= 0)
        {
            float speed = chase ? (m.kind == 0 ? 1.65f : 2.1f) : .5f;
            Vec before = m.p;
            Move(m.p, {(goal.x - m.p.x) / distance * speed * dt,
                       (goal.y - m.p.y) / distance * speed * dt});
            if (Safe(m.p))
                m.p = before;
            const float traveled = Dist(m.p, before);
            if (traveled > .001f)
            {
                m.facing = {(m.p.x - before.x) / traveled, (m.p.y - before.y) / traveled};
            }
        }
        if (!Safe(player) && Dist(player, m.p) < .9f && m.cooldown <= 0)
        {
            m.cooldown = 1.8f;
            m.attackMotion = MonsterAnimation::Duration;
            const float targetDistance = Dist(player, m.p);
            if (targetDistance > .001f)
            {
                m.facing = {(player.x - m.p.x) / targetDistance,
                            (player.y - m.p.y) / targetDistance};
            }
        }
    }
    if (!exploration && interior < 0 && stage == 1 && Dist(player, boss) < 6.5f)
    {
        pulse += dt;
        if (pulse >= 3.2f)
        {
            pulse = 0;
            Burst(boss, 35, {1, .2f, .25f, 1});
            if (Dist(player, boss) < 3)
                Damage();
        }
    }
    else
        pulse = 0;
    if (MagicUnlocked() && interior < 0 && keys['q'] && attackCD <= 0 && actions.Fire(mana))
    {
        attackCD = .5f;
        castPose = CharacterVisual::CastDuration;
        AutoTarget::Selection aim(player.x, player.y, facingDirection.x, facingDirection.y, 6.5f);
        int target = -1;
        bool hitBoss = false;
        {
            Performance::Scope scope("cpu.combat.auto_target_ms");
            if (!exploration && stage == 1)
            {
                hitBoss = aim.Consider(boss.x, boss.y, bossHP > 0);
            }
            for (int i = 0; i < int(monsters.size()); ++i)
            {
                if (aim.Consider(monsters[i].p.x, monsters[i].p.y, monsters[i].hp > 0))
                {
                    target = i;
                    hitBoss = false;
                }
            }
        }
        castDirection = {aim.directionX, aim.directionY};
        beamTarget =
            aim.found ? Vec{aim.x, aim.y}
                      : Vec{player.x + castDirection.x * 6.5f, player.y + castDirection.y * 6.5f};
        castMirror = castDirection.x - castDirection.y < 0;
        float spellDistance = (std::max)(.01f, Dist(player, beamTarget));
        spells.push_back(
            {player,
             {(beamTarget.x - player.x) / spellDistance, (beamTarget.y - player.y) / spellDistance},
             0,
             (std::max)(.05f, spellDistance / CharacterVisual::SpellSpeed),
             castMirror,
             nextSpellId++,
             actions.height});
        Burst(beamTarget, 16, teal, 25);
        if (hitBoss)
        {
            if (--bossHP <= 0)
            {
                stage = 2;
                Burst(boss, 40, gold, 40);
                Say("악몽 교감을 물리쳤습니다! 마법 핵심과 스킬 포인트를 얻었습니다.");
            }
        }
        else if (target >= 0 && --monsters[target].hp <= 0)
        {
            ++kills;
            ++townKills[CoastalWorld::NearestTown(monsters[target].p.y)];
            monsters[target].respawn = CoastalWorld::RespawnSeconds;
            manaPotions = (std::min)(99, manaPotions + 1);
            QuestTwo::StoreWorldPotions(potions, manaPotions);
            hp = (std::min)(5, hp + 1);
            Burst(monsters[target].p, 24, gold);
        }
    }
    for (auto& p : particles)
    {
        p.life -= dt;
        p.p.x += p.velocity.x * dt;
        p.p.y += p.velocity.y * dt;
        p.z += p.vz * dt;
        p.vz -= 65 * dt;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
                                   [](const Particle& p)
                                   {
                                       return p.life <= 0;
                                   }),
                    particles.end());
    Performance::Profiler::Get().Gauge("world.collision_tests_per_tick_count", collisionTests);
    Performance::Profiler::Get().Gauge("world.wildlife_slots_count", double(monsters.size()));
}

inline Vec Target()
{
    float x, y;
    int person;
    std::string objective;
    if (exploration && Prologue::WorldObjective(x, y, person, objective))
    {
        return {x, y};
    }
    if (stage == 0 || stage >= 4)
        return villagers[0].p;
    if (stage < 3)
        return boss;
    return shrine;
}
} // namespace Tutorial

#include "TutorialArt.h"

namespace Tutorial
{
inline void DrawHud()
{
    if (shopOpen)
    {
        Rpg::Progress inventory;
        QuestTwo::ShopProgress(inventory);
        Box(340, 260, 610, 230, {.06f, .07f, .13f, .96f});
        Text(370, 299, "별빛 편의점 · 물약 판매", gold, 24);
        Text(370, 344, "[1] 체력물약 20골드 · 보유 " + std::to_string(potions), gold, 19);
        Text(370, 384, "[2] 마나물약 25골드 · 보유 " + std::to_string(manaPotions), teal, 19);
        Text(370, 429, "골드 " + std::to_string(inventory.coins), gold, 19);
        Text(370, 466, "[E / ESC] 닫기", teal, 16);
    }
    // The HUD is deliberately outside the color grading / bloom pass.
    Vec target = Project(Target(), 85);
    float tx = (std::max)(35.f, (std::min)(1245.f, target.x));
    float ty = (std::max)(167.f, (std::min)(643.f, target.y));
    if (!exploration && interior < 0)
    {
        Diamond({tx, ty + std::sin(clockTime * 3) * 4}, 7, 12, gold);
    }
    Box(20, 20, 730, 118, {ink.r, ink.g, ink.b, .94f});
    Box(20, 20, 4, 118, gold);
    Text(38, 49,
         interior >= 0 ? (CoastalWorld::Buildings()[interior].interior == 1
                              ? std::string(CoastalWorld::Buildings()[interior].name) + " · " +
                                    SchoolLayout::Get(interiorRoom).name
                              : CoastalWorld::Buildings()[interior].name)
         : exploration ? CoastalWorld::TownName(CurrentTown())
                       : "별빛 학교  ·  갑자기 마법소녀!",
         gold, 23);
    const char* objectives[] = {
        "01  담임 모라에게 말 걸기 [E]",         "02  악몽 교감 처치하기 [Q]",
        "03  스킬 강화 선택하기 [1 / 2]",        "04  해안 별빛 장치의 봉인 풀기 [E]",
        "05  선생님에게 별빛 조각 전달하기 [E]", "완료  ·  학교에 일상이 돌아왔습니다"};
    float objectiveX, objectiveY;
    int objectivePerson;
    std::string worldObjective;
    const bool hasWorldObjective =
        exploration &&
        Prologue::WorldObjective(objectiveX, objectiveY, objectivePerson, worldObjective);
    Text(38, 81,
         hasWorldObjective ? worldObjective.c_str()
         : interior >= 0   ? "노란 안내 지점에서 [E] 조사 / 이동 · 출입구로 마을 복귀"
         : exploration
             ? (MagicUnlocked() ? "숲의 악몽 처치 · 세 마을 탐험 · 학교에서 사건 이어가기"
                                : "집과 별빛 학교에서 이야기 진행 · 변신 전에는 악몽이 없습니다")
             : objectives[stage],
         gold, 18);
    char status[256];
    sprintf_s(status, "시간 %02d:%02d    목표 %.0f타일    야생 몬스터 처치 %d", int(elapsed) / 60,
              int(elapsed) % 60, Dist(player, Target()), kills);
    Text(38, 114,
         exploration ? "F1 마을 · F2 사냥터 · F3 이야기 · F4 학교 사건 · F6 마을 탐험" : status,
         {.8f, .83f, .85f, 1}, 16);
    Box(22, 212, 152, 354, {ink.r, ink.g, ink.b, .96f});
    const float healthRatio = (std::max)(0.f, (std::min)(1.f, hp / 5.f));
    const float manaRatio = (std::max)(0.f, (std::min)(1.f, mana / 30.f));
    Text(41, 231, "체력", gold, 18);
    Text(105, 231, "마나", gold, 18);
    Box(48, 243, 22, 280, {.22f, .23f, .30f, 1});
    Box(48, 243 + 280 * (1 - healthRatio), 22, 280 * healthRatio, {.95f, .40f, .58f, 1});
    Box(112, 243, 22, 280, {.22f, .23f, .30f, 1});
    Box(112, 243 + 280 * (1 - manaRatio), 22, 280 * manaRatio, {.52f, .65f, .98f, 1});
    Text(34, 542, std::to_string(hp) + "/5", gold, 14);
    Text(98, 542, std::to_string(int(mana)) + "/30", gold, 14);
    Text(34, 560, "[Z] " + std::to_string(potions) + "개", gold, 15);
    Text(98, 560, "[X] " + std::to_string(manaPotions) + "개", gold, 15);
    Text(970, 38, exploration ? "싱글플레이 · 마을 탐험" : "싱글플레이 · 튜토리얼", gold, 16);
    Text(970, 62,
         interior >= 0   ? "건물 내부 · 안전 지역"
         : Safe(player)  ? "마을 중심 · 안전 지역"
         : player.x > 41 ? "별빛 해안"
                         : "안개 숲 · 야생 지역",
         teal, 16);
    if (interior < 0)
    {
        DrawMinimap();
    }
    else if (CoastalWorld::Buildings()[interior].interior == 1)
    {
        DrawSchoolMinimap();
    }
    Box(20, 748, 1240, 34, {ink.r, ink.g, ink.b, .94f});
    Text(36, 772,
         "WASD 이동 · Shift 달리기 · Space 점프 · Q 공격 · X 마나 · Z 체력 · E 출입/대화 · F6 마을",
         gold, 17);
    int nearestNpcIndex = NearestNPC();
    if (interior < 0)
    {
        for (const auto& building : CoastalWorld::Buildings())
        {
            if (Dist(player, {CoastalWorld::DoorX(building), CoastalWorld::DoorY(building)}) < 1.5f)
            {
                Text(480, 585, std::string("[E] ") + building.name + " 들어가기", gold, 18);
                break;
            }
        }
    }
    if (nearestNpcIndex >= 0)
        Text(550, 500, std::string("[E] ") + villagers[nearestNpcIndex].name, gold, 18);
    else if (!exploration && interior < 0 && Dist(player, shrine) < (upgrade == 1 ? 4.f : 2.1f))
        Text(544, 500, "[E] 해안 별빛 장치");
    if (messageTime > 0)
    {
        Box(250, 677, 1010, 56, {ink.r, ink.g, ink.b, .95f});
        Text(268, 711, message, teal, 18);
    }
    if (!exploration && interior < 0 && stage == 1 && Dist(player, boss) < 6.5f)
    {
        Box(415, 150, 450, 62, ink);
        Box(425, 161, 430 * bossHP / 10.f, 7, {.85f, .25f, .33f, 1});
        Text(430, 196,
             pulse > 2.1f ? "악몽 교감: 공격 준비! 붉은 범위 밖으로 피하세요!"
                          : "악몽 교감 · Q를 눌러 마법 공격",
             gold, 16);
    }
    if ((!exploration && (stage == 2 || stage == 5)) || paused)
    {
        Box(0, 0, 1280, 800, {.025f, .02f, .06f, .72f});
        Box(250, 240, 780, 302, ink);
        Box(250, 240, 780, 3, gold);
        if (paused)
        {
            Text(295, 303, "일시 정지", gold, 27);
            Text(295, 365, "ESC를 눌러 계속합니다. 시간도 잠시 쉬어 갑니다.");
        }
        else if (stage == 2)
        {
            Text(295, 295, "악몽 교감 처치 · 스킬 포인트 획득", gold, 26);
            Text(295, 340, "기본 악몽 봉인 해제를 배웠습니다.", teal, 20);
            Text(295, 390, "[1] 먼 손길: 별빛 장치 상호작용 거리를 4타일로 확장");
            Text(295, 435, "[2] 가벼운 발: 탐험 이동 속도 증가");
            Text(295, 494, "어느 쪽을 골라도 해안 별빛 장치를 열 수 있습니다.", teal, 17);
        }
        else
        {
            Text(295, 295, "튜토리얼 완료 · 별빛 조각 회수", gold, 25);
            Text(295, 345, "보스 처치 → 스킬 성장 → 탐험 → 보상", teal, 20);
            char result[180];
            sprintf_s(result, "완료 시간 %02d:%02d  ·  목표 5분 이내", int(finishTime) / 60,
                      int(finishTime) % 60);
            Text(295, 389, result);
            Text(295, 435, "모라: 빛도, 세금도 돌아왔군요. 고마워요!");
            Text(295, 490, "[N] 다시 시작    창을 닫으면 종료합니다.", gold, 17);
        }
    }
}

inline void Draw()
{
    auto syncStart = Performance::Clock::now();
    scene.BeginSync();
    DrawWorld();
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
    renderer->BeginWorld();
    scene.Render(Game::RenderLayer::Background, Game::RenderLayer::Overlay,
                 [](const Game::Transform& world)
                 {
                     return Visible(Project({world.x, world.y}, world.z), 240 * world.scale);
                 });
    renderer->EndWorld(clockTime);
    scene.Render(Game::RenderLayer::UserInterface, Game::RenderLayer::UserInterface);
    renderer->Flush();
}
} // namespace Tutorial
