#include "stdafx.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include "Prologue.h"
#include "PrologueStory.h"
#include "QuestCombat.h"
#include "RuntimeFiles.h"
#include "PrimitiveModels.h"
#include "SceneGraph.h"
#include "Dependencies/freeglut.h"

namespace Prologue
{
namespace
{
using Game::Actor;
using Game::RenderLayer;
using Game::Transform;
using Hunting::ModelId;

using MainQuest::Distance;
using MainQuest::Point;
using QuestStep = MainQuest::Step;

struct Color
{
    float r, g, b, a = 1;
};

struct Obstacle
{
    float x, y, width, depth;
};

constexpr Point television = {2, 2};
constexpr Point door = {7, 5};
constexpr Point yuna = {19, 6};
constexpr Color ink = {.12f, .14f, .23f};
constexpr Color paper = {.96f, .94f, .91f};
constexpr Color pink = {.95f, .57f, .70f};
const std::array<Obstacle, 3> roomObstacles = {{
    {1, 1, 2, 1},
    {1, 5, 1.6f, 2},
    {5, 1, 2, 1.2f},
}};

Renderer* graphics = nullptr;
Hunting::PrimitiveModels models;
Game::SceneGraph scene;
Story story;
MainQuest::State quest;
MainQuest::Combat combat;
QuestStep enteredStep = QuestStep::Complete;
bool questDialogue = false, saveSucceeded = true, builtAlley = false;
bool confirmRestart = false;
bool restartFailed = false;
float transformTime = 0;
float endingTime = 0, farewellTime = 0;

bool EndingSchool()
{
    return quest.active && quest.step == QuestStep::Complete && endingTime >= 4;
}

bool InAlleyScene()
{
    return quest.InAlley() && !EndingSchool();
}

std::array<bool, 256> keys = {};
std::array<bool, 4> cameraKeys = {};
Point player = {4, 4.5f};
Point camera = player;
Point cameraOffset = {0, 0};
int width = 1280, height = 800;
float time = 0, walkFrame = 0, jumpHeight = 0, jumpVelocity = 0;
float facing = 1;
bool paused = false, moving = false;
bool moved = false, ran = false, jumped = false, looked = false;
bool builtHome = true;

bool RunHeld()
{
    return GetActiveWindow() == GetForegroundWindow() && (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
}

float Scale()
{
    return (std::min)(width / 1280.f, height / 800.f);
}

Point Project(Transform p)
{
    const float dx = p.x - camera.x, dy = p.y - camera.y;
    return {width * .5f + (dx - dy) * 40 * Scale(),
            height * .52f + ((dx + dy) * 20 - p.z) * Scale()};
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
    const float s = Scale();
    const float ox = (width - 1280 * s) * .5f, oy = (height - 800 * s) * .5f;
    Quad({ox + x * s, oy + y * s}, {ox + (x + w) * s, oy + y * s},
         {ox + (x + w) * s, oy + (y + h) * s}, {ox + x * s, oy + (y + h) * s}, color);
}

void Text(float x, float y, const std::string& value, float size = 20, Color color = paper)
{
    const float s = Scale();
    graphics->Text((width - 1280 * s) * .5f + x * s, (height - 800 * s) * .5f + y * s, value,
                   size * s, color.r, color.g, color.b, color.a);
}

void Floor(Transform p, float w, float d, Color color)
{
    Quad(Project(p), Project({p.x + w, p.y, p.z}), Project({p.x + w, p.y + d, p.z}),
         Project({p.x, p.y + d, p.z}), color);
}

void DialogueText(float x, float y, const std::string& value)
{
    // UTF-8 codepoint boundaries; Korean glyphs occupy approximately twice an ASCII column.
    std::string line;
    int columns = 0;
    for (size_t i = 0; i < value.size();)
    {
        const unsigned char lead = static_cast<unsigned char>(value[i]);
        const size_t bytes = lead < 0x80 ? 1 : lead < 0xE0 ? 2 : lead < 0xF0 ? 3 : 4;
        const int advance = lead < 0x80 ? 1 : 2;
        if (columns + advance > 102)
        {
            Text(x, y, line, 19);
            y += 29;
            line.clear();
            columns = 0;
        }
        line.append(value, i, bytes);
        columns += advance;
        i += bytes;
    }
    Text(x, y, line, 19);
}

void Box(Transform p, float w, float d, float h, Color color)
{
    const Point a = Project({p.x, p.y + d, p.z});
    const Point b = Project({p.x + w, p.y + d, p.z});
    const Point c = Project({p.x + w, p.y, p.z});
    const Point at = Project({p.x, p.y + d, p.z + h});
    const Point bt = Project({p.x + w, p.y + d, p.z + h});
    const Point ct = Project({p.x + w, p.y, p.z + h});
    Quad(a, b, bt, at, {color.r * .8f, color.g * .8f, color.b * .8f, color.a});
    Quad(b, c, ct, bt, {color.r * .65f, color.g * .65f, color.b * .65f, color.a});
    Floor({p.x, p.y, p.z + h}, w, d, color);
}

Actor& PlaceModel(const std::string& name, Transform p, ModelId model,
                  RenderLayer layer = RenderLayer::World, Actor* parent = nullptr)
{
    return scene.Place(
        name, p, layer,
        [model](const Actor&, const Transform& world)
        {
            Point s = Project(world);
            graphics->DrawModel(models.Get(model), s.x, s.y, world.scale * Scale(),
                                world.scale * Scale());
        },
        parent);
}

void Label(const std::string& name, Transform p, const std::string& text, Actor* parent = nullptr)
{
    scene.Place(
        name, p, RenderLayer::Overlay,
        [text](const Actor&, const Transform& world)
        {
            Point s = Project(world);
            graphics->Nameplate(s.x, s.y, text, 17 * Scale());
        },
        parent);
}

bool Walkable(Point p)
{
    Performance::Profiler::Get().Count("prologue.collision_checks");
    const float margin = .22f;
    const float xmax = story.IsHome() || EndingSchool() ? 7.6f : 23.6f;
    const float ymax = story.IsHome() || EndingSchool() ? 7.6f : 13.6f;
    if (p.x < .4f || p.y < .4f || p.x > xmax || p.y > ymax)
    {
        return false;
    }
    if (EndingSchool())
    {
        return p.y >= 2;
    }
    if (story.IsHome())
    {
        for (const auto& box : roomObstacles)
        {
            if (p.x > box.x - margin && p.x < box.x + box.width + margin && p.y > box.y - margin &&
                p.y < box.y + box.depth + margin)
            {
                return false;
            }
        }
    }
    else if (InAlleyScene())
    {
        return p.x >= 1 && p.x <= 22 && p.y >= 5.5f && p.y <= 11.5f;
    }
    else
    {
        // The road and school entrance remain connected. Buildings are not jump-through.
        if ((p.x > 17 && p.x < 22 && p.y < 4.4f) || (!quest.active && Distance(p, yuna) < .55f))
        {
            return false;
        }
    }
    return true;
}

void BuildScene()
{
    Performance::Scope scope("cpu.prologue.static_scene_build_ms");
    scene.Clear();
    builtHome = story.IsHome();
    builtAlley = InAlleyScene();
    auto& root = scene.Place("environment", {}, RenderLayer::Ground);
    const int columns = builtHome || EndingSchool() ? 8 : 24;
    const int rows = builtHome || EndingSchool() ? 8 : 14;
    for (int y = 0; y < rows; ++y)
    {
        for (int x = 0; x < columns; ++x)
        {
            const bool road = y >= 7 && y <= 10;
            Color color = builtHome    ? Color{.73f, .60f, .50f}
                          : builtAlley ? Color{.30f, .29f, .36f}
                          : road       ? Color{.75f, .75f, .78f}
                                       : Color{.52f, .68f, .57f};
            scene.Place(
                "floor/" + std::to_string(y * columns + x), {float(x), float(y)},
                RenderLayer::Ground,
                [color](const Actor&, const Transform& world)
                {
                    Floor(world, 1.005f, 1.005f, color);
                },
                &root);
        }
    }
    if (EndingSchool())
    {
        scene.Place(
            "classroom/window", {}, RenderLayer::World,
            [](const Actor&, const Transform& p)
            {
                Box(p, 8, .1f, 105, {.80f, .85f, .88f});
                Box({.5f, .2f, 30}, 7, .05f, 62, {.43f, .66f, .78f});
                for (int i = 1; i < 8; ++i)
                {
                    Box({float(i), .28f, 30}, .06f, .04f, 62, paper);
                }
                Box({.5f, 1, 0}, 1.3f, .7f, 25, {.66f, .48f, .34f});
                Box({5.5f, 1, 0}, 1.3f, .7f, 25, {.66f, .48f, .34f});
            },
            &root);
        Label("classroom/name", {4, 0, 125}, "다음 날 · 별빛 학교 교실", &root);
    }
    else if (builtHome)
    {
        scene.Place(
            "walls", {}, RenderLayer::World,
            [](const Actor&, const Transform& p)
            {
                Box(p, 8, .12f, 95, {.87f, .83f, .86f});
                Box(p, .12f, 8, 95, {.83f, .78f, .83f});
                Box({.13f, 3, 40}, .05f, 1.8f, 40, {.56f, .78f, .89f});
            },
            &root);
        scene.Place(
            "tv", {1, 1}, RenderLayer::World,
            [](const Actor&, const Transform& p)
            {
                Box(p, 2, 1, 25, {.51f, .35f, .30f});
                Box({p.x + .2f, p.y + .2f, 25}, 1.5f, .15f, 40, ink);
                Box({p.x + .26f, p.y + .37f, 30}, 1.35f, .02f, 29, {.40f, .67f, .81f});
            },
            &root);
        scene.Place(
            "bed", {1, 5}, RenderLayer::World,
            [](const Actor&, const Transform& p)
            {
                Box(p, 1.6f, 2, 18, {.58f, .39f, .42f});
                Box({p.x, p.y, 18}, 1.6f, 2, 8, pink);
                Box({p.x + .15f, p.y + .1f, 26}, 1.3f, .5f, 4, paper);
            },
            &root);
        scene.Place(
            "desk", {5, 1}, RenderLayer::World,
            [](const Actor&, const Transform& p)
            {
                Box(p, 2, 1.2f, 30, {.65f, .47f, .35f});
                Box({p.x + .2f, p.y + .3f, 30}, .65f, .55f, 3, paper);
                Box({p.x + 1.1f, p.y + .2f, 30}, .5f, .6f, 6, pink);
            },
            &root);
        scene.Place(
            "door", {6.7f, 4.6f}, RenderLayer::Ground,
            [](const Actor&, const Transform& p)
            {
                Floor(p, .8f, 1, {.94f, .78f, .46f});
            },
            &root);
        Label("tv/name", {2, 1.5f, 83}, "TV · 아침 뉴스", &root);
        Label("door/name", {7, 5, 30}, "현관", &root);
    }
    else if (builtAlley)
    {
        for (int i = 0; i < 6; ++i)
        {
            scene.Place(
                "alley/wall/" + std::to_string(i), {1.f + i * 3.7f, 3.5f}, RenderLayer::World,
                [](const Actor&, const Transform& p)
                {
                    Box(p, 3.6f, 1.8f, 85, {.39f, .36f, .43f});
                    Box({p.x + .5f, p.y + 1.82f, 40}, 1.2f, .03f, 25, {.58f, .49f, .59f});
                },
                &root);
        }
        Label("alley/name", {11, 5, 110}, "학교 뒤 오래된 골목", &root);
    }
    else
    {
        PlaceModel("school", {19.5f, 4, 0, 2.1f}, ModelId::Shelter, RenderLayer::World, &root);
        Label("school/name", {19.5f, 4, 255}, "별빛 학교", &root);
        for (int i = 0; i < 8; ++i)
        {
            PlaceModel("tree/" + std::to_string(i), {3.f + i * 2, 12.5f}, ModelId::Tree,
                       RenderLayer::World, &root);
        }
        for (int i = 0; i < 5; ++i)
        {
            PlaceModel("garden/" + std::to_string(i), {3.f + i * 2.5f, 3}, ModelId::Tree,
                       RenderLayer::World, &root);
        }
        if (!quest.active)
        {
            auto& friendActor =
                PlaceModel("NPC_YUNA", {yuna.x, yuna.y}, ModelId::Yuna, RenderLayer::World, &root);
            PlaceModel("NPC_YUNA/shadow", {}, ModelId::Shadow, RenderLayer::Shadow, &friendActor);
            Label("NPC_YUNA/name", {0, 0, 83}, "유나 [E]", &friendActor);
        }
        else
        {
            const ModelId npcModels[] = {ModelId::Jisoo, ModelId::SchoolGuard, ModelId::StoreClerk};
            const char* names[] = {"지수", "학교 경비원", "편의점 직원"};
            for (int i = 0; i < 3; ++i)
            {
                const auto& objective = MainQuest::Objectives()[i];
                const auto id = "quest/npc/" + std::to_string(i);
                auto& npc = PlaceModel(id, {objective.x, objective.y}, npcModels[i],
                                       RenderLayer::World, &root);
                PlaceModel(id + "/shadow", {}, ModelId::Shadow, RenderLayer::Shadow, &npc);
                Label(id + "/name", {0, 0, 85}, names[i], &npc);
            }
            PlaceModel("store", {9, 7, 0, .65f}, ModelId::Shelter, RenderLayer::World, &root);
            Label("store/name", {9, 7, 95}, "별빛 편의점", &root);
            Label("alley/entrance", {22, 12, 40}, "학교 뒤 골목 [E]", &root);
        }
        Label("road/sign", {8, 7, 28}, "학교 가는 길 →", &root);
    }
    scene.RetainSubtree(root);
}

void Reset()
{
    confirmRestart = restartFailed = false;
    endingTime = farewellTime = 0;
    story = Story{};
    quest = MainQuest::State{};
    combat = MainQuest::Combat{};
    enteredStep = QuestStep::Complete;
    questDialogue = false;
    player = {4, 4.5f};
    camera = player;
    cameraOffset = {0, 0};
    keys.fill(false);
    cameraKeys.fill(false);
    jumpHeight = jumpVelocity = walkFrame = time = 0;
    paused = moving = moved = ran = jumped = looked = false;
    BuildScene();
    Performance::Profiler::Get().Count("prologue.restarts");
}

void EnterQuestStep(bool restorePosition = false)
{
    if (!quest.active)
    {
        return;
    }
    const auto previous = enteredStep;
    enteredStep = quest.step;
    combat.Enter(quest);
    transformTime = 0;
    questDialogue = false;
    farewellTime = 0;
    if (quest.step == QuestStep::Complete)
    {
        endingTime = quest.epilogueComplete ? 8.f : 0.f;
    }
    if (restorePosition)
    {
        const auto& target = quest.Current();
        player = {target.x - 1.5f, target.y + .5f};
        if (quest.step == QuestStep::Escape)
        {
            player = {16, 8};
        }
        if (quest.step == QuestStep::Shadows || quest.step == QuestStep::Boss)
        {
            player = {4, 9};
        }
    }
    if (previous == QuestStep::Alley && quest.step == QuestStep::Fragment)
    {
        player = {2, 8};
    }
    if (EndingSchool())
    {
        player = {4, 4.5f};
    }
    if (builtAlley != InAlleyScene())
    {
        BuildScene();
    }
    jumpHeight = jumpVelocity = 0;
    cameraOffset = {0, 0};
    camera = player;
    saveSucceeded = quest.Save();
    Performance::Profiler::Get().Count("quest.step_entered");
}

void UpdateQuest(float dt)
{
    Performance::Scope scope("cpu.quest.story_update_ms");
    if (!quest.active && story.step == Step::Complete)
    {
        quest.active = true;
        BuildScene();
    }
    if (!quest.active)
    {
        return;
    }
    if (enteredStep != quest.step)
    {
        EnterQuestStep();
    }
    if (questDialogue || combat.Dead())
    {
        return;
    }
    if (quest.step == QuestStep::Report)
    {
        farewellTime += dt;
    }
    if (quest.step == QuestStep::Complete)
    {
        const bool wasSchool = EndingSchool();
        endingTime += dt;
        if (!wasSchool && EndingSchool())
        {
            player = {4, 4.5f};
            camera = player;
            cameraOffset = {0, 0};
            jumpHeight = jumpVelocity = 0;
            BuildScene();
        }
        return;
    }
    if (quest.step == QuestStep::Escape && Distance(player, {3, 8}) < 1.2f)
    {
        quest.Emit(MainQuest::Event::Arrive);
    }
    if (quest.step == QuestStep::Transform)
    {
        transformTime += dt;
        if (transformTime >= 3)
        {
            quest.Emit(MainQuest::Event::Timer);
        }
    }
    if (enteredStep == quest.step)
    {
        combat.Update(dt, player, quest, Walkable);
    }
    if (enteredStep != quest.step)
    {
        EnterQuestStep();
    }
}

void Interact()
{
    if (quest.active && quest.step == QuestStep::Complete)
    {
        if (endingTime >= 7 && quest.FinishEpilogue())
        {
            keys.fill(false);
            saveSucceeded = quest.Save();
        }
        return;
    }
    if (quest.active)
    {
        if (combat.Dead())
        {
            return;
        }
        if (questDialogue)
        {
            questDialogue = false;
            quest.Emit(MainQuest::Event::Interact);
            EnterQuestStep();
            return;
        }
        const auto& target = quest.Current();
        if (quest.step == QuestStep::Report && farewellTime < 3)
        {
            return;
        }
        if (quest.step != QuestStep::Complete && target.event == MainQuest::Event::Interact &&
            Distance(player, {target.x, target.y}) < 1.8f && jumpHeight <= 0)
        {
            questDialogue = true;
            moving = false;
        }
        return;
    }
    if (story.dialogueOpen)
    {
        story.AdvanceDialogue();
        Performance::Profiler::Get().Count("prologue.dialogue_advance");
        return;
    }
    if (jumpHeight > 0)
    {
        return;
    }
    if (story.step == Step::News && Distance(player, television) < 1.8f)
    {
        story.InteractNews();
    }
    else if (story.step == Step::LeaveHome && Distance(player, door) < 1.3f)
    {
        story.LeaveHome();
        player = {2, 9};
        camera = player;
        cameraOffset = {0, 0};
        BuildScene();
    }
    else if (story.step == Step::TalkYuna && Distance(player, yuna) < 1.8f)
    {
        story.TalkYuna();
    }
}

void DrawHud()
{
    Panel(22, 20, 810, 115, {.08f, .10f, .18f, .94f});
    Text(42, 53,
         quest.epilogueComplete ? "메인 퀘스트 2 · 학교에 숨어든 것"
         : quest.active         ? "메인 퀘스트 1 · 연락이 끊긴 친구"
         : story.IsHome()       ? "프롤로그 · 평범한 아침"
                                : "프롤로그 · 학교의 소문",
         25, pink);
    Text(42, 87,
         quest.epilogueComplete ? "새 퀘스트: 학교에 숨어든 것 (#19 구간 준비 중)"
         : quest.active         ? quest.Current().text
                                : story.Objective(),
         19);
    if (quest.active)
    {
        Text(42, 117,
             "HP " + std::to_string(int(combat.health)) + " / " +
                 std::to_string(quest.progress.stats.maxHealth) + "   MP " +
                 std::to_string(int(combat.mana)) + "   검은 조각 " +
                 std::to_string(quest.fragments) + "   물약 " +
                 std::to_string(quest.progress.potions) + " [Z]" +
                 (quest.CanFight() && !EndingSchool() ? "   마법소녀" : "   평범한 학생"),
             15);
        if (quest.step == QuestStep::Shadows)
        {
            Text(42, 159, "그림자 처치 " + std::to_string(quest.defeated) + " / 3", 20, pink);
        }
        if (!saveSucceeded)
        {
            Text(42, 188, "저장 실패: 현재 진행은 유지되지만 재실행 시 복원되지 않을 수 있습니다.",
                 16);
        }
    }
    else
    {
        Text(42, 117, "평범한 학생 · 아직 마법이나 전투 능력이 없습니다", 15, {.70f, .77f, .86f});
    }

    Panel(22, 640, 1236, 137, {.08f, .10f, .18f, .94f});
    Text(
        42, 670,
        quest.CanFight()
            ? "Space 점프 · Q 공격 · R 재충전 · Z 물약 · 1 마법 | WASD 이동 · Shift 달리기 · E 대화"
            : "WASD 이동 | Shift 달리기 | 스페이스 점프 | E 상호작용 | Z 물약(퀘스트 중)",
        18);
    Text(42, 702,
         "방향키 카메라 · C 복귀 · ESC 메뉴 | 충전 " + std::to_string(combat.actions.charges) +
             "/6" + (combat.actions.reload > 0 ? " · 재충전 중" : ""),
         18);
    Text(42, 736, "F1 튜토리얼 / F2 사냥터 / F3 프롤로그·퀘스트1 / F4 퀘스트2 / F5 로그", 17, pink);
    Text(42, 760,
         std::string(moved ? "[완료] 이동  " : "[ ] 이동  ") +
             (looked ? "[완료] 카메라  " : "[ ] 카메라  ") +
             (ran ? "[완료] 달리기  " : "[ ] 달리기  ") + (jumped ? "[완료] 점프" : "[ ] 점프"),
         15);

    if (story.IsHome() && time < 7 && !story.dialogueOpen)
    {
        Panel(190, 520, 900, 70, {.10f, .16f, .24f, .95f});
        Text(220, 562, "TV 뉴스: 최근 도시에서 학생 실종 신고가 증가하고 있습니다.", 21);
    }
    Point target = quest.active               ? Point{quest.Current().x, quest.Current().y}
                   : story.step == Step::News ? television
                   : story.IsHome()           ? door
                                              : yuna;
    if (!story.dialogueOpen && !questDialogue &&
        ((quest.active && quest.step != QuestStep::Complete) || story.step < Step::AfterSchool))
    {
        Point p = Project({target.x, target.y, 105});
        // Keep distant objectives inside the readable HUD area.
        p.x = (std::max)(120 * Scale(), (std::min)(width - 120 * Scale(), p.x));
        p.y = (std::max)(210 * Scale(), (std::min)(height - 220 * Scale(), p.y));
        graphics->Nameplate(
            p.x, p.y,
            Distance(player, target) < 1.8f &&
                    (!quest.active || quest.Current().event == MainQuest::Event::Interact)
                ? "[E] 상호작용"
                : "목표",
            18 * Scale());
    }
    if (story.dialogueOpen)
    {
        const auto line = story.CurrentLine();
        Panel(110, 440, 1060, 170, {.07f, .08f, .15f, .98f});
        Text(140, 476, line.speaker, 24, pink);
        Text(140, 522, line.text, 22);
        Text(140, 579, "[E] 다음", 18);
    }
    if (questDialogue)
    {
        Panel(70, 430, 1140, 180, {.07f, .08f, .15f, .98f});
        Text(100, 473, quest.Current().speaker, 24, pink);
        DialogueText(100, 513, quest.Current().dialogue);
        Text(100, 579, "[E] 확인하고 계속", 18);
    }
    if (story.step == Step::AfterSchool)
    {
        Panel(0, 0, 1280, 800, {.07f, .06f, .13f, .96f});
        Text(450, 373, "그리고, 방과 후", 36, pink);
        Text(350, 423, "종이 울리고 교실 밖으로 노을이 번집니다.", 23);
    }
    if (quest.active && quest.step == QuestStep::Complete)
    {
        if (endingTime < 4)
        {
            Panel(0, 0, 1280, 800, {.06f, .07f, .14f, .97f});
            Text(360, 340, "유나는 무사히 집으로 돌아갔다.", 28, pink);
            Text(360, 392, "그리고 다음 날, 학교…", 25);
            Text(230, 470, "퀘스트 보상: 검은 조각 +3 / 경험치 +120 / 골드 +100", 20);
        }
        else if (!quest.epilogueComplete)
        {
            Panel(150, 440, 980, 160, {.08f, .07f, .16f, .96f});
            Text(180, 480, endingTime < 7 ? "나" : "루미", 25, pink);
            Text(180, 525,
                 endingTime < 7 ? "검은 조각이… 반응하고 있어. 창밖에 저건?!" : "또 나타났어.", 24);
            Text(180, 568,
                 endingTime < 7 ? "연출 중 · 잠시 기다려주세요"
                                : "[E] 확인 · 다음 퀘스트 시작 / 이동 재개",
                 18);
        }
        else
        {
            Panel(22, 140, 810, 90, {.08f, .07f, .16f, .94f});
            Text(42, 176, "#18 완료 · 새 퀘스트: 학교에 숨어든 것", 23, pink);
            Text(42, 210, "교실 이동 가능 · #19 조사/전투 구간은 다음 작업입니다.", 18);
        }
    }
    if (quest.active && quest.step == QuestStep::Transform)
    {
        Panel(320, 440, 640, 90, {.17f, .08f, .25f, .92f});
        Text(365, 490, "별빛이 깨어납니다… 첫 변신!", 28, pink);
    }
    if (quest.active && combat.Dead())
    {
        Panel(260, 300, 760, 220, {.10f, .04f, .12f, .97f});
        Text(300, 356, "쓰러졌습니다", 30, pink);
        Text(300, 405, "[N] 현재 퀘스트 단계부터 다시 도전", 23);
        Text(300, 460, "조사한 단서와 완료한 단계는 유지됩니다.", 18);
    }
    if (paused)
    {
        Panel(260, 245, 760, 320, {.06f, .07f, .13f, .98f});
        Text(310, 305, "프롤로그 일시정지", 30, pink);
        Text(310, 365,
             quest.active ? "ESC 계속하기 / N 현재 단계 재시작"
                          : "ESC 계속하기 / N 프롤로그 처음부터",
             23);
        Text(310, 415, "기존 사냥 레벨의 성장 데이터는 변경하지 않습니다.", 18);
        Text(310, 459, "[B] 프롤로그부터 새로 시작 (기존 저장 백업)", 20, pink);
        if (confirmRestart)
        {
            Text(310, 505, "Enter: 처음부터 시작 / ESC: 취소", 22);
        }
        if (restartFailed)
        {
            Text(310, 541, "백업 실패: 진행을 초기화하지 않았습니다.", 18);
        }
    }
}

void Circle(Transform p, float radius, Color color)
{
    for (int i = 0; i < 32; ++i)
    {
        const float a = i * 6.2831853f / 32;
        const float b = (i + 1) * 6.2831853f / 32;
        Triangle(Project(p), Project({p.x + std::cos(a) * radius, p.y + std::sin(a) * radius, p.z}),
                 Project({p.x + std::cos(b) * radius, p.y + std::sin(b) * radius, p.z}), color);
    }
}

void PlaceGlow(const std::string& id, Transform pose, float size, Actor* parent = nullptr)
{
    scene.Place(
        id, pose, RenderLayer::World,
        [size](const Actor&, const Transform& world)
        {
            Point p = Project(world);
            const float s = size * Scale();
            graphics->Effect(p.x - s * .5f, p.y - s * .5f, s, s, time, 2);
        },
        parent);
}

void DrawQuestActors()
{
    Performance::Scope scope("cpu.quest.actor_sync_ms");
    if (EndingSchool())
    {
        auto& lumi = PlaceModel("ending/lumi", {player.x + 1, player.y, 30}, ModelId::Lumi);
        Label("ending/lumi/name", {0, 0, 60}, "루미", &lumi);
        if (!quest.epilogueComplete)
        {
            auto& shard = PlaceModel("ending/fragment", {player.x - .5f, player.y, 35},
                                     ModelId::BlackFragment);
            PlaceGlow("ending/fragment/glow", {}, 45 + 12 * std::sin(time * 9), &shard);
            if (endingTime < 7)
            {
                PlaceModel("ending/window_shadow", {.6f + (endingTime - 4) * 2.2f, .4f, 40, .6f},
                           ModelId::Shade0);
            }
        }
        return;
    }
    if (!InAlleyScene())
    {
        return;
    }
    if (quest.step == QuestStep::Fragment)
    {
        auto& shard = PlaceModel("clue/fragment", {5, 8}, ModelId::BlackFragment);
        PlaceGlow("clue/fragment/glow", {0, 0, 17}, 40, &shard);
    }
    if (quest.step <= QuestStep::Bag)
    {
        PlaceModel("clue/bag", {10, 8}, ModelId::YunaBag);
    }
    if (quest.step <= QuestStep::CheckYuna)
    {
        auto& friendActor =
            PlaceModel("alley/yuna", {17, 8},
                       quest.step == QuestStep::CheckYuna ? ModelId::Yuna : ModelId::YunaFallen);
        PlaceModel("alley/yuna/shadow", {}, ModelId::Shadow, RenderLayer::Shadow, &friendActor);
        Label("alley/yuna/name", {0, 0, 75}, "유나", &friendActor);
    }
    if (quest.step == QuestStep::Report && farewellTime < 3)
    {
        PlaceModel("yuna/leaving", {17 - farewellTime * 5, 8}, ModelId::Yuna);
    }
    if (quest.step >= QuestStep::Lumi)
    {
        const Point p = quest.step >= QuestStep::Report ? Point{5, 8} : Point{3, 7};
        auto& lumi = PlaceModel("lumi", {p.x, p.y, 25 + std::sin(time * 3) * 5}, ModelId::Lumi);
        Label("lumi/name", {0, 0, 60}, "루미", &lumi);
    }
    if (quest.step >= QuestStep::Escape && quest.step <= QuestStep::Boss)
    {
        scene.Place("alley/seal", {1, 5.5f}, RenderLayer::World,
                    [](const Actor&, const Transform& world)
                    {
                        Box(world, .1f, 6, 80, {.51f, .22f, .64f, .5f});
                    });
    }
    for (size_t i = 0; i < combat.enemies.size(); ++i)
    {
        const auto enemy = combat.enemies[i];
        if (enemy.health <= 0)
        {
            continue;
        }
        const auto id = "enemy/" + std::to_string(i);
        const float lift = enemy.boss && enemy.pattern == 2 && enemy.phase == 1
                               ? std::sin((1.1f - enemy.timer) / 1.1f * 3.14159f) * 55
                               : 0;
        auto& actor = PlaceModel(id, {enemy.position.x, enemy.position.y, lift},
                                 enemy.boss ? ModelId::ShadowEater : ModelId::Shade0);
        PlaceModel(id + "/shadow", {0, 0, -lift}, ModelId::Shadow, RenderLayer::Shadow, &actor);
        Label(id + "/hp", {0, 0, enemy.boss ? 110.f : 70.f},
              std::string(enemy.boss ? "그림자 포식자 " : "그림자 ") +
                  std::to_string(int(enemy.health)),
              &actor);
        if (enemy.boss && enemy.phase == 1)
        {
            const Point aim = enemy.pattern == 0 ? enemy.position : enemy.aim;
            scene.Place(id + "/warning", {aim.x, aim.y, 1}, RenderLayer::Shadow,
                        [enemy](const Actor&, const Transform& world)
                        {
                            Circle(world, enemy.pattern == 0 ? 2.4f : 2.f,
                                   {.95f, .15f, .22f, .32f + .12f * std::sin(time * 15)});
                            if (enemy.pattern == 1)
                            {
                                for (int j = 0; j < 6; ++j)
                                {
                                    const float t = j / 5.f;
                                    Circle({enemy.position.x + (enemy.aim.x - enemy.position.x) * t,
                                            enemy.position.y + (enemy.aim.y - enemy.position.y) * t,
                                            1},
                                           .65f, {.95f, .15f, .22f, .25f});
                                }
                            }
                        });
        }
    }
    for (const auto& shot : combat.shots)
    {
        auto pose =
            CharacterVisual::SpellPose(shot.position.x, shot.position.y, shot.age, shot.mirror, 40);
        pose.z += shot.launchHeight * (1 - (std::min)(1.f, shot.age / .18f));
        PlaceGlow("shot/" + std::to_string(shot.id), pose, CharacterVisual::SpellSize);
    }
    if (combat.burst > 0)
    {
        scene.Place(
            "star_burst", {combat.burstOrigin.x, combat.burstOrigin.y, 2}, RenderLayer::Shadow,
            [](const Actor&, const Transform& world)
            {
                Circle(world, 3 * (1 - combat.burst * .6f), {.96f, .45f, .80f, combat.burst});
            });
    }
}
} // namespace

void Initialize(Renderer* renderer)
{
    graphics = renderer;
    models.Initialize();
    Reset();
    if (quest.Load())
    {
        story.step = Step::Complete;
        BuildScene();
        EnterQuestStep(true);
    }
}

void Resize(int w, int h)
{
    width = (std::max)(1, w);
    height = (std::max)(1, h);
}

WorldMonitor::Scene MonitorSnapshot()
{
    WorldMonitor::Scene snapshot;
    snapshot.id = "story";
    snapshot.name = "프롤로그 · 메인 스토리";
    snapshot.paused = paused;
    snapshot.objective = quest.epilogueComplete ? "#18 완료 / 다음 퀘스트 활성화"
                         : quest.active ? quest.Current().text : story.Objective();
    std::string location = story.IsHome() ? "주인공의 방"
                           : EndingSchool() ? "다음 날 교실"
                           : InAlleyScene() ? "학교 뒤 골목" : "학교/마을 거리";
    std::string mood = quest.CanFight() && quest.step <= QuestStep::Boss ? "전투 중"
                       : quest.active && quest.step == QuestStep::Escape ? "도주 중"
                       : quest.epilogueComplete ? "새로운 이상 징후" : "학생 실종으로 불안";
    snapshot.towns.push_back({"story_region", location, mood,
                              {"학생 실종 신고가 늘고 있다.", "마지막 목격 장소는 학교 뒤 골목이다."}});
    auto add = [&](const char* id, const char* name, const std::string& state, Point p, bool present)
    {
        snapshot.npcs.push_back({id, name, state, p.x, p.y, 0, present});
    };
    if (!quest.active)
    {
        add("NPC_YUNA", "유나", story.dialogueOpen && !story.IsHome() ? "대화 중" : "등교 대기",
            yuna, !story.IsHome());
    }
    else
    {
        bool outside = !InAlleyScene() && !EndingSchool();
        const char* ids[] = {"NPC_JISOO", "NPC_GUARD", "NPC_CLERK"};
        const char* names[] = {"지수", "학교 경비원", "편의점 직원"};
        for (int i = 0; i < 3; ++i)
        {
            const auto& objective = MainQuest::Objectives()[i];
            add(ids[i], names[i], !outside ? "현재 지역 밖"
                : questDialogue && int(quest.step) == i ? "대화 중" : "대기",
                {objective.x, objective.y}, outside);
        }
        const bool leaving = quest.step == QuestStep::Report && farewellTime < 3;
        const bool yunaPresent = InAlleyScene() && (quest.step <= QuestStep::CheckYuna || leaving);
        add("NPC_YUNA", "유나", leaving ? "귀가 중" : quest.step < QuestStep::Fragment ? "실종"
            : quest.step < QuestStep::CheckYuna ? "쓰러짐" : quest.step == QuestStep::CheckYuna ? "구조됨" : "귀가",
            leaving ? Point{17 - farewellTime * 5, 8} : Point{17, 8}, yunaPresent);
        const bool lumiPresent = EndingSchool() || (InAlleyScene() && quest.step >= QuestStep::Lumi);
        Point lumiPosition = EndingSchool() ? Point{player.x + 1, player.y}
                             : quest.step >= QuestStep::Report ? Point{5, 8} : Point{3, 7};
        add("NPC_LUMI", "루미", questDialogue && lumiPresent ? "대화 중" : "안내 대기", lumiPosition, lumiPresent);
        snapshot.npcs.back().z = EndingSchool() ? 30.f : 25 + std::sin(time * 3) * 5;
    }
    for (const auto& enemy : combat.enemies)
    {
        enemy.health > 0 ? ++snapshot.mobsAlive : ++snapshot.mobsDead;
    }
    return snapshot;
}

void Pause()
{
    confirmRestart = false;
    keys.fill(false);
    cameraKeys.fill(false);
    moving = false;
    paused = true;
}

void Key(unsigned char key, bool down)
{
    if (key >= 'A' && key <= 'Z')
    {
        key = key - 'A' + 'a';
    }
    const bool pressed = down && !keys[key];
    keys[key] = down;
    if (!pressed)
    {
        return;
    }
    if (paused && confirmRestart)
    {
        if (key == 27)
        {
            confirmRestart = false;
        }
        else if (key == 13)
        {
            Performance::Scope scope("cpu.storage.new_story_backup_ms");
            const auto path = RuntimeFiles::Path(L"main_quest_001.txt");
            const auto backup = path + L".backup_" + std::to_wstring(GetCurrentProcessId()) + L"_" +
                                std::to_wstring(GetTickCount64());
            bool success = false;
            if (!path.empty())
            {
                // No replacement flag: never overwrite a previous backup.
                success =
                    MoveFileExW(path.c_str(), backup.c_str(), MOVEFILE_WRITE_THROUGH) != FALSE;
                if (!success)
                {
                    success = GetLastError() == ERROR_FILE_NOT_FOUND;
                }
            }
            if (success)
            {
                Reset();
            }
            else
            {
                restartFailed = true;
            }
        }
        return;
    }
    if (paused && key == 'b')
    {
        confirmRestart = true;
        restartFailed = false;
        return;
    }
    if (key == 27)
    {
        paused = !paused;
        keys.fill(false);
        cameraKeys.fill(false);
        return;
    }
    if (key == 'n' && (paused || (quest.active && combat.Dead())))
    {
        if (quest.active)
        {
            EnterQuestStep(true);
            keys.fill(false);
            cameraKeys.fill(false);
            paused = false;
        }
        else
        {
            Reset();
        }
        return;
    }
    if (paused || (quest.active && (combat.Dead() || quest.step == QuestStep::Transform)) ||
        (!quest.active && story.step >= Step::AfterSchool))
    {
        return;
    }
    if (key == 'e')
    {
        Interact();
    }
    else if (quest.active && quest.step == QuestStep::Complete && !quest.epilogueComplete)
    {
        return;
    }
    else if (key == 'c')
    {
        cameraOffset = {0, 0};
    }
    else if (!story.dialogueOpen && !questDialogue && quest.active && key == 'z')
    {
        if (combat.UsePotion(quest))
        {
            saveSucceeded = quest.Save();
        }
    }
    else if (!story.dialogueOpen && !questDialogue && quest.CanFight() && !EndingSchool() &&
             key == 'r')
    {
        combat.actions.Reload();
    }
    else if (!story.dialogueOpen && !questDialogue && quest.CanFight() && !EndingSchool() &&
             key == '1')
    {
        combat.Burst(player, quest);
    }
    else if (key == ' ' && !story.dialogueOpen && !questDialogue && jumpHeight == 0)
    {
        jumpVelocity = 180;
        jumped = true;
    }
}

void CameraKey(int key, bool down)
{
    const int codes[] = {GLUT_KEY_LEFT, GLUT_KEY_RIGHT, GLUT_KEY_UP, GLUT_KEY_DOWN};
    for (int i = 0; i < 4; ++i)
    {
        if (key == codes[i])
        {
            cameraKeys[i] = down;
        }
    }
}

void Update(float dt)
{
    Performance::Scope scope("cpu.prologue.update_ms");
    if (paused)
    {
        return;
    }
    time += dt;
    story.Update(dt);
    UpdateQuest(dt);
    moving = false;
    if (!story.dialogueOpen && !questDialogue && !combat.Dead() &&
        !(quest.active && quest.step == QuestStep::Complete && !quest.epilogueComplete) &&
        (story.step < Step::AfterSchool || (quest.active && quest.step != QuestStep::Transform)))
    {
        const float horizontal = float(keys['d']) - float(keys['a']);
        const float vertical = float(keys['s']) - float(keys['w']);
        const float length = std::hypot(horizontal, vertical);
        if (length > 0)
        {
            // Screen-direction input converted into world axes, independently of the camera.
            const bool running = RunHeld();
            const float speed = (running ? 5.0f : 2.8f) * dt / (length * 1.41421356f);
            Point next = {player.x + (horizontal + vertical) * speed,
                          player.y + (vertical - horizontal) * speed};
            const Point before = player;
            if (Walkable({next.x, player.y}))
            {
                player.x = next.x;
            }
            if (Walkable({player.x, next.y}))
            {
                player.y = next.y;
            }
            moving = Distance(before, player) > .0001f;
            moved = moved || moving;
            ran = ran || (moving && running);
            if (horizontal != 0)
            {
                facing = horizontal < 0 ? -1.f : 1.f;
            }
            walkFrame += dt * (running ? 13 : 9);
        }
        const float cx = float(cameraKeys[1]) - float(cameraKeys[0]);
        const float cy = float(cameraKeys[3]) - float(cameraKeys[2]);
        cameraOffset.x = (std::max)(-3.f, (std::min)(3.f, cameraOffset.x + (cx + cy) * dt * 3));
        cameraOffset.y = (std::max)(-3.f, (std::min)(3.f, cameraOffset.y + (cy - cx) * dt * 3));
        looked = looked || cx != 0 || cy != 0;
        if (quest.CanFight() && !EndingSchool() && keys['q'])
        {
            combat.Attack(player, facing < 0, quest, jumpHeight);
        }
        if (story.step == Step::ReachSchool && Distance(player, yuna) < 3)
        {
            story.ReachSchool();
        }
    }
    if (jumpVelocity != 0 || jumpHeight > 0)
    {
        jumpVelocity -= 480 * dt;
        jumpHeight = (std::max)(0.f, jumpHeight + jumpVelocity * dt);
        if (jumpHeight == 0)
        {
            jumpVelocity = 0;
        }
    }
    const float follow = 1 - std::exp(-8 * dt);
    camera.x += (player.x + cameraOffset.x - camera.x) * follow;
    camera.y += (player.y + cameraOffset.y - camera.y) * follow;
    Performance::Profiler::Get().Gauge("prologue.story_step", static_cast<int>(story.step));
}

void Draw()
{
    Performance::Scope scope("cpu.prologue.draw_ms");
    scene.BeginSync();
    if (builtHome != story.IsHome())
    {
        BuildScene();
    }
    auto& actor = scene.Place("player", {player.x, player.y}, RenderLayer::World);
    PlaceModel("player/shadow", {}, ModelId::Shadow, RenderLayer::Shadow, &actor);
    const auto model =
        quest.CanFight() && !EndingSchool()
            ? static_cast<ModelId>(CharacterVisual::FrameIndex(combat.cast, moving, walkFrame))
            : static_cast<ModelId>(static_cast<int>(ModelId::StudentIdle) +
                                   (moving ? 1 + int(walkFrame) % 4 : 0));
    scene.Place(
        "player/body", {0, 0, jumpHeight}, RenderLayer::World,
        [model](const Actor&, const Transform& world)
        {
            Point p = Project(world);
            const float direction =
                quest.CanFight() && combat.cast > 0 ? (combat.castMirror ? -1.f : 1.f) : facing;
            graphics->DrawModel(models.Get(model), p.x, p.y, Scale() * direction, Scale());
        },
        &actor);
    if (quest.CanFight() && !EndingSchool())
    {
        auto& wand =
            scene.Place("player/wand",
                        CharacterVisual::WandSocket(
                            combat.cast > 0, combat.cast > 0 ? combat.castMirror : facing < 0, 40),
                        RenderLayer::World, {}, &actor);
        wand.local.z += jumpHeight;
        if (combat.cast > 0)
        {
            PlaceGlow("player/wand/flash", {}, 24, &wand);
        }
    }
    if (quest.active && quest.step == QuestStep::Transform)
    {
        PlaceGlow("player/transform", {0, 0, 40}, 100 + 70 * std::sin(transformTime * 3), &actor);
    }
    DrawQuestActors();
    scene.Place("sunset", {}, RenderLayer::UserInterface,
                [](const Actor&, const Transform&)
                {
                    if (story.step == Step::Complete && !EndingSchool())
                    {
                        Panel(0, 0, 1280, 800, {.86f, .35f, .16f, .17f});
                    }
                });
    scene.Place("ui", {}, RenderLayer::UserInterface,
                [](const Actor&, const Transform&)
                {
                    DrawHud();
                });
    scene.Place("diagnostics", {}, RenderLayer::UserInterface,
                [](const Actor&, const Transform&)
                {
                    graphics->DrawFrameStats();
                });
    scene.EndSync();
    graphics->BeginWorld();
    scene.Render(RenderLayer::Background, RenderLayer::Overlay,
                 [](const Transform& world)
                 {
                     Point p = Project(world);
                     const float margin = 300 * Scale();
                     return p.x > -margin && p.x < width + margin && p.y > -margin &&
                            p.y < height + margin;
                 });
    graphics->EndWorld(time);
    scene.Render(RenderLayer::UserInterface, RenderLayer::UserInterface);
    graphics->Flush();
}

void DrawQuestTwoPreview()
{
    Performance::Scope scope("cpu.quest_two.preview_ms");
    static Game::SceneGraph preview;
    if (!preview.Find("ui"))
    {
        auto& ui = preview.Place(
            "ui", {}, RenderLayer::UserInterface,
            [](const Actor&, const Transform&)
            {
                Panel(140, 200, 1000, 380, {.08f, .07f, .16f, .97f});
                Text(185, 265, "메인 퀘스트 2 · 학교에 숨어든 것", 30, pink);
                Text(185, 333, "#19 플레이 구간은 아직 준비 중입니다.", 24);
                Text(185, 382, "이 화면에서는 퀘스트 진행과 저장 데이터를 변경하지 않습니다.", 19);
                Text(185, 448, "F1 튜토리얼 / F2 사냥터 / F3 프롤로그·메인 퀘스트 1", 20);
                Text(185, 508, "처음부터: F3 메뉴에서 B → Enter", 22, pink);
            });
        preview.RetainSubtree(ui);
    }
    graphics->BeginWorld();
    graphics->EndWorld(0);
    preview.Render(RenderLayer::UserInterface, RenderLayer::UserInterface);
    graphics->Flush();
}
} // namespace Prologue
