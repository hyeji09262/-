#include "stdafx.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include "Prologue.h"
#include "PrologueStory.h"
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

struct Point
{
    float x, y;
};

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

float Distance(Point a, Point b)
{
    return std::hypot(a.x - b.x, a.y - b.y);
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
    const float xmax = story.IsHome() ? 7.6f : 23.6f;
    const float ymax = story.IsHome() ? 7.6f : 13.6f;
    if (p.x < .4f || p.y < .4f || p.x > xmax || p.y > ymax)
    {
        return false;
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
    else
    {
        // The road and school entrance remain connected. Buildings are not jump-through.
        if ((p.x > 17 && p.x < 22 && p.y < 4.4f) || Distance(p, yuna) < .55f)
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
    auto& root = scene.Place("environment", {}, RenderLayer::Ground);
    const int columns = builtHome ? 8 : 24, rows = builtHome ? 8 : 14;
    for (int y = 0; y < rows; ++y)
    {
        for (int x = 0; x < columns; ++x)
        {
            const bool road = y >= 7 && y <= 10;
            Color color = builtHome ? Color{.73f + (x % 2) * .035f, .60f, .50f}
                          : road    ? Color{.75f, .75f, .78f}
                                    : Color{.52f, .67f + (y % 2) * .025f, .57f};
            scene.Place(
                "floor/" + std::to_string(y * columns + x), {float(x), float(y)},
                RenderLayer::Ground,
                [color](const Actor&, const Transform& world)
                {
                    Floor(world, .98f, .98f, color);
                },
                &root);
        }
    }
    if (builtHome)
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
        auto& friendActor = PlaceModel("NPC_YUNA", {yuna.x, yuna.y}, ModelId::StudentIdle,
                                       RenderLayer::World, &root);
        PlaceModel("NPC_YUNA/shadow", {}, ModelId::Shadow, RenderLayer::Shadow, &friendActor);
        Label("NPC_YUNA/name", {0, 0, 83}, "유나 [E]", &friendActor);
        Label("road/sign", {8, 7, 28}, "학교 가는 길 →", &root);
    }
    scene.RetainSubtree(root);
}

void Reset()
{
    story = Story{};
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

void Interact()
{
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
    Text(42, 53, story.IsHome() ? "프롤로그 · 평범한 아침" : "프롤로그 · 학교의 소문", 25, pink);
    Text(42, 87, story.Objective(), 19);
    Text(42, 117, "평범한 학생 · 아직 마법이나 전투 능력이 없습니다", 15, {.70f, .77f, .86f});

    Panel(22, 640, 1236, 137, {.08f, .10f, .18f, .94f});
    Text(42, 670, "WASD 이동  |  R 누르며 달리기  |  스페이스 점프  |  E 상호작용", 19);
    Text(42, 702, "방향키 카메라 살펴보기 · C 복귀  |  ESC 일시정지", 18);
    Text(42, 736, "F1 기존 튜토리얼  /  F2 기존 사냥 레벨  /  F4 프롤로그", 17, pink);
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
    Point target = story.step == Step::News ? television : story.IsHome() ? door : yuna;
    if (!story.dialogueOpen && story.step < Step::AfterSchool)
    {
        const Point p = Project({target.x, target.y, 105});
        graphics->Nameplate(p.x, p.y, Distance(player, target) < 1.8f ? "[E] 상호작용" : "목표",
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
    if (story.step == Step::AfterSchool)
    {
        Panel(0, 0, 1280, 800, {.07f, .06f, .13f, .96f});
        Text(450, 373, "그리고, 방과 후", 36, pink);
        Text(350, 423, "종이 울리고 교실 밖으로 노을이 번집니다.", 23);
    }
    if (story.step == Step::Complete)
    {
        Panel(200, 425, 880, 170, {.12f, .08f, .16f, .96f});
        Text(230, 465, "학교 이벤트 완료 · 방과 후", 28, pink);
        Text(230, 509, "유나의 소문을 들었습니다. 이번 이야기는 여기까지입니다.", 21);
        Text(230, 554, "[N] 처음부터  /  F1·F2 기존 레벨  /  #6 퀘스트는 미구현", 18);
    }
    if (paused)
    {
        Panel(260, 245, 760, 220, {.06f, .07f, .13f, .98f});
        Text(310, 305, "프롤로그 일시정지", 30, pink);
        Text(310, 365, "ESC 계속하기  /  N 프롤로그 처음부터", 23);
        Text(310, 415, "기존 사냥 레벨의 성장 데이터는 변경하지 않습니다.", 18);
    }
}
} // namespace

void Initialize(Renderer* renderer)
{
    graphics = renderer;
    models.Initialize();
    Reset();
}

void Resize(int w, int h)
{
    width = (std::max)(1, w);
    height = (std::max)(1, h);
}

void Pause()
{
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
    if (key == 27)
    {
        paused = !paused;
        keys.fill(false);
        cameraKeys.fill(false);
        return;
    }
    if (key == 'n' && (paused || story.step == Step::Complete))
    {
        Reset();
        return;
    }
    if (paused || story.step >= Step::AfterSchool)
    {
        return;
    }
    if (key == 'e')
    {
        Interact();
    }
    else if (key == 'c')
    {
        cameraOffset = {0, 0};
    }
    else if (key == ' ' && !story.dialogueOpen && jumpHeight == 0)
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
    moving = false;
    if (!story.dialogueOpen && story.step < Step::AfterSchool)
    {
        const float horizontal = float(keys['d']) - float(keys['a']);
        const float vertical = float(keys['s']) - float(keys['w']);
        const float length = std::hypot(horizontal, vertical);
        if (length > 0)
        {
            // Screen-direction input converted into world axes, independently of the camera.
            const float speed = (keys['r'] ? 5.0f : 2.8f) * dt / (length * 1.41421356f);
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
            ran = ran || (moving && keys['r']);
            if (horizontal != 0)
            {
                facing = horizontal < 0 ? -1.f : 1.f;
            }
            walkFrame += dt * (keys['r'] ? 13 : 9);
        }
        const float cx = float(cameraKeys[1]) - float(cameraKeys[0]);
        const float cy = float(cameraKeys[3]) - float(cameraKeys[2]);
        cameraOffset.x = (std::max)(-3.f, (std::min)(3.f, cameraOffset.x + (cx + cy) * dt * 3));
        cameraOffset.y = (std::max)(-3.f, (std::min)(3.f, cameraOffset.y + (cy - cx) * dt * 3));
        looked = looked || cx != 0 || cy != 0;
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
    const auto model = static_cast<ModelId>(static_cast<int>(ModelId::StudentIdle) +
                                            (moving ? 1 + int(walkFrame) % 4 : 0));
    scene.Place(
        "player/body", {0, 0, jumpHeight}, RenderLayer::World,
        [model](const Actor&, const Transform& world)
        {
            Point p = Project(world);
            graphics->DrawModel(models.Get(model), p.x, p.y, Scale() * facing, Scale());
        },
        &actor);
    scene.Place("sunset", {}, RenderLayer::UserInterface,
                [](const Actor&, const Transform&)
                {
                    if (story.step == Step::Complete)
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
} // namespace Prologue
