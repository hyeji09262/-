#include "stdafx.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "GameInput.h"
#include "CompanionFollow.h"
#include <algorithm>
#include <array>
#include <cmath>
#include "QuestTwo.h"
#include "QuestTwoStory.h"
#include "QuestCombat.h"
#include "CombatVitals.h"
#include "CombatEffects.h"
#include "CombatKeys.h"
#include "PrimitiveModels.h"
#include "SceneGraph.h"
#include "SchoolScene.h"
#include "TownPopulation.h"
#include "Transformation.h"

namespace QuestTwo
{
namespace
{
using Game::Actor;
using Game::RenderLayer;
using Game::Transform;
using Hunting::ModelId;
using MainQuest::Distance;
using MainQuest::Point;

struct Color
{
    float r, g, b, a = 1;
};

const Color pink{.95f, .40f, .63f}, blue{.52f, .70f, 1.f}, gold{1, .83f, .34f};
const Color paper{.95f, .95f, 1}, panel{.06f, .08f, .15f, .96f};
Renderer* graphics = nullptr;
Hunting::PrimitiveModels models;
Game::SceneGraph graph;
StoryState story;
// Adapter for the existing combat system. It never loads or saves MQ001 data.
MainQuest::State encounter;
MainQuest::Combat combat;
Point player{3, 9}, camera{3, 9};
std::array<bool, 256> keys{};
int width = 1280, height = 800;
bool available = false, loaded = false, paused = true;
bool moving = false, mirror = false;
Point facingDirection{.70710678f, -.70710678f};
float clockTime = 0, walkFrame = 0;
std::string speaker, dialogue;
int pendingClue = -1;
bool pendingAdvance = false;
bool builtGym = false;
int schoolRoom = 0;
bool worldRequested = false;
bool awaitingHall = false, magicalForm = false;
Transformation::Sequence transformation;
float introTime = 0;

bool BattleReady()
{
    return !awaitingHall &&
           story.ReadyToFight(story.Gym() ? 2 : schoolRoom, transformation.Active());
}

Point LumiPosition()
{
    if (story.step == Step::Reaction)
    {
        return {SchoolLayout::IntroLumiX, SchoolLayout::IntroLumiY};
    }
    const Point look = combat.cast > 0 ? combat.castDirection : facingDirection;
    return CompanionFollow::RightOf(player, look);
}

int SceneRoom()
{
    return story.Gym() ? 2 : schoolRoom;
}

Point RoomExit()
{
    const auto& room = SchoolLayout::Get(SceneRoom());
    return {room.width * .5f, room.height - 1.5f};
}

Point CluePosition(unsigned index)
{
    if (schoolRoom == 0)
    {
        return index == 0 ? Point{6, 4} : index == 1 ? Point{24, 3} : Point{29, 3};
    }
    const auto& site = Investigations()[index];
    if (schoolRoom == site.room)
    {
        return {site.x, site.y};
    }
    return RoomExit();
}

float Scale()
{
    return (std::min)(width / 1280.f, height / 800.f);
}

Point Screen(float x, float y)
{
    const float scale = Scale();
    return {(width - 1280 * scale) * .5f + x * scale, (height - 800 * scale) * .5f + y * scale};
}

Point Project(Transform p)
{
    return Screen(640 + (p.x - camera.x - p.y + camera.y) * 40,
                  440 + (p.x - camera.x + p.y - camera.y) * 20 - p.z);
}

void Triangle(Point a, Point b, Point c, Color color)
{
    graphics->Triangle(a.x, a.y, b.x, b.y, c.x, c.y, color.r, color.g, color.b, color.a);
}

void Panel(float x, float y, float w, float h, Color color = panel)
{
    Point a = Screen(x, y), b = Screen(x + w, y), c = Screen(x + w, y + h), d = Screen(x, y + h);
    Triangle(a, b, c, color);
    Triangle(a, c, d, color);
}

void Text(float x, float y, const std::string& text, float size = 18, Color color = paper)
{
    Point p = Screen(x, y);
    graphics->Text(p.x, p.y, text, size * Scale(), color.r, color.g, color.b);
}

void Floor(Transform p, float w, float h, Color color)
{
    Point a = Project(p), b = Project({p.x + w, p.y, p.z});
    Point c = Project({p.x + w, p.y + h, p.z}), d = Project({p.x, p.y + h, p.z});
    Triangle(a, b, c, color);
    Triangle(a, c, d, color);
}

void Box(Transform p, float w, float h, float tall, Color color)
{
    Point a = Project(p), b = Project({p.x + w, p.y, p.z});
    Point c = Project({p.x + w, p.y + h, p.z}), d = Project({p.x, p.y + h, p.z});
    Point at = Project({p.x, p.y, p.z + tall}), bt = Project({p.x + w, p.y, p.z + tall});
    Point ct = Project({p.x + w, p.y + h, p.z + tall}), dt = Project({p.x, p.y + h, p.z + tall});
    Color shade{color.r * .7f, color.g * .7f, color.b * .7f, color.a};
    Triangle(b, c, ct, shade);
    Triangle(b, ct, bt, shade);
    Triangle(c, d, dt, color);
    Triangle(c, dt, ct, color);
    Triangle(at, bt, ct, color);
    Triangle(at, ct, dt, color);
}

Actor& Model(const std::string& id, Transform pose, ModelId model, Actor* parent = nullptr,
             bool mirrored = false, RenderLayer layer = RenderLayer::World)
{
    return graph.Place(
        id, pose, layer,
        [model, mirrored](const Actor&, const Transform& world)
        {
            Point p = Project(world);
            const Point look = combat.cast > 0 ? combat.castDirection : facingDirection;
            const auto& mesh = model == ModelId::Lumi && story.step != Step::Reaction
                                   ? models.GetFacing(model, look.x, look.y)
                                   : models.Get(model);
            graphics->DrawModel(mesh, p.x, p.y,
                                Scale() * world.scale * (mirrored ? -1 : 1), Scale() * world.scale);
        },
        parent);
}

void Label(const std::string& id, Transform pose, const std::string& text, Color color = paper,
           Actor* parent = nullptr)
{
    graph.Place(
        id, pose, RenderLayer::World,
        [text, color](const Actor&, const Transform& world)
        {
            Point p = Project(world);
            graphics->Nameplate(p.x, p.y, text, 16 * Scale(), color.r, color.g, color.b);
        },
        parent);
}

void Glow(const std::string& id, Transform pose, float size, Actor* parent = nullptr,
          RenderLayer layer = RenderLayer::World)
{
    graph.Place(
        id, pose, layer,
        [size](const Actor&, const Transform& world)
        {
            Point p = Project(world);
            float extent = size * Scale();
            graphics->Effect(p.x - extent / 2, p.y - extent / 2, extent, extent, clockTime, 2);
        },
        parent);
}

bool Walkable(Point p)
{
    if (!SchoolLayout::Walkable(SceneRoom(), p.x, p.y))
    {
        return false;
    }
    return true;
}

void BuildScene()
{
    Performance::Scope scope("cpu.quest_two.static_scene_build_ms");
    graph.Clear();
    builtGym = story.Gym();
    auto& root = graph.Place("school", {}, RenderLayer::Ground);
    SchoolScene::Build(graph, SceneRoom(),
                       [](ModelId model, const Transform& world)
                       {
                           const Point p = Project(world);
                           graphics->DrawModel(models.Get(model), p.x, p.y,
                                               Scale() * world.scale * 1.25f,
                                               Scale() * world.scale * 1.25f);
                       },
                       *graphics, Project);
    Label("school/name", {12, 1, 120}, SchoolLayout::Get(SceneRoom()).name, gold, &root);
    if (!builtGym && schoolRoom == 0)
    {
        auto& yuna = Model("yuna", {3, 11}, ModelId::Yuna, &root);
        Label("yuna/name", {0, 0, 80}, "유나 · 수업 대기", paper, &yuna);
        auto& jisoo = Model("jisoo", {7, 11}, ModelId::Jisoo, &root);
        Label("jisoo/name", {0, 0, 80}, "지수 · 수업 대기", paper, &jisoo);
    }
    else if (SceneRoom() == 2)
    {
        graph.Place(
            "court", {7, 6, 1}, RenderLayer::Ground,
            [](const Actor&, const Transform& p)
            {
                Floor(p, 18, .06f, paper);
                Floor({p.x, p.y + 15, 1}, 18, .06f, paper);
                Floor(p, .06f, 15, paper);
                Floor({p.x + 18, p.y, 1}, .06f, 15, paper);
                Floor({p.x, p.y + 7.5f, 1}, 18, .06f, paper);
            },
            &root);
    }
    for (size_t i = 0; i < TownPopulation::StaffMembers().size(); ++i)
    {
        const auto& staff = TownPopulation::StaffMembers()[i];
        if (staff.kind == 1 && staff.room == SceneRoom())
        {
            auto& actor =
                Model("school/staff/" + std::to_string(i), {staff.x, staff.y}, staff.model, &root);
            Label("school/staff/name/" + std::to_string(i), {0, 0, 112}, staff.name, paper, &actor);
        }
    }
    graph.RetainSubtree(root);
}

void EnterStep(bool retry = false)
{
    Performance::Scope scope("cpu.quest_two.step_enter_ms");
    if (retry && story.step == Step::Elite)
    {
        story.progress.potions = (std::max)(3, story.progress.potions);
        story.progress.manaPotions = (std::max)(3, story.progress.manaPotions);
    }
    encounter = MainQuest::State{};
    encounter.active = true;
    encounter.progress = story.progress;
    encounter.step = story.step == Step::Shadows ? MainQuest::Step::Shadows
                     : story.step == Step::Elite ? MainQuest::Step::Boss
                                                 : MainQuest::Step::Complete;
    combat.Enter(encounter);
    combat.variedSkills = true;
    if (story.step == Step::Shadows)
    {
        const int kinds[] = {1, 4, 9};
        for (size_t i = 0; i < combat.enemies.size(); ++i)
        {
            auto& enemy = combat.enemies[i];
            enemy.kind = kinds[i % 3];
            enemy.position = {12.f + float(i) * 7, 7.f + float(i % 2)};
            enemy.health = 24.f + story.progress.stats.level * 6;
            enemy.timer = 1.f + float(i) * .4f;
        }
    }
    if (story.step == Step::Elite && !combat.enemies.empty())
    {
        auto& boss = combat.enemies.front();
        boss.enhancedBossMaxHealth = 250.f + (story.progress.stats.level - 1) * 24;
        boss.health = boss.enhancedBossMaxHealth;
    }
    if (retry || builtGym != story.Gym())
    {
        schoolRoom = story.step == Step::Reaction ? 4 : story.step == Step::Gym ? 2 : 0;
        player = story.step == Step::Reaction
                     ? Point{SchoolLayout::IntroPlayerX, SchoolLayout::IntroPlayerY}
                 : story.Gym() || story.step == Step::Gym ? Places::GymEntry
                                                          : Point{3, 9};
        camera = player;
        if (story.step == Step::Reaction)
        {
            camera.y = 7.f;
            introTime = 0;
        }
    }
    if (retry)
    {
        magicalForm = false;
    }
    awaitingHall = story.step == Step::Shadows && schoolRoom != 0;
    transformation.remaining = 0;
    if (story.step == Step::Elite && !magicalForm)
    {
        transformation.Start();
    }
    BuildScene();
    keys.fill(false);
    moving = false;
    Performance::Profiler::Get().Count("quest_two.step_entries");
}

void Talk(const std::string& name, const std::string& text)
{
    speaker = name;
    dialogue = text;
    keys.fill(false);
    moving = false;
}

void Interact()
{
    if (!dialogue.empty())
    {
        dialogue.clear();
        const Step previous = story.step;
        if (pendingClue >= 0)
        {
            story.Inspect(unsigned(pendingClue));
        }
        else if (pendingAdvance)
        {
            story.Advance(MainQuest::Event::Interact);
        }
        pendingClue = -1;
        pendingAdvance = false;
        if (previous != story.step)
        {
            EnterStep();
        }
        else
        {
        }
        if (story.step == Step::Complete && story.treasureTaken)
        {
            worldRequested = true;
            keys.fill(false);
        }
        return;
    }
    if (combat.actions.height > 0)
    {
        return;
    }
    if (transformation.Active())
    {
        return;
    }
    if (story.step == Step::Reaction)
    {
        if (introTime >= 4.f)
        {
            story.Advance(MainQuest::Event::Interact);
            EnterStep();
        }
        return;
    }
    if (story.step == Step::Complete && story.treasureTaken)
    {
        worldRequested = true;
        keys.fill(false);
        return;
    }
    if (!story.Fighting() && Distance(player, RoomExit()) < 2.1f &&
        (story.Gym() || schoolRoom == 0))
    {
        worldRequested = true;
        keys.fill(false);
        return;
    }
    if (!story.Gym() && (!story.Fighting() || awaitingHall))
    {
        if (schoolRoom != 0 && Distance(player, RoomExit()) < 2.1f)
        {
            if (story.step == Step::Reaction)
            {
                Talk("루미", "먼저 창가 쪽으로 와서 검은 조각의 반응을 확인하자.");
                return;
            }
            for (const auto& door : SchoolLayout::Doors())
            {
                if (door.room == schoolRoom)
                {
                    player = {door.x, door.y + 2};
                    break;
                }
            }
            schoolRoom = 0;
            if (awaitingHall)
            {
                awaitingHall = false;
                transformation.remaining = 0;
            }
            camera = player;
            keys.fill(false);
            BuildScene();
            return;
        }
        if (schoolRoom == 0)
        {
            for (const auto& door : SchoolLayout::Doors())
            {
                if (Distance(player, {door.x, door.y}) >= 2.f)
                {
                    continue;
                }
                if (door.room == 2)
                {
                    if (story.step == Step::Gym)
                    {
                        schoolRoom = 2;
                        player = Places::GymEntry;
                        camera = player;
                        keys.fill(false);
                        BuildScene();
                    }
                    else
                    {
                        Talk("체육관",
                             "먼저 복도, 도서관, 방송실의 흔적을 확인하고 그림자를 물리치자.");
                    }
                    return;
                }
                schoolRoom = door.room;
                const auto& room = SchoolLayout::Get(schoolRoom);
                player = {room.width * .5f, room.height - 3.f};
                camera = player;
                keys.fill(false);
                BuildScene();
                return;
            }
        }
    }
    if (story.step == Step::Investigate)
    {
        for (unsigned i = 0; i < 3; ++i)
        {
            const auto& site = Investigations()[i];
            const bool inRoom = schoolRoom == site.room;
            if (inRoom && !(story.clues & (1u << i)) && Distance(player, CluePosition(i)) < 2.3f)
            {
                pendingClue = int(i);
                Talk(site.name, site.text);
                return;
            }
        }
    }
    const auto& goal = story.Current();
    if (!story.Gym() && schoolRoom != 0 && story.step != Step::Reaction && story.step != Step::Gym)
    {
        return;
    }
    const Point position = story.step == Step::Report ? LumiPosition() : Point{goal.x, goal.y};
    if (story.step != Step::Complete && story.step != Step::Investigate &&
        goal.event == MainQuest::Event::Interact && Distance(player, position) < 2.3f)
    {
        pendingAdvance = true;
        Talk(goal.speaker, goal.dialogue);
        return;
    }
    if (story.step == Step::Complete && Distance(player, Places::Treasure) < 1.8f &&
        story.TakeTreasure())
    {
        Talk(
            "창고의 작은 보상",
            "체력 물약 2개와 마나 물약 2개를 얻었다. 학교의 악몽은 사라졌다! [E]를 누르면 마을로 돌아갑니다.");
        return;
    }
    for (const auto& staff : TownPopulation::StaffMembers())
    {
        if (staff.kind == 1 && staff.room == SceneRoom() &&
            Distance(player, {staff.x, staff.y}) < 1.6f)
        {
            Talk(staff.name, story.step == Step::Complete
                                 ? "이상한 소리가 멈췄구나. 다친 친구는 없니?"
                                 : "학생들은 안전한 곳으로 이동시켰어. 체육관 쪽 소리를 조심하렴.");
            return;
        }
    }
    if (!story.Gym() && !story.Fighting())
    {
        if (Distance(player, {3, 11}) < 1.8f)
        {
            Talk(
                "유나",
                "어제는 고마웠어. 그런데 오늘 방송실에서 내 이름을 부른 것 같아… 아직 수업 시작도 안 했는데?");
        }
        else if (Distance(player, {7, 11}) < 1.8f)
        {
            Talk("지수",
                 "도서관 책이 혼자 움직였다니까! 공부 대신 책이 우리를 읽는 시대가 온 거야?");
        }
    }
}

void DrawHud()
{
    Performance::Scope scope("cpu.quest_two.hud_ms");
    Panel(22, 20, 1000, 150);
    Text(42, 52, "메인 퀘스트 2 · 학교에 숨어든 것", 25, pink);
    if (!available)
    {
        Text(42, 90, "F3에서 퀘스트 1과 다음 날 1학년 1반의 마지막 대화를 완료하세요.", 20, gold);
        return;
    }
    Text(42, 88,
         awaitingHall              ? "조사 완료 · 출입문 [E]로 중앙 복도에 돌아가세요."
         : transformation.Active() ? "별빛 변신 중 · 연출 후 J로 전투가 시작됩니다."
                                   : story.Current().text,
         19, gold);
    std::string detail = "조사 " + std::to_string(story.ClueCount()) + "/3 · 검은 조각 " +
                         std::to_string(story.fragments);
    if (story.Fighting())
    {
        int alive = 0;
        for (const auto& enemy : combat.enemies)
        {
            alive += enemy.health > 0 ? 1 : 0;
        }
        detail += " · 남은 적 " + std::to_string(alive);
    }
    Text(42, 120, detail + " · " + SchoolLayout::Get(SceneRoom()).name, 17);
    Text(42, 150,
         story.step == Step::Complete && story.treasureTaken
             ? "[E]로 마을 귀환 · 편의점 직원에게 물약 구매 가능"
         : story.step == Step::Complete && story.sealOpened
             ? "봉인 해제 완료 · 보급품에 가까이 가서 [E]"
         : story.step == Step::Complete ? "출구로 돌아가세요"
                                        : "[E] 조사/대화",
         16, blue);
    CombatVitals::Draw(*graphics, width, height, combat.health, combat.mana, story.progress);
    Panel(22, 710, 1236, 76);
    Text(42, 740,
         "WASD 이동 · Shift 달리기 · J 공격 · K 별빛 폭발 (10) · L 별빛 낙하 (8) · U/I 물약 · E 조사", 17);
    Text(42, 770,
         "ESC 메뉴 · N 재시도 · F1 마을 · F2 사냥터 · F3 이전 이야기 · F4 퀘스트2 · F6 마을", 16,
         pink);
    if (!paused && !combat.Dead() && dialogue.empty() && (!story.Fighting() || awaitingHall) &&
        !transformation.Active() && story.step != Step::Reaction && available)
    {
        const auto& objective = story.Current();
        Point target{objective.x, objective.y};
        std::string targetHint = "[E] 조사";
        std::string distantHint = "목표";
        if (story.step == Step::Investigate)
        {
            float nearest = 1000;
            for (unsigned i = 0; i < 3; ++i)
            {
                if (schoolRoom != 0 && schoolRoom != Investigations()[i].room)
                {
                    continue;
                }
                const Point site = CluePosition(i);
                const float distance = Distance(player, site);
                if (!(story.clues & (1u << i)) && distance < nearest)
                {
                    nearest = distance;
                    target = site;
                    const auto& investigation = Investigations()[i];
                    if (schoolRoom == 0 && investigation.room != 0)
                    {
                        const auto& room = SchoolLayout::Get(investigation.room);
                        distantHint = std::string("목표 · ") + room.name;
                        targetHint = std::string(room.name) + " [E]";
                    }
                    else
                    {
                        targetHint = std::string(investigation.name) + " [E]";
                    }
                }
            }
            if (nearest == 1000)
            {
                target = RoomExit();
                targetHint = "[E] 복도로";
            }
        }
        if (story.step == Step::Complete)
        {
            targetHint = story.treasureTaken ? "[E] 마을로"
                         : story.sealOpened  ? "[E] 보급품"
                                             : "체육 창고";
            target = story.treasureTaken ? player
                     : story.sealOpened  ? Places::Treasure
                                         : Places::Seal;
        }
        if (!story.Gym() && schoolRoom != 0 && story.step != Step::Investigate &&
            story.step != Step::Reaction && story.step != Step::Gym)
        {
            target = RoomExit();
            targetHint = "[E] 복도로";
        }
        if (awaitingHall)
        {
            target = RoomExit();
            targetHint = "[E] 복도로 돌아가 변신";
        }
        if (story.step == Step::Gym && schoolRoom == 0)
        {
            target = {33, 3};
            distantHint = "목표 · 체육관";
            targetHint = "체육관 [E]";
        }
        float labelHeight = 114.f;
        if (story.step == Step::Report)
        {
            target = LumiPosition();
            targetHint = "루미 [E]";
            labelHeight = 116.f + 4 * std::sin(clockTime * 3);
        }
        Point p = Project({target.x, target.y, labelHeight});
        const Point low = Screen(240, 205), high = Screen(1200, 660);
        p.x = (std::max)(low.x, (std::min)(high.x, p.x));
        p.y = (std::max)(low.y, (std::min)(high.y, p.y));
        const std::string& text = Distance(player, target) <= 2.3f ? targetHint : distantHint;
        graphics->Nameplate(p.x, p.y, text, 16 * Scale(), 0, 0, 0, true);
    }
    if (!dialogue.empty())
    {
        Panel(200, 430, 1010, 200);
        Text(225, 467, speaker, 23, pink);
        // UTF-8 character-aware wrapping; no split inside a Korean code point.
        std::string line;
        int columns = 0, row = 0;
        for (size_t i = 0; i < dialogue.size();)
        {
            unsigned char c = static_cast<unsigned char>(dialogue[i]);
            size_t bytes = c < 0x80 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;
            int advance = c < 0x80 ? 1 : 2;
            if (columns + advance > 88)
            {
                Text(225, 505 + row++ * 28.f, line, 19);
                line.clear();
                columns = 0;
            }
            line += dialogue.substr(i, bytes);
            i += bytes;
            columns += advance;
        }
        Text(225, 505 + row * 28.f, line, 19);
        Text(1000, 609, "[E] 계속", 18, gold);
    }
    if (transformation.Active())
    {
        Panel(390, 580, 610, 64);
        Text(420, 622, "별빛이여, 깨어나라!", 27, pink);
    }
    if (story.step == Step::Reaction)
    {
        Panel(260, 460, 790, 120);
        Text(285, 501, "루미: 창밖에… 어제 골목에서 본 그림자야!", 22, pink);
        Text(285, 548,
             introTime < 4.f ? "연출 중 · 잠시 기다려주세요"
                             : "[E] 확인 · 학교의 이상 현상 조사 시작",
             18, gold);
    }
    if (combat.Dead() || paused)
    {
        Panel(300, 270, 700, 160);
        Text(330, 318, combat.Dead() ? "쓰러졌습니다" : "메인 퀘스트 2 · 일시정지", 27, pink);
        Text(330, 363, "[N] 다시 도전", 21);
        Text(330, 401, combat.Dead() ? "[M] 마을로 돌아가기 · 물약 정비" : "[ESC] 계속하기", 17);
    }
}
} // namespace

void Initialize(Renderer* renderer)
{
    graphics = renderer;
    models.Initialize();
}

void Resize(int w, int h)
{
    width = (std::max)(1, w);
    height = (std::max)(1, h);
}

void Open(bool unlocked, const Rpg::Progress& inherited, int fragments)
{
    available = unlocked;
    paused = false;
    keys.fill(false);
    if (!available || loaded)
    {
        return;
    }
    story = StoryState{};
    story.progress = inherited;
    story.fragments = fragments;
    loaded = true;
    EnterStep(true);
}

void Pause()
{
    paused = true;
    moving = false;
    keys.fill(false);
}

bool ConsumeWorldRequest()
{
    const bool requested = worldRequested;
    worldRequested = false;
    return requested;
}

bool Defeated()
{
    return loaded && combat.Dead();
}

const Rpg::Progress& Progress()
{
    return story.progress;
}

void Restock(int healthPotions, int manaPotions)
{
    story.progress.potions = healthPotions;
    story.progress.manaPotions = manaPotions;
    dialogue.clear();
    pendingAdvance = false;
    pendingClue = -1;
    EnterStep(true);
}

bool Finished()
{
    return available && loaded && story.step == Step::Complete && story.treasureTaken;
}

bool NeedsPotionShop()
{
    return Finished() && !story.shopVisited;
}

bool ShopProgress(Rpg::Progress& progress)
{
    if (!Finished())
    {
        return false;
    }
    progress = story.progress;
    return true;
}

bool BuyPotion(bool magic, std::string& message)
{
    if (!Finished())
    {
        message = "학교 사건을 해결하면 물약 판매가 시작됩니다.";
        return false;
    }
    Performance::Scope scope("cpu.shop.purchase_ms");
    if (!story.progress.BuyPotion(magic))
    {
        message = (magic ? story.progress.manaPotions : story.progress.potions) >= 99
                      ? "물약은 99개까지 보유할 수 있습니다."
                      : "골드가 부족합니다. 체력 물약 20골드 / 마나 물약 25골드";
        return false;
    }
    story.shopVisited = true;
    message = std::string(magic ? "마나 물약 +1" : "체력 물약 +1") + " · 남은 골드 " +
              std::to_string(story.progress.coins);
    Performance::Profiler::Get().Count("shop.potions_purchased_count");
    return true;
}

void StoreWorldPotions(int healthPotions, int magicPotions)
{
    if (!Finished())
    {
        return;
    }
    story.progress.potions = (std::max)(0, (std::min)(99, healthPotions));
    story.progress.manaPotions = (std::max)(0, (std::min)(99, magicPotions));
}

void StoreParkProgress(const Rpg::Progress& progress, bool foundFragment)
{
    if (Finished())
    {
        story.progress = progress;
        if (foundFragment)
        {
            ++story.fragments;
        }
    }
}

bool SchoolObjective(int currentRoom, float playerX, float playerY, int& room, float& x, float& y,
                     std::string& label)
{
    if (!available || !loaded || (story.step == Step::Complete && story.treasureTaken))
    {
        return false;
    }
    room = story.step == Step::Reaction ? 4 : story.step >= Step::Gym ? 2 : 0;
    Point target{story.Current().x, story.Current().y};
    label = story.Current().text;
    if (story.step == Step::Investigate)
    {
        float best = 10000;
        for (unsigned i = 0; i < Investigations().size(); ++i)
        {
            if (story.clues & (1u << i))
            {
                continue;
            }
            const auto& site = Investigations()[i];
            Point route{site.x, site.y};
            if (currentRoom == 0 && site.room != 0)
            {
                for (const auto& door : SchoolLayout::Doors())
                {
                    if (door.room == site.room)
                    {
                        route = {door.x, door.y};
                    }
                }
            }
            const float distance =
                site.room == currentRoom ? -1.f : Distance({playerX, playerY}, route);
            if (distance < best)
            {
                best = distance;
                room = site.room;
                target = {site.x, site.y};
                label = std::string(site.name) + "[E]";
            }
        }
    }
    if (story.step == Step::Complete)
    {
        target = story.sealOpened ? Places::Treasure : Places::Seal;
        label = story.sealOpened ? "체육관 창고 보급품 [E]" : "체육관 창고";
    }
    x = target.x;
    y = target.y;
    return true;
}

void EnterFromSchool(int room, float x, float y)
{
    if (!available || !loaded)
    {
        return;
    }
    if ((story.Gym() || story.step == Step::Gym) && room != 2)
    {
        return;
    }
    schoolRoom = room;
    player = {x, y};
    if (!Walkable(player))
    {
        player = story.Gym() || room == 2
                     ? Places::GymEntry
                     : Point{SchoolLayout::IntroPlayerX, SchoolLayout::IntroPlayerY};
    }
    if (awaitingHall && room == 0)
    {
        awaitingHall = false;
        transformation.remaining = 0;
    }
    camera = player;
    keys.fill(false);
    paused = false;
    BuildScene();
}

void Key(unsigned char key, bool down)
{
    if (key >= 'A' && key <= 'Z')
    {
        key += 'a' - 'A';
    }
    bool fresh = down && !keys[key];
    keys[key] = down;
    if (!fresh || !available || !loaded)
    {
        return;
    }
    if (key == 27)
    {
        paused = !paused;
        keys.fill(false);
        return;
    }
    if (Defeated() && key == CombatKeys::ReturnToTown)
    {
        worldRequested = true;
        keys.fill(false);
        return;
    }
    if (key == CombatKeys::Retry && (paused || combat.Dead()))
    {
        dialogue.clear();
        pendingAdvance = false;
        pendingClue = -1;
        EnterStep(true);
        paused = false;
        return;
    }
    if (paused || combat.Dead())
    {
        return;
    }
    if (key == 'e')
    {
        Interact();
        return;
    }
    if (!dialogue.empty())
    {
        return;
    }
    if (story.step == Step::Reaction)
    {
        return;
    }
    if (transformation.Active() || awaitingHall)
    {
        return;
    }
    if (key == 'f' && !story.Fighting())
    {
        Talk("루미", story.step == Step::Complete
                         ? "학교의 악몽은 사라졌어. 마을로 돌아가 편의점에 들러 보자!"
                         : "노란 목표를 따라가면 다음 단서를 찾을 수 있어. 내가 옆을 지킬게!");
        return;
    }
    if (key == ' ')
    {
        combat.actions.Jump();
    }
    if (key == CombatKeys::Burst && story.Fighting())
    {
        combat.Burst(player, encounter);
    }
    if (key == CombatKeys::Starfall && BattleReady())
    {
        combat.Starfall(player, facingDirection, encounter, Walkable);
    }
    if (key == CombatKeys::HealthPotion && combat.UsePotion(encounter))
    {
        story.progress = encounter.progress;
    }
    if (key == CombatKeys::ManaPotion && combat.actions.UseManaPotion(combat.mana, float(story.progress.stats.maxMana),
                                                   story.progress.manaPotions))
    {
        encounter.progress = story.progress;
    }
}

void Update(float dt)
{
    Performance::Scope scope("cpu.quest_two.update_ms");
    if (!available || !loaded || paused || combat.Dead())
    {
        return;
    }
    if (story.step == Step::Reaction)
    {
        introTime += dt;
        camera = {SchoolLayout::IntroPlayerX, 7.f};
        moving = false;
        return;
    }
    if (!dialogue.empty())
    {
        return;
    }
    clockTime += dt;
    if (transformation.Active())
    {
        if (transformation.Update(dt))
        {
            magicalForm = true;
            keys.fill(false);
            Performance::Profiler::Get().Count("quest_two.transformations_finished_count");
        }
        moving = false;
        return;
    }
    float horizontal = float(keys['d']) - float(keys['a']);
    float vertical = float(keys['s']) - float(keys['w']);
    float length = std::hypot(horizontal, vertical);
    moving = false;
    if (length > 0)
    {
        bool running = GameInput::SprintHeld();
        Point direction{(horizontal + vertical) / (length * 1.41421356f),
                        (vertical - horizontal) / (length * 1.41421356f)};
        Point before = player;
        facingDirection = direction;
        MainQuest::Combat::Move(player, direction, (running ? 5.f : 2.8f) * dt, Walkable);
        moving = Distance(before, player) > .0001f;
        if (horizontal != 0)
        {
            mirror = horizontal < 0;
        }
        walkFrame += dt * (running ? 13 : 9);
    }
    if (!awaitingHall)
    {
        combat.Update(dt, player, encounter, Walkable);
    }
    if (BattleReady() && !combat.Dead())
    {
        bool alive = false;
        for (const auto& enemy : combat.enemies)
        {
            alive = alive || enemy.health > 0;
        }
        if (!alive)
        {
            const bool alreadyRewarded = story.rewardClaimed;
            story.Advance(MainQuest::Event::Defeat);
            EnterStep();
            Talk(
                "학교 사건 해결",
                alreadyRewarded
                    ? "체육관의 악몽을 다시 물리쳤다! 기존 성장과 보상은 유지된다. E로 마을로 돌아가자."
                    : "체육관의 악몽이 사라졌다! 경험치 180, 골드 150, 물약을 받았다. E로 마을에 돌아가 쇼핑몰의 미아를 만나자.");
        }
        else if (keys[CombatKeys::Attack])
        {
            combat.Attack(player, facingDirection, encounter, combat.actions.height);
        }
    }
    float follow = 1 - std::exp(-8 * dt);
    camera.x += (player.x - camera.x) * follow;
    const float focusY = story.step == Step::Reaction && introTime < 4.f ? 7.f : player.y;
    camera.y += (focusY - camera.y) * follow;
    Performance::Profiler::Get().Gauge("quest_two.step_index", int(story.step));
}

void Draw()
{
    Performance::Scope scope("cpu.quest_two.draw_ms");
    graph.BeginSync();
    if (available && loaded)
    {
        if (!story.Fighting() && story.step != Step::Complete && (story.Gym() || schoolRoom == 0) &&
            Distance(player, RoomExit()) <= 2.1f)
        {
            const Point exit = RoomExit();
            Label("world/exit", {exit.x, exit.y, 75}, "[E] 마을로 나가기", gold);
        }
        auto& hero = graph.Place("hero", {player.x, player.y}, RenderLayer::World);
        Model("hero/shadow", {}, ModelId::Shadow, &hero, false, RenderLayer::Shadow);
        bool magical = magicalForm;
        ModelId model =
            magical
                ? static_cast<ModelId>(CharacterVisual::FrameIndex(combat.cast, moving, walkFrame))
                : static_cast<ModelId>(int(ModelId::StudentIdle) +
                                       (moving ? 1 + int(walkFrame) % 4 : 0));
        const auto morph =
            Transformation::Sample(Transformation::Duration - transformation.remaining,
                                   facingDirection.x, facingDirection.y);
        if (transformation.Active())
        {
            model = morph.model;
            Glow("hero/transformation", {0, 0, 40}, morph.glow, &hero, RenderLayer::Shadow);
        }
        bool castMirror = combat.cast > 0 ? combat.castMirror : mirror;
        const Point look = transformation.Active() ? Point{morph.x, morph.y}
                           : combat.cast > 0       ? combat.castDirection
                                                   : facingDirection;
        graph.Place(
            "hero/body", {0, 0, combat.actions.height + (transformation.Active() ? morph.lift : 0)},
            RenderLayer::World,
            [model, look, castMirror](const Actor&, const Transform& world)
            {
                const Point p = Project(world);
                graphics->DrawModel(models.GetFacing(model, look.x, look.y), p.x, p.y,
                                    Scale() * (models.loadedFromResource ? 1.f
                                               : castMirror              ? -1.f
                                                                         : 1.f),
                                    Scale());
            },
            &hero);
        if (combat.cast > 0)
        {
            Transform socket = CharacterVisual::FacingSocket(true, look.x, look.y, 40);
            socket.z += combat.actions.height;
            Glow("hero/wand", socket, 24, &hero);
        }
        const Point lumiPosition = LumiPosition();
        auto& lumi =
            Model("lumi", {lumiPosition.x, lumiPosition.y, 28 + 4 * std::sin(clockTime * 3)},
                  ModelId::Lumi);
        Label("lumi/name", {0, 0, 60},
              story.step == Step::Reaction || story.Fighting() || transformation.Active()
                  ? "루미"
                  : "루미 [F]",
              paper, &lumi);
        if (story.step == Step::Reaction && introTime < 4.f)
        {
            Model("intro/window_shadow",
                  {SchoolLayout::IntroWindowX - .45f + introTime / 4.f * .9f,
                   SchoolLayout::IntroWindowY, 61, .30f},
                  MonsterAnimation::Model(1, introTime, 0), nullptr, false, RenderLayer::Overlay);
        }
        for (size_t i = 0; i < combat.enemies.size(); ++i)
        {
            const auto enemy = combat.enemies[i];
            if (enemy.health <= 0 || !BattleReady())
            {
                continue;
            }
            std::string id = "enemy/" + std::to_string(i);
            auto& actor =
                Model(id, {enemy.position.x, enemy.position.y, 0, enemy.boss ? 1.3f : 1.f},
                      MonsterAnimation::FacingModel(
                          enemy.boss ? 11 : enemy.kind, clockTime,
                          enemy.boss ? MonsterAnimation::BossMotion(enemy.phase, enemy.timer)
                                     : enemy.attackMotion,
                          enemy.facing.x, enemy.facing.y));
            Model(id + "/shadow", {}, ModelId::Shadow, &actor, false, RenderLayer::Shadow);
            if (!enemy.boss && enemy.attackMotion > MonsterAnimation::ImpactRemaining)
            {
                Label(id + "/skill", {0, 0, 112}, MonsterSkills::Get(enemy.kind).warning, gold,
                      &actor);
            }
            Label(id + "/hp", {0, 0, enemy.boss ? 140.f : 75.f},
                  std::string(enemy.Enraged() ? "체육관의 악몽 · 격노 "
                              : enemy.boss ? "체육관의 악몽 " : MonsterAnimation::Name(enemy.kind)) +
                      " " + std::to_string(int(enemy.health)),
                  pink, &actor);
            if (enemy.boss && enemy.phase == 1)
            {
                const char* warnings[] = {"내려찍기", "돌진", "도약 강타", "부채꼴 마법탄"};
                Label(id + "/attack-name", {0, 0, 171}, warnings[enemy.pattern], gold, &actor);
                graph.Place(
                    id + "/warning", {}, RenderLayer::Shadow,
                    [enemy](const Actor&, const Transform&)
                    {
                        const Color warning{.95f, .12f, .22f, .45f};
                        const auto lane = [&](Point direction, float length, float halfWidth)
                        {
                            const Point start = enemy.position;
                            const Point end{start.x + direction.x * length,
                                            start.y + direction.y * length};
                            const Point side{-direction.y * halfWidth, direction.x * halfWidth};
                            const Point a = Project({start.x + side.x, start.y + side.y, 2});
                            const Point b = Project({end.x + side.x, end.y + side.y, 2});
                            const Point c = Project({end.x - side.x, end.y - side.y, 2});
                            const Point d = Project({start.x - side.x, start.y - side.y, 2});
                            Triangle(a, b, c, warning);
                            Triangle(a, c, d, warning);
                        };
                        if (enemy.pattern == 1)
                        {
                            lane(enemy.facing, enemy.ChargeSpeed() * .5f, 1.2f);
                            return;
                        }
                        if (enemy.pattern == 3)
                        {
                            const int spread = enemy.bossAttackEnraged ? 3 : 2;
                            for (int ray = -spread; ray <= spread; ++ray)
                            {
                                lane(MainQuest::BossFanDirection(enemy.facing, ray), 9.f, .4f);
                            }
                            return;
                        }
                        const Point target = enemy.pattern == 0 ? enemy.position : enemy.aim;
                        const float radius = enemy.BossRadius();
                        for (int j = 0; j < 32; ++j)
                        {
                            float a = j * 6.2831853f / 32, b = (j + 1) * 6.2831853f / 32;
                            Triangle(Project({target.x, target.y, 2}),
                                     Project({target.x + std::cos(a) * radius,
                                              target.y + std::sin(a) * radius, 2}),
                                     Project({target.x + std::cos(b) * radius,
                                              target.y + std::sin(b) * radius, 2}),
                                     warning);
                        }
                    },
                    &actor);
            }
            if (enemy.EnhancedBoss() && enemy.phase == 3 && enemy.pattern != 3 &&
                enemy.timer > enemy.BossRecovery() - .22f)
            {
                Glow(id + "/impact", {0, 0, 8}, 110.f, &actor, RenderLayer::Shadow);
            }
        }
        for (const auto& shot : combat.shots)
        {
            auto pose = CharacterVisual::FacingSpellPose(shot.position.x, shot.position.y, shot.age,
                                                         shot.direction.x, shot.direction.y, 40);
            pose.z += shot.launchHeight * (1 - (std::min)(1.f, shot.age / .18f));
            Glow("shot/" + std::to_string(shot.id), pose, CharacterVisual::SpellSize);
        }
        {
            Performance::Scope skillScope("cpu.quest_two.enemy_skills_sync_ms");
            auto& effects = graph.Place("hostile/skills", {}, RenderLayer::World);
            for (const auto& shot : combat.hostileShots)
            {
                graph.Place(
                    "hostile/shot/" + std::to_string(shot.id),
                    {shot.position.x, shot.position.y, 27}, RenderLayer::World,
                    [](const Actor&, const Transform& p)
                    {
                        Box(p, .2f, .2f, 16, {.92f, .25f, .56f});
                    },
                    &effects);
            }
            for (const auto& poison : combat.poisonZones)
            {
                graph.Place(
                    "hostile/poison/" + std::to_string(poison.id),
                    {poison.position.x, poison.position.y, 1}, RenderLayer::Ground,
                    [poison](const Actor&, const Transform& p)
                    {
                        const Color color = poison.age < MonsterSkills::Poison::Warning
                                                ? Color{1, .3f, .4f, .65f}
                                                : Color{.3f, .8f, .25f, .6f};
                        for (int i = 0; i < 24; ++i)
                        {
                            const float a = i * 6.2831853f / 24;
                            const float b = (i + 1) * 6.2831853f / 24;
                            const float radius = MonsterSkills::Poison::Radius;
                            Triangle(Project(p),
                                     Project({p.x + std::cos(a) * radius,
                                              p.y + std::sin(a) * radius, p.z}),
                                     Project({p.x + std::cos(b) * radius,
                                              p.y + std::sin(b) * radius, p.z}),
                                     color);
                        }
                    },
                    &effects);
            }
        }
        CombatEffects::Spells(graph, *graphics, combat, Scale(), Project);
        CombatEffects::Recovery(graph, *graphics, hero, combat.actions, combat.actions.height,
                                Scale(), Project);
    }
    graph.Place("hud", {}, RenderLayer::UserInterface,
                [](const Actor&, const Transform&)
                {
                    DrawHud();
                });
    graph.EndSync();
    graphics->BeginWorld();
    if (available && loaded)
    {
        graph.Render(RenderLayer::Background, RenderLayer::Overlay,
                     [](const Transform& p)
                     {
                         Point screen = Project(p);
                         float margin = 300 * Scale();
                         return screen.x > -margin && screen.x < width + margin &&
                                screen.y > -margin && screen.y < height + margin;
                     });
    }
    graphics->EndWorld(clockTime);
    graph.Render(RenderLayer::UserInterface, RenderLayer::UserInterface);
    graphics->Flush();
}

WorldMonitor::Scene MonitorSnapshot()
{
    WorldMonitor::Scene snapshot;
    snapshot.id = "quest_two";
    snapshot.name = "메인 퀘스트 2 · 학교에 숨어든 것";
    snapshot.paused = paused || !available || !dialogue.empty();
    snapshot.objective = available ? story.Current().text : "퀘스트 1 완료 대기";
    if (!loaded || !available)
    {
        return snapshot;
    }
    const Point lumi = LumiPosition();
    snapshot.npcs.push_back({"NPC_LUMI", "루미",
                             !dialogue.empty() && speaker == "루미" ? "대화 중" : "안내 대기",
                             lumi.x, lumi.y, 28 + 4 * std::sin(clockTime * 3), true});
    snapshot.npcs.push_back({"NPC_YUNA", "유나",
                             !dialogue.empty() && speaker == "유나" ? "대화 중" : "수업 대기", 3,
                             11, 0, !story.Gym() && schoolRoom == 0});
    snapshot.npcs.push_back({"NPC_JISOO", "지수",
                             !dialogue.empty() && speaker == "지수" ? "대화 중" : "수업 대기", 7,
                             11, 0, !story.Gym() && schoolRoom == 0});
    for (const auto& enemy : combat.enemies)
    {
        enemy.health > 0 ? ++snapshot.mobsAlive : ++snapshot.mobsDead;
    }
    snapshot.towns.push_back(
        {"school",
         SchoolLayout::Get(SceneRoom()).name,
         story.step == Step::Complete ? "사건 해결 · 일상 회복" : "학교 안에서 이상 현상 발생",
         {"사물함, 도서관, 방송실의 흔적이 체육관으로 이어진다."}});
    for (const auto& staff : TownPopulation::StaffMembers())
    {
        if (staff.kind == 1 && staff.room == SceneRoom())
        {
            snapshot.npcs.push_back({"MQ2_STAFF_" + std::to_string(staff.room), staff.name,
                                     story.step == Step::Complete ? "수업 재개" : "학생 대피 안내",
                                     staff.x, staff.y, 0, true});
        }
    }
    return snapshot;
}
} // namespace QuestTwo
