#include "stdafx.h"
#include "ParkQuest.h"

#include "CharacterVisual.h"
#include "CombatVitals.h"
#include "CombatEffects.h"
#include "CombatKeys.h"
#include "CompanionFollow.h"
#include "GameInput.h"
#include "MonsterAnimation.h"
#include "PrimitiveModels.h"
#include "QuestCombat.h"
#include "SceneGraph.h"

namespace ParkQuest
{
namespace
{
using Game::Actor;
using Game::RenderLayer;
using Game::Transform;
using Hunting::ModelId;
using MainQuest::Distance;
using MainQuest::Point;

enum class Step
{
    MeetMinho,
    FindTracks,
    ClearShadows,
    FindDog,
    FightFlower,
    RescueDog,
    Return
};

struct Color
{
    float r, g, b, a = 1.f;
};

constexpr Point Exit{14, 26}, Minho{11, 23}, Tracks{15, 19}, Dog{21, 7}, Flower{15, 9};
constexpr float InteractRange = 2.f;
constexpr Color Gold{.96f, .85f, .52f};
constexpr Color Pink{.99f, .50f, .71f};
constexpr Color Ink{.08f, .07f, .14f, .94f};
constexpr std::array<Point, 14> Trees{{
    {3, 4}, {6, 7}, {3, 13}, {6, 16}, {3, 21}, {6, 25}, {10, 5},
    {20, 3}, {25, 4}, {25, 10}, {24, 14}, {25, 19}, {24, 24}, {21, 26}}};

Renderer* graphics = nullptr;
const Hunting::PrimitiveModels* models = nullptr;
Game::SceneGraph scene;
MainQuest::State encounter;
MainQuest::Combat combat;
Step step = Step::MeetMinho;
Point player = Exit, camera = Exit, facing{-.70710678f, -.70710678f};
std::array<bool, 256> keys{};
bool paused = false, exitRequested = false, summoned = false, moving = false;
float clockTime = 0, walkFrame = 0, messageTime = 0;
std::string message;
int screenWidth = 1280, screenHeight = 800;

bool Fighting()
{
    return step == Step::ClearShadows || step == Step::FightFlower;
}

void Say(const std::string& line)
{
    message = line;
    messageTime = 5.f;
}

float Scale()
{
    return (std::min)(screenWidth / 1280.f, screenHeight / 800.f);
}

Point Screen(float x, float y)
{
    return {(screenWidth - 1280 * Scale()) * .5f + x * Scale(),
            (screenHeight - 800 * Scale()) * .5f + y * Scale()};
}

Point Project(Transform p)
{
    const float x = p.x - camera.x, y = p.y - camera.y;
    return Screen(640 + (x - y) * 40, 430 + (x + y) * 20 - p.z);
}

void Triangle(Point a, Point b, Point c, Color color)
{
    graphics->Triangle(a.x, a.y, b.x, b.y, c.x, c.y, color.r, color.g, color.b, color.a);
}

void Quad(Point a, Point b, Point c, Point d, Color color)
{
    Triangle(a, b, c, color);
    Triangle(a, c, d, color);
}

void Panel(float x, float y, float w, float h, Color color)
{
    Quad(Screen(x, y), Screen(x + w, y), Screen(x + w, y + h), Screen(x, y + h), color);
}

void Text(float x, float y, const std::string& text, float size, Color color)
{
    const Point p = Screen(x, y);
    graphics->Text(p.x, p.y, text, size * Scale(), color.r, color.g, color.b, color.a);
}

void Ground(Transform p, float w, float h, Color color)
{
    Quad(Project(p), Project({p.x + w, p.y, p.z}), Project({p.x + w, p.y + h, p.z}),
         Project({p.x, p.y + h, p.z}), color);
}

void Disc(Transform p, float radius, Color color)
{
    const Point center = Project(p);
    for (int i = 0; i < 24; ++i)
    {
        const float a = i * .26179939f, b = (i + 1) * .26179939f;
        Triangle(center, Project({p.x + radius * std::cos(a), p.y + radius * std::sin(a), p.z}),
                 Project({p.x + radius * std::cos(b), p.y + radius * std::sin(b), p.z}), color);
    }
}

void Model(Transform p, ModelId id, Point look)
{
    const Point screen = Project(p);
    graphics->DrawModel(models->GetFacing(id, look.x, look.y), screen.x, screen.y,
                        p.scale * Scale(), p.scale * Scale());
}

void Label(const std::string& id, Transform p, const char* text, bool objective = false,
           Actor* parent = nullptr)
{
    scene.Place(id, p, objective ? RenderLayer::Objective : RenderLayer::Overlay,
                [text, objective](const Actor&, const Transform& world)
                {
                    Point s = Project(world);
                    if (objective)
                    {
                        const Point low = Screen(135, 170), high = Screen(1145, 670);
                        s.x = (std::max)(low.x, (std::min)(high.x, s.x));
                        s.y = (std::max)(low.y, (std::min)(high.y, s.y));
                    }
                    graphics->Nameplate(s.x, s.y, text, 16 * Scale(), 1.f, .95f, .86f, objective);
                },
                parent);
}

bool Walkable(Point p)
{
    if (p.x < 1.f || p.x > 27.f || p.y < 1.f || p.y > 27.f)
    {
        return false;
    }
    for (Point tree : Trees)
    {
        if (Distance(p, tree) < .65f)
        {
            return false;
        }
    }
    return true;
}

void BuildScene()
{
    Performance::Scope scope("cpu.park.static_scene_build_ms");
    scene.Clear();
    auto& root = scene.Place("park", {}, RenderLayer::Ground);
    scene.Place("park/lawn", {}, RenderLayer::Background,
                [](const Actor&, const Transform& p)
                {
                    Ground(p, 28.f, 28.f, {.26f, .39f, .31f});
                },
                &root);
    scene.Place("park/path", {12.5f, 4.f, 1.f}, RenderLayer::Background,
                [](const Actor&, const Transform& p)
                {
                    Ground(p, 3.f, 23.f, {.55f, .51f, .44f});
                    Ground({p.x - 4.f, p.y + 9.f, 1}, 14.f, 2.4f, {.55f, .51f, .44f});
                },
                &root);
    for (size_t i = 0; i < Trees.size(); ++i)
    {
        const Point p = Trees[i];
        const ModelId model = i % 3 == 0 ? ModelId::TreeB : ModelId::Tree;
        auto& tree = scene.Place("park/tree/" + std::to_string(i), {p.x, p.y}, RenderLayer::World,
                                 [model](const Actor&, const Transform& world)
                                 {
                                     Model(world, model, {1, 0});
                                 },
                                 &root);
        scene.Place("park/tree/shadow/" + std::to_string(i), {}, RenderLayer::Shadow,
                    [](const Actor&, const Transform& world)
                    {
                        Disc(world, .8f, {.06f, .12f, .1f, .27f});
                    },
                    &tree);
    }
    scene.RetainSubtree(root);
}

void StartFight(bool boss)
{
    encounter.active = true;
    encounter.step = boss ? MainQuest::Step::Boss : MainQuest::Step::Shadows;
    const float health = combat.health, mana = combat.mana;
    combat.Enter(encounter);
    combat.health = health;
    combat.mana = mana;
    if (boss)
    {
        auto& flower = combat.enemies.front();
        flower.position = Flower;
        flower.enhancedBossMaxHealth = 225.f + (encounter.progress.stats.level - 1) * 22;
        flower.health = flower.enhancedBossMaxHealth;
        flower.timer = .6f;
        summoned = false;
        step = Step::FightFlower;
    }
    else
    {
        for (size_t i = 0; i < combat.enemies.size(); ++i)
        {
            auto& enemy = combat.enemies[i];
            enemy.position = {11.f + i * 3.f, 14.f};
            enemy.kind = i == 1 ? 2 : 9;
            enemy.timer = .5f + i * .35f;
        }
        step = Step::ClearShadows;
    }
}

void RetryFight()
{
    encounter.progress.EnsureStorySupplies();
    combat.health = float(encounter.progress.stats.maxHealth);
    combat.mana = float(encounter.progress.stats.maxMana);
    const bool boss = step == Step::FightFlower;
    player = camera = boss ? Point{18, 10} : Point{14, 19};
    StartFight(boss);
    paused = false;
    keys.fill(false);
}

const char* Objective()
{
    switch (step)
    {
    case Step::MeetMinho:
        return "입구에 있는 민호에게 말을 걸어 보세요.";
    case Step::FindTracks:
        return "산책로에서 강아지 발자국을 조사하세요.";
    case Step::ClearShadows:
        return "산책로를 막고 있는 악몽을 처치하세요.";
    case Step::FindDog:
        return "공원 안쪽에서 강아지를 찾으세요.";
    case Step::FightFlower:
        return "검은 꽃을 쓰러뜨리세요. 붉은 예고와 독안개를 피하세요.";
    case Step::RescueDog:
        return "강아지에게 다가가 구해 주세요.";
    case Step::Return:
        return "입구를 통해 마을로 나가 미아에게 보고하세요.";
    }
    return "";
}

void Interact()
{
    if (combat.actions.height > 0)
    {
        return;
    }
    if (Distance(player, Exit) < InteractRange)
    {
        exitRequested = true;
        return;
    }
    if (step == Step::MeetMinho && Distance(player, Minho) < InteractRange)
    {
        step = Step::FindTracks;
        Say("민호: 산책하다가 우리 두부가 뛰어갔어요. 발자국을 따라가면 찾을 수 있을 거예요.");
    }
    else if (step == Step::FindTracks && Distance(player, Tracks) < InteractRange)
    {
        StartFight(false);
        Say("루미: 발자국 옆에 가시가 떨어져 있어. 조심해, 악몽들이 모여들고 있어!");
    }
    else if (step == Step::FindDog && Distance(player, Dog) < InteractRange)
    {
        StartFight(true);
        Say("루미: 저 꽃이 두부를 가시로 가뒀어! 바닥을 보고 피하면서 마법을 써!");
    }
    else if (step == Step::RescueDog && Distance(player, Dog) < InteractRange)
    {
        const auto previous = encounter.progress.stats;
        encounter.progress.Award(Rpg::ParkQuestExperience);
        combat.health += encounter.progress.stats.maxHealth - previous.maxHealth;
        combat.mana += encounter.progress.stats.maxMana - previous.maxMana;
        encounter.progress.coins = (std::min)(1000000, encounter.progress.coins + 180);
        step = Step::Return;
        Say("두부를 구하고 검은 조각을 얻었어요! 경험치 +" +
            std::to_string(Rpg::ParkQuestExperience) + " · 골드 +180");
    }
}

void DrawDog(Transform p)
{
    const Point s = Project(p);
    const float scale = Scale();
    const auto point = [&](float x, float y)
    {
        return Point{s.x + x * scale, s.y + y * scale};
    };
    const Color fur{.93f, .70f, .44f}, darkFur{.64f, .40f, .25f};
    const Color cream{1.f, .89f, .69f}, nose{.23f, .15f, .19f};
    const auto oval = [&](float x, float y, float w, float h, Color color)
    {
        for (int i = 0; i < 10; ++i)
        {
            const float a = i * .62831853f, b = (i + 1) * .62831853f;
            Triangle(point(x, y), point(x + w * std::cos(a), y + h * std::sin(a)),
                     point(x + w * std::cos(b), y + h * std::sin(b)), color);
        }
    };
    oval(-1, 0, 21, 5, {.10f, .13f, .12f, .36f});
    const float wag = 2.f * std::sin(clockTime * 11.f);
    oval(-19, -13 + wag, 7, 3, fur);
    oval(-11, -4, 3, 6, darkFur);
    oval(7, -4, 3, 6, darkFur);
    oval(-3, -10, 16, 8, darkFur);
    oval(-4, -12, 14, 7, fur);
    oval(-7, -2, 3, 5, fur);
    oval(9, -2, 3, 5, cream);
    oval(10, -13, 5, 6, {.86f, .31f, .42f});
    oval(13, -20, 9, 8, fur);
    oval(8, -20, 3, 6, darkFur);
    oval(20, -16, 6, 4, cream);
    oval(24, -17, 2.4f, 1.8f, nose);
    oval(16, -22, 1.5f, 1.7f, nose);
    oval(16.4f, -22.6f, .55f, .55f, cream);
}

void DrawTracks(Transform p)
{
    for (int i = 0; i < 5; ++i)
    {
        const float x = p.x + (i % 2 == 0 ? -.28f : .28f);
        const float y = p.y + 1.55f - i * .55f;
        const Color mud{.20f, .13f, .15f};
        Disc({x, y, p.z}, .14f, mud);
        for (int toe = -1; toe <= 1; ++toe)
        {
            Disc({x + toe * .12f, y - .19f + std::abs(toe) * .035f, p.z}, .058f, mud);
        }
    }
}

void DrawTarget(Point target, float height, const char* title, const char* prompt)
{
    const bool ready = Distance(player, target) < InteractRange && combat.actions.height <= 0;
    scene.Place("target/ground", {target.x, target.y, 2}, RenderLayer::Shadow,
                [ready](const Actor&, const Transform& p)
                {
                    const Color color = ready ? Gold : Color{.91f, .72f, .35f, .8f};
                    Quad(Project({p.x - .35f, p.y - .35f, p.z}),
                         Project({p.x + .35f, p.y - .35f, p.z}),
                         Project({p.x + .35f, p.y + .35f, p.z}),
                         Project({p.x - .35f, p.y + .35f, p.z}), color);
                });
    Label("target/name", {target.x, target.y, height}, ready ? prompt : title, true);
}

void DrawHud()
{
    Panel(22, 20, 800, 121, Ink);
    Text(44, 56, "메인 퀘스트 4 · 공원의 검은 흔적", 25, Pink);
    Text(600, 56, "레벨 " + std::to_string(encounter.progress.stats.level) + " · 골드 " +
                       std::to_string(encounter.progress.coins), 17, Gold);
    Text(44, 91, Objective(), 19, Gold);
    Text(44, 121, "E 상호작용 · J 마법 · K 별빛 폭발 (10) · L 별빛 낙하 (8) · F 루미", 16, Gold);
    CombatVitals::Draw(*graphics, screenWidth, screenHeight, combat.health, combat.mana,
                       encounter.progress);
    if (step == Step::FightFlower)
    {
        const auto& boss = combat.enemies.front();
        Panel(370, 151, 540, 40, Ink);
        Text(390, 175, boss.Enraged() ? "검은 꽃 · 격노" : "검은 꽃", 16, Pink);
        const float ratio = (std::max)(0.f, boss.health / boss.enhancedBossMaxHealth);
        Panel(535, 166, 350 * ratio, 9, Pink);
    }
    if (messageTime > 0)
    {
        Panel(155, 680, 970, 62, Ink);
        Text(180, 718, message, 19, Gold);
    }
    if (paused || combat.Dead())
    {
        Panel(355, 316, 570, 170, Ink);
        Text(385, 363, combat.Dead() ? "쓰러졌습니다" : "공원 · 일시정지", 26, Pink);
        Text(385, 411, combat.Dead() ? "[N] 다시 도전" : "[ESC] 계속", 18, Gold);
        if (combat.Dead())
        {
            Text(385, 449, "[M] 마을로 돌아가기 · 물약 정비", 18, Gold);
        }
    }
}
} // namespace

void Open(Renderer& renderer, const Hunting::PrimitiveModels& assets, const Rpg::Progress& progress)
{
    const auto previous = encounter.progress.stats;
    encounter.progress = progress;
    encounter.progress.Recalculate();
    if (graphics && !combat.Dead())
    {
        combat.health += encounter.progress.stats.maxHealth - previous.maxHealth;
        combat.mana += encounter.progress.stats.maxMana - previous.maxMana;
    }
    if (!graphics)
    {
        graphics = &renderer;
        models = &assets;
        encounter.active = true;
        combat.health = float(encounter.progress.stats.maxHealth);
        combat.mana = float(encounter.progress.stats.maxMana);
        combat.variedSkills = true;
        combat.advanceQuestOnDefeat = false;
        BuildScene();
        Say("루미: 검은 조각이 반응해. 입구에 있는 아이부터 만나 보자.");
    }
    paused = false;
    exitRequested = false;
    keys.fill(false);
}

void Resize(int width, int height)
{
    screenWidth = width;
    screenHeight = height;
}

void Pause()
{
    paused = true;
    keys.fill(false);
}

void Key(unsigned char key, bool down)
{
    const bool pressed = down && !keys[key];
    keys[key] = down;
    if (!pressed)
    {
        return;
    }
    if (key == 27)
    {
        paused = !paused;
        keys.fill(false);
        return;
    }
    if (combat.Dead())
    {
        if (key == CombatKeys::Retry && Fighting())
        {
            RetryFight();
        }
        else if (key == CombatKeys::ReturnToTown)
        {
            exitRequested = true;
            keys.fill(false);
        }
        return;
    }
    if (paused)
    {
        return;
    }
    if (key == 'e')
    {
        Interact();
    }
    else if (key == 'f')
    {
        Say(Fighting() ? "루미: 붉은 예고에서 벗어난 뒤에 반격해!"
                       : "루미: 노란 표식을 따라가 보자. 내가 옆에 있을게.");
    }
    else if (key == ' ')
    {
        combat.actions.Jump();
    }
    else if (key == CombatKeys::Burst && Fighting())
    {
        combat.Burst(player, encounter);
    }
    else if (key == CombatKeys::Starfall && Fighting())
    {
        combat.Starfall(player, facing, encounter, Walkable);
    }
    else if (key == CombatKeys::HealthPotion)
    {
        combat.UsePotion(encounter);
    }
    else if (key == CombatKeys::ManaPotion)
    {
        combat.actions.UseManaPotion(combat.mana, float(encounter.progress.stats.maxMana),
                                     encounter.progress.manaPotions);
    }
}

void Update(float dt)
{
    if (paused || combat.Dead())
    {
        return;
    }
    Performance::Scope scope("cpu.park.update_ms");
    clockTime += dt;
    messageTime -= dt;
    const float horizontal = float(keys['d']) - float(keys['a']);
    const float vertical = float(keys['s']) - float(keys['w']);
    const float length = std::hypot(horizontal, vertical);
    moving = false;
    if (length > 0)
    {
        const Point before = player;
        facing = {(horizontal + vertical) / (length * 1.41421356f),
                  (vertical - horizontal) / (length * 1.41421356f)};
        MainQuest::Combat::Move(player, facing, (GameInput::SprintHeld() ? 5.f : 3.f) * dt,
                                Walkable);
        moving = Distance(before, player) > .001f;
        walkFrame += dt * 9.f;
    }
    if (Fighting())
    {
        const int previousPhase = combat.enemies.front().phase;
        combat.Update(dt, player, encounter, Walkable);
        if (combat.Dead())
        {
            return;
        }
        if (step == Step::FightFlower)
        {
            const auto& boss = combat.enemies.front();
            if (boss.health > 0 && boss.pattern == 2 && previousPhase == 1 && boss.phase == 3)
            {
                combat.poisonZones.push_back({{boss.aim.x, boss.aim.y}, 13, combat.nextShot++});
            }
            if (boss.health > 0 && boss.Enraged() && !summoned)
            {
                summoned = true;
                for (Point p : {Point{11, 10}, Point{19, 10}})
                {
                    MainQuest::Enemy seed;
                    seed.position = p;
                    seed.kind = 9;
                    seed.timer = .7f;
                    combat.enemies.push_back(seed);
                }
                Say("루미: 검은 꽃이 가시 씨앗을 불러냈어! 독안개도 조심해.");
            }
        }
        const bool defeated = step == Step::FightFlower
                                  ? combat.enemies.front().health <= 0
                                  : std::none_of(combat.enemies.begin(), combat.enemies.end(),
                                                 [](const MainQuest::Enemy& enemy)
                                                 {
                                                     return enemy.health > 0;
                                                 });
        if (defeated)
        {
            const bool boss = step == Step::FightFlower;
            combat.enemies.clear();
            combat.shots.clear();
            combat.hostileShots.clear();
            combat.poisonZones.clear();
            combat.cast = 0;
            step = boss ? Step::RescueDog : Step::FindDog;
            Say(boss ? "루미: 가시가 풀렸어. 두부를 데리러 가자!"
                     : "루미: 산책로가 열렸어. 공원 안쪽에서 강아지 소리가 나.");
        }
        else if (keys[CombatKeys::Attack])
        {
            combat.Attack(player, facing, encounter, combat.actions.height, Walkable);
        }
    }
    else
    {
        combat.actions.Update(dt, combat.mana, float(encounter.progress.stats.maxMana));
        combat.burst -= dt;
        combat.starfall -= dt;
    }
    const float blend = 1.f - std::exp(-8.f * dt);
    camera.x += (player.x - camera.x) * blend;
    camera.y += (player.y - camera.y) * blend;
}

void Draw()
{
    Performance::Scope scope("cpu.park.draw_ms");
    scene.BeginSync();
    const Point look = combat.cast > 0 ? combat.castDirection : facing;
    const ModelId hero = static_cast<ModelId>(
        CharacterVisual::FrameIndex(combat.cast, moving, walkFrame));
    auto& heroActor = scene.Place("hero", {player.x, player.y, combat.actions.height}, RenderLayer::World,
                [hero, look](const Actor&, const Transform& p)
                {
                    Model(p, hero, look);
                });
    CombatEffects::Spells(scene, *graphics, combat, Scale(), Project);
    CombatEffects::Recovery(scene, *graphics, heroActor, combat.actions, 0.f, Scale(), Project);
    const Point lumi = CompanionFollow::RightOf(player, look);
    auto& companion = scene.Place("lumi", {lumi.x, lumi.y, 27 + 4 * std::sin(clockTime * 3)},
                                  RenderLayer::World,
                                  [look](const Actor&, const Transform& p)
                                  {
                                      Model(p, ModelId::Lumi, look);
                                  });
    Label("lumi/name", {0, 0, 57}, "루미 [F]", false, &companion);
    auto& child = scene.Place("minho", {Minho.x, Minho.y, 0, .85f}, RenderLayer::World,
                              [](const Actor&, const Transform& p)
                              {
                                  Model(p, ModelId::Villager_01View0, {player.x - p.x, player.y - p.y});
                              });
    Label("minho/name", {0, 0, 75}, "민호", false, &child);
    if (step >= Step::FindDog)
    {
        const Point dog = RescuedDog() ? Point{Minho.x + 1, Minho.y} : Dog;
        auto& pet = scene.Place("dog", {dog.x, dog.y}, RenderLayer::World,
                                [](const Actor&, const Transform& p)
                                {
                                    DrawDog(p);
                                });
        Label("dog/name", {0, 0, 44}, "두부", false, &pet);
    }
    if (step == Step::MeetMinho)
        DrawTarget(Minho, 105, "목표 · 민호", "민호에게 말 걸기 [E]");
    else if (step == Step::FindTracks)
    {
        scene.Place("park/pawprints", {Tracks.x, Tracks.y, 2}, RenderLayer::Shadow,
                    [](const Actor&, const Transform& p)
                    {
                        DrawTracks(p);
                    });
        DrawTarget(Tracks, 48, "목표 · 발자국", "발자국 조사 [E]");
    }
    else if (step == Step::FindDog || step == Step::RescueDog)
        DrawTarget(Dog, 74, "목표 · 두부", "두부에게 다가가기 [E]");
    else if (step == Step::Return || Distance(player, Exit) < InteractRange)
        DrawTarget(Exit, 48, "공원 출구", "마을로 돌아가기 [E]");

    for (size_t i = 0; i < combat.enemies.size(); ++i)
    {
        const auto enemy = combat.enemies[i];
        if (enemy.health <= 0)
        {
            continue;
        }
        const std::string id = "enemy/" + std::to_string(i);
        scene.Place(id, {enemy.position.x, enemy.position.y}, RenderLayer::World,
                    [enemy](const Actor&, const Transform& p)
                    {
                        if (enemy.boss)
                        {
                            const Point s = Project(p);
                            graphics->DrawModel(models->GetParkBoss(enemy.facing.x, enemy.facing.y,
                                                                    enemy.phase == 1 || enemy.phase == 2),
                                                s.x, s.y, Scale() * 1.35f, Scale() * 1.35f);
                        }
                        else
                        {
                            const auto model = MonsterAnimation::FacingModel(
                                enemy.kind, clockTime, enemy.attackMotion,
                                enemy.facing.x, enemy.facing.y);
                            Model(p, model, enemy.facing);
                        }
                    });
        if (enemy.boss && enemy.phase == 1)
        {
            const Point center = enemy.pattern == 0 ? enemy.position : enemy.aim;
            scene.Place(id + "/warning", {center.x, center.y, 2}, RenderLayer::Shadow,
                        [enemy](const Actor&, const Transform& p)
                        {
                            const Color red{.98f, .20f, .30f, .54f};
                            if (enemy.pattern == 1 || enemy.pattern == 3)
                            {
                                const int spread = enemy.pattern == 1 ? 0 :
                                                   enemy.bossAttackEnraged ? 3 : 2;
                                const float length = enemy.pattern == 1 ? enemy.ChargeSpeed() * .5f : 9.f;
                                const float width = enemy.pattern == 1 ? 1.2f : .4f;
                                for (int ray = -spread; ray <= spread; ++ray)
                                {
                                    const Point d = MainQuest::BossFanDirection(enemy.facing, ray);
                                    const Point side{-d.y * width, d.x * width};
                                    const Point a = enemy.position;
                                    const Point b{a.x + d.x * length, a.y + d.y * length};
                                    Quad(Project({a.x + side.x, a.y + side.y, p.z}),
                                         Project({b.x + side.x, b.y + side.y, p.z}),
                                         Project({b.x - side.x, b.y - side.y, p.z}),
                                         Project({a.x - side.x, a.y - side.y, p.z}), red);
                                }
                            }
                            else
                            {
                                Disc(p, enemy.BossRadius(), red);
                            }
                        });
        }
    }
    for (const auto& poison : combat.poisonZones)
    {
        scene.Place("poison/" + std::to_string(poison.id), {poison.position.x, poison.position.y, 2},
                    RenderLayer::Shadow,
                    [poison](const Actor&, const Transform& p)
                    {
                        Disc(p, MonsterSkills::Poison::Radius,
                             poison.age < MonsterSkills::Poison::Warning
                                 ? Color{.94f, .35f, .23f, .55f}
                                 : Color{.52f, .83f, .29f, .66f});
                    });
    }
    for (const auto& shot : combat.shots)
    {
        auto p = CharacterVisual::FacingSpellPose(shot.position.x, shot.position.y, shot.age,
                                                  shot.wandFacing.x, shot.wandFacing.y, 40);
        p.z += shot.launchHeight * (1 - (std::min)(1.f, shot.age / .18f));
        scene.Place("spell/" + std::to_string(shot.id), p, RenderLayer::World,
                    [](const Actor&, const Transform& world)
                    {
                        const Point s = Project(world);
                        const float size = CharacterVisual::SpellSize * Scale();
                        graphics->Effect(s.x - size / 2, s.y - size / 2, size, size, clockTime, 2);
                    });
    }
    for (const auto& shot : combat.hostileShots)
    {
        scene.Place("thorn/" + std::to_string(shot.id), {shot.position.x, shot.position.y, 24},
                    RenderLayer::World,
                    [](const Actor&, const Transform& p)
                    {
                        const Point s = Project(p);
                        const float size = 9.f * Scale();
                        Triangle({s.x - size, s.y + size}, {s.x + size, s.y + size},
                                 {s.x, s.y - size}, Pink);
                    });
    }
    scene.Place("hud", {}, RenderLayer::UserInterface, [](const Actor&, const Transform&)
                {
                    DrawHud();
                });
    scene.EndSync();
    graphics->BeginWorld();
    scene.Render(RenderLayer::Background, RenderLayer::Overlay,
                 [](const Transform& p)
                 {
                     const Point s = Project(p);
                     const float margin = 260 * Scale();
                     return s.x >= -margin && s.x <= screenWidth + margin && s.y >= -margin &&
                            s.y <= screenHeight + margin;
                 });
    graphics->EndWorld(clockTime);
    scene.Render(RenderLayer::Objective, RenderLayer::UserInterface);
    graphics->Flush();
}

bool ConsumeExit()
{
    const bool requested = exitRequested;
    exitRequested = false;
    return requested;
}

bool Defeated()
{
    return combat.Dead();
}

void Restock(int healthPotions, int manaPotions)
{
    encounter.progress.potions = healthPotions;
    encounter.progress.manaPotions = manaPotions;
    RetryFight();
}

bool RescuedDog()
{
    return step == Step::Return;
}

const Rpg::Progress& Progress()
{
    return encounter.progress;
}

WorldMonitor::Scene MonitorSnapshot()
{
    WorldMonitor::Scene snapshot;
    snapshot.id = "park";
    snapshot.name = "별빛 중앙공원";
    snapshot.objective = Objective();
    snapshot.paused = paused || combat.Dead();
    for (const auto& enemy : combat.enemies)
    {
        enemy.health > 0 ? ++snapshot.mobsAlive : ++snapshot.mobsDead;
    }
    return snapshot;
}
} // namespace ParkQuest
