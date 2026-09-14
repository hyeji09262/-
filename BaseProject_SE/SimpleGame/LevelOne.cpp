#include "stdafx.h"
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
};

struct Drop
{
    Position position;
    int kind;
    int quantity;
};

struct Projectile
{
    Position position;
    Position direction;
    int damage;
    float lifetime;
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
Rpg::Progress progress;
std::vector<Enemy> enemies;
std::vector<Drop> drops;
std::vector<Projectile> projectiles;
std::vector<FloatingText> floating;
std::array<int, Hunting::Map::Count> navigation;
std::mt19937 random;
Position player;
Position facing = {0.707f, -0.707f};
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
float noticeTimer = 0;
bool paused = false;
bool moving = false;
bool initialized = false;
bool saveSucceeded = true;
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
    if (Visible(screen))
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

void NewMap()
{
    map.Generate(random());
    player = map.camp;
    enemies.clear();
    drops.clear();
    projectiles.clear();
    floating.clear();
    health = static_cast<float>(progress.stats.maxHealth);
    mana = static_cast<float>(progress.stats.maxMana);
    attackTimer = damageTimer = castTimer = navigationTimer = 0;
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
        enemy.kind = static_cast<int>(random() % 2);
        enemy.level = (std::min)(3, 1 + static_cast<int>(Distance(p, map.camp) / 12));
        enemy.health = enemy.maxHealth = 18 + enemy.level * 6;
        enemies.push_back(enemy);
        if (enemies.size() == 24)
        {
            break;
        }
    }
    navigation = map.Distances(player);
    Notify("첫 사냥터입니다. 스페이스로 공격하고 E로 전리품을 획득하세요.");
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
    if (attackTimer > 0 || mana < 4)
    {
        return;
    }

    Position direction = facing;
    float closest = 7;
    for (const auto& enemy : enemies)
    {
        float distance = Distance(player, enemy.position);
        if (enemy.health > 0 && distance < closest && distance > .01f &&
            LineOfSight(player, enemy.position))
        {
            closest = distance;
            direction = {(enemy.position.x - player.x) / distance,
                         (enemy.position.y - player.y) / distance};
        }
    }

    mana -= 4;
    attackTimer = .5f;
    castTimer = .25f;
    projectiles.push_back({player, direction, progress.stats.attack, .85f});
}

void Kill(Enemy& enemy)
{
    enemy.health = 0;
    enemy.respawnTimer = 10;
    int reward = 12 + enemy.level * 6;
    bool leveled = progress.Award(reward);
    FloatText(enemy.position, "+" + std::to_string(reward) + " 경험치", teal);

    // Guaranteed coins, plus a potion or a mana crystal. Position is already walkable.
    if (drops.size() < 300)
    {
        drops.push_back({enemy.position, 0, 2 + enemy.level});
        drops.push_back({enemy.position, random() % 3 == 0 ? 1 : 2, 1});
    }

    if (leveled)
    {
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
    for (auto& shot : projectiles)
    {
        shot.lifetime -= dt;
        int steps = (std::max)(1, static_cast<int>(std::ceil(9 * dt / .10f)));
        for (int step = 0; step < steps && shot.lifetime > 0; ++step)
        {
            Position next = {shot.position.x + shot.direction.x * 9 * dt / steps,
                             shot.position.y + shot.direction.y * 9 * dt / steps};
            if (!map.Walkable(next, .06f))
            {
                shot.lifetime = 0;
                break;
            }
            shot.position = next;
            for (auto& enemy : enemies)
            {
                if (enemy.health > 0 && Distance(shot.position, enemy.position) < .5f)
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
        Notify("캠프에서 회복했습니다. 경험치와 소지품은 잃지 않습니다.");
    }
}

void UpdateEnemies(float dt)
{
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
                enemy.health = enemy.maxHealth;
                enemy.attackTimer = 1;
                enemy.wanderTimer = 0;
            }
            continue;
        }

        enemy.attackTimer -= dt;
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
        if (distance > .08f && (!chase || Distance(enemy.position, player) > .65f))
        {
            float speed = chase ? 1.6f + enemy.kind * .35f : .65f;
            Position before = enemy.position;
            map.Move(enemy.position, {(target.x - enemy.position.x) / distance * speed * dt,
                                      (target.y - enemy.position.y) / distance * speed * dt});
            if (map.Safe(enemy.position))
            {
                enemy.position = before;
            }
            enemy.animation += Distance(enemy.position, before) * 7;
        }
        if (Distance(enemy.position, player) < .9f && enemy.attackTimer <= 0 &&
            LineOfSight(enemy.position, player))
        {
            enemy.attackTimer = 1.4f;
            ReceiveDamage(4 + enemy.level * 2);
        }
    }
}

void PickUp()
{
    int collected = 0;
    for (auto it = drops.begin(); it != drops.end();)
    {
        if (Distance(player, it->position) > 1.5f || !LineOfSight(player, it->position))
        {
            ++it;
            continue;
        }
        if (it->kind == 1 && progress.potions >= 99)
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
        else
        {
            progress.crystals = (std::min)(1000000, progress.crystals + it->quantity);
            mana = (std::min)(float(progress.stats.maxMana), mana + 8.f);
        }
        ++collected;
        it = drops.erase(it);
    }
    if (collected > 0)
    {
        Notify("전리품 획득! 골드·물약·마력석은 소지품에 저장됩니다.");
        Save();
    }
    else
    {
        Notify("가까운 전리품이 없습니다. 물약은 최대 99개까지 소지할 수 있습니다.");
    }
}

void DrawMap()
{
    // Ground is dynamic tile placement. Reusable scenery / actor meshes come from disk cache.
    for (int sum = 0; sum < Hunting::Map::Width + Hunting::Map::Height; ++sum)
    {
        for (int x = 0; x < Hunting::Map::Width; ++x)
        {
            int y = sum - x;
            if (y < 0 || y >= Hunting::Map::Height)
            {
                continue;
            }
            Position p = {x + .5f, y + .5f};
            Position s = Project(p);
            if (!Visible(s, 36))
            {
                continue;
            }
            auto tile = map.tiles[Hunting::Map::Index(x, y)];
            if (tile == Hunting::Tile::Water)
            {
                // UV movement / ripples are entirely in Effect.fs.
                Effect(p, 65, 33, 0, -16.5f, x * .63f + y * .91f);
            }
            else
            {
                float noise = ((x * 7 + y * 13) % 5) * .007f;
                Color ground = map.Safe(p) ? Color{.38f + noise, .33f + noise, .27f + noise, 1}
                                           : Color{.18f + noise, .28f + noise, .24f + noise, 1};
                Diamond(s, 32.5f, 16.5f, ground);
            }
        }
    }

    for (int y = 1; y < Hunting::Map::Height - 1; ++y)
    {
        for (int x = 1; x < Hunting::Map::Width - 1; ++x)
        {
            auto tile = map.tiles[Hunting::Map::Index(x, y)];
            if (tile == Hunting::Tile::Tree || tile == Hunting::Tile::Rock)
            {
                Model(ModelId::Shadow, {x + .5f, y + .5f}, tile == Hunting::Tile::Tree ? 1.4f : 1);
            }
        }
    }
    Model(ModelId::Shadow, player);
    for (const auto& enemy : enemies)
    {
        if (enemy.health > 0)
        {
            Model(ModelId::Shadow, enemy.position);
        }
    }
}

void DrawActors()
{
    struct Item
    {
        Position position;
        ModelId model;
        float scale;
        bool mirror;
    };

    std::vector<Item> items;
    for (int y = 1; y < Hunting::Map::Height - 1; ++y)
    {
        for (int x = 1; x < Hunting::Map::Width - 1; ++x)
        {
            auto tile = map.tiles[Hunting::Map::Index(x, y)];
            Position p = {x + .5f, y + .5f};
            if (!Visible(Project(p)))
            {
                continue;
            }
            if (tile == Hunting::Tile::Tree || tile == Hunting::Tile::Rock)
            {
                items.push_back(
                    {p, tile == Hunting::Tile::Tree ? ModelId::Tree : ModelId::Rock, 1, false});
            }
        }
    }

    // These camp props are decorative; no hidden collision is added over the connected floor grid.
    items.push_back({{map.camp.x - 2, map.camp.y - 2}, ModelId::Shelter, 1, false});
    items.push_back({{map.camp.x + 1, map.camp.y - 1}, ModelId::Campfire, 1, false});
    ModelId playerModel = castTimer > 0 ? ModelId::PlayerCast : ModelId::PlayerIdle;
    if (castTimer <= 0 && moving)
    {
        playerModel = static_cast<ModelId>(static_cast<int>(ModelId::PlayerWalk0) +
                                           static_cast<int>(walkFrame) % 4);
    }
    if (damageTimer <= 0 || static_cast<int>(time * 14) % 2 == 0)
    {
        items.push_back({player, playerModel, 1, facing.x - facing.y < 0});
    }
    for (const auto& enemy : enemies)
    {
        if (enemy.health > 0)
        {
            int frame = static_cast<int>(enemy.animation) % 4;
            int base = static_cast<int>(enemy.kind == 0 ? ModelId::Slime0 : ModelId::Shade0);
            items.push_back({enemy.position, static_cast<ModelId>(base + frame), 1, false});
        }
    }
    for (const auto& drop : drops)
    {
        ModelId model = drop.kind == 0   ? ModelId::Coin
                        : drop.kind == 1 ? ModelId::Potion
                                         : ModelId::Crystal;
        items.push_back({drop.position, model, 1, false});
    }
    std::stable_sort(items.begin(), items.end(),
                     [](const Item& a, const Item& b)
                     {
                         return a.position.x + a.position.y < b.position.x + b.position.y;
                     });
    for (const auto& item : items)
    {
        Model(item.model, item.position, item.scale, 0, item.mirror);
    }

    Effect({map.camp.x + 1, map.camp.y - 1}, 42, 63, 1);
    for (const auto& shot : projectiles)
    {
        Effect(shot.position, 30, 30, 2, 12);
    }
    if (castTimer > 0)
    {
        Effect(player, 38, 38, 2, 24);
    }
}

void DrawHud()
{
    Box(18, 18, 820, 136, panel);
    Text(35, 48, "첫 번째 레벨 · 황혼의 사냥터", gold, 25);
    char line[320];
    sprintf_s(line, "레벨 %d   체력 %.0f / %d   마나 %.0f / %d   공격력 %d   방어력 %d",
              progress.stats.level, health, progress.stats.maxHealth, mana, progress.stats.maxMana,
              progress.stats.attack, progress.stats.defense);
    Text(35, 80, line);

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
    sprintf_s(line, "골드 %d    회복 물약 %d [Q]    마력석 %d    %s", progress.coins,
              progress.potions, progress.crystals,
              map.Safe(player) ? "캠프: 회복 중" : "야생 사냥 지역");
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
            sprintf_s(line, "레벨 %d %s", enemy.level,
                      enemy.kind == 0 ? "이끼 괴물" : "그림자 짐승");
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
        if (Distance(player, drop.position) < 1.5f)
        {
            Text(548, 492, "[E] 주변 전리품 획득", gold);
            break;
        }
    }
    Box(18, 710, 1244, 74, panel);
    Text(35, 738,
         "WASD 이동   스페이스 공격   E 획득   Q 물약   ESC 메뉴   F1 튜토리얼 / F2 사냥터", gold,
         17);
    std::string cache = models.loadedFromFile ? "모델: 파일 캐시 로딩"
                        : models.storedToFile ? "모델: 최초 생성·캐시 저장 완료"
                                              : "모델: 캐시 쓰기 실패";
    Text(35, 769, cache + (saveSucceeded ? "   |   성장 자동 저장" : "   |   성장 저장 실패"),
         white, 15);
    if (noticeTimer > 0)
    {
        Box(18, 655, 1244, 45, panel);
        Text(35, 685, notice, teal, 18);
    }
    if (paused)
    {
        Box(0, 0, 1280, 800, {.02f, .025f, .04f, .78f});
        Box(270, 250, 740, 275, panel);
        Text(307, 298, "사냥터 메뉴", gold, 26);
        Text(307, 347, "ESC: 계속하기   |   R: 새로운 랜덤 맵");
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

void Resize(int width, int height)
{
    screenWidth = (std::max)(1, width);
    screenHeight = (std::max)(1, height);
}

void Save()
{
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
        if (key == 'r')
        {
            NewMap();
            paused = false;
        }
        return;
    }
    if (key == 'e')
    {
        PickUp();
    }
    if (key == 'q' && progress.potions > 0 && health < progress.stats.maxHealth)
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
    if (!initialized || paused)
    {
        return;
    }
    time += dt;
    attackTimer -= dt;
    damageTimer -= dt;
    castTimer -= dt;
    noticeTimer -= dt;
    mana = (std::min)(float(progress.stats.maxMana), mana + dt * 6);
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
        map.Move(player, {facing.x * 3.5f * dt, facing.y * 3.5f * dt});
    }
    moving = Distance(before, player) > .001f;
    if (moving)
    {
        walkFrame += dt * 9;
    }
    if (keys[' '])
    {
        Attack();
    }

    UpdateProjectiles(dt);
    UpdateEnemies(dt);
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
    graphics->BeginWorld();
    DrawMap();
    DrawActors();
    graphics->EndWorld(time);
    DrawHud();
    graphics->Flush();
}
} // namespace LevelOne
