#include "stdafx.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include "QuestTwo.h"
#include "QuestTwoStory.h"
#include "QuestCombat.h"
#include "PrimitiveModels.h"
#include "SceneGraph.h"
#include "RuntimeFiles.h"

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
bool available = false, loaded = false, paused = true, saveOk = true, loadError = false;
bool moving = false, mirror = false;
float clockTime = 0, walkFrame = 0;
std::string speaker, dialogue;
int pendingClue = -1;
bool pendingAdvance = false;
bool builtGym = false;

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
            graphics->DrawModel(models.Get(model), p.x, p.y,
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

void Glow(const std::string& id, Transform pose, float size, Actor* parent = nullptr)
{
    graph.Place(
        id, pose, RenderLayer::World,
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
    if (p.x < .5f || p.x > 23 || p.y < 2.5f || p.y > 12.5f)
    {
        return false;
    }
    // The optional storage area is never on the main quest route.
    return !(story.Gym() && !story.sealOpened && p.x > 19 && p.y < 6);
}

bool Save()
{
    Performance::Scope scope("cpu.storage.quest_two_save_ms");
    const auto path = RuntimeFiles::Path(L"main_quest_002.txt");
    if (path.empty())
    {
        return false;
    }
    const auto temporary = path + L".tmp";
    {
        std::ofstream file(temporary.c_str(), std::ios::trunc);
        file << "MAGICAL_QUEST_002_V1\n"
             << int(story.step) << ' ' << story.clues << ' ' << story.fragments << ' '
             << story.sealOpened << ' ' << story.treasureTaken << '\n'
             << story.progress.totalExperience << ' ' << story.progress.coins << ' '
             << story.progress.crystals << ' ' << story.progress.potions << ' '
             << story.progress.manaPotions << '\n';
        file.flush();
        if (!file)
        {
            return false;
        }
    }
    return RuntimeFiles::Commit(temporary, path);
}

bool Load()
{
    Performance::Scope scope("cpu.storage.quest_two_load_ms");
    const auto path = RuntimeFiles::Path(L"main_quest_002.txt");
    if (path.empty())
    {
        loadError = true;
        return false;
    }
    std::ifstream file(path.c_str());
    if (!file)
    {
        loadError = GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
        return false;
    }
    StoryState candidate;
    std::string magic;
    int step = 0, seal = 0, treasure = 0;
    auto& p = candidate.progress;
    if (!(file >> magic >> step >> candidate.clues >> candidate.fragments >> seal >> treasure >>
          p.totalExperience >> p.coins >> p.crystals >> p.potions >> p.manaPotions) ||
        magic != "MAGICAL_QUEST_002_V1" || step < 0 || step > int(Step::Complete) ||
        candidate.clues > 7 || (step >= int(Step::Shadows) && candidate.clues != 7) ||
        (step == int(Step::Reaction) && candidate.clues != 0) ||
        (step == int(Step::Investigate) && candidate.clues == 7) ||
        candidate.fragments != (step >= int(Step::Report) ? 5 : 4) || seal < 0 || seal > 1 ||
        treasure < 0 || treasure > 1 || (treasure && !seal) ||
        (seal && step != int(Step::Complete)) || p.totalExperience < 0 ||
        p.totalExperience > Rpg::ExperienceLimit || p.coins < 0 || p.coins > 1000000 ||
        p.crystals < 0 || p.crystals > 1000000 || p.potions < 0 || p.potions > 99 ||
        p.manaPotions < 0 || p.manaPotions > 99)
    {
        loadError = true;
        return false;
    }
    candidate.step = static_cast<Step>(step);
    candidate.sealOpened = seal != 0;
    candidate.treasureTaken = treasure != 0;
    p.Recalculate();
    story = candidate;
    return true;
}

void BuildScene()
{
    Performance::Scope scope("cpu.quest_two.static_scene_build_ms");
    graph.Clear();
    builtGym = story.Gym();
    auto& root = graph.Place("school", {}, RenderLayer::Ground);
    for (int y = 0; y < 14; ++y)
    {
        for (int x = 0; x < 24; ++x)
        {
            Color floor = builtGym ? Color{.61f, .42f, .31f} : Color{.65f, .69f, .76f};
            graph.Place(
                "floor/" + std::to_string(y * 24 + x), {float(x), float(y)}, RenderLayer::Ground,
                [floor](const Actor&, const Transform& p)
                {
                    Floor(p, 1.005f, 1.005f, floor);
                },
                &root);
        }
    }
    for (int i = 0; i < 6; ++i)
    {
        graph.Place(
            "wall/" + std::to_string(i), {i * 4.f, 1}, RenderLayer::World,
            [](const Actor&, const Transform& p)
            {
                Box(p, 3.9f, .3f, 95, {.70f, .75f, .84f});
                Box({p.x + .6f, p.y + .31f, 40}, 2.4f, .05f, 40, {.36f, .59f, .76f});
            },
            &root);
    }
    Label("school/name", {12, 1, 115}, builtGym ? "별빛 학교 · 체육관" : "별빛 학교 · 복도", gold,
          &root);
    if (!builtGym)
    {
        for (int i = 0; i < 3; ++i)
        {
            const auto site = Investigations()[i];
            graph.Place(
                "station/" + std::to_string(i), {site.x, 1.5f}, RenderLayer::World,
                [i](const Actor&, const Transform& p)
                {
                    Box(p, 1.5f, .6f, 45 + i * 10.f,
                        i == 0 ? Color{.40f, .55f, .72f} : Color{.61f, .45f, .39f});
                },
                &root);
            Label("station/name/" + std::to_string(i), {site.x, 2, 85}, site.name, paper, &root);
        }
        auto& yuna = Model("yuna", {3, 11}, ModelId::Yuna, &root);
        Label("yuna/name", {0, 0, 80}, "유나 · 수업 대기", paper, &yuna);
        auto& jisoo = Model("jisoo", {7, 11}, ModelId::Jisoo, &root);
        Label("jisoo/name", {0, 0, 80}, "지수 · 수업 대기", paper, &jisoo);
        Label("gym/door", {21, 8, 70}, "체육관 입구 [E]", gold, &root);
    }
    else
    {
        graph.Place(
            "court", {7, 5, 1}, RenderLayer::Ground,
            [](const Actor&, const Transform& p)
            {
                Floor(p, 10, .06f, paper);
                Floor({p.x, p.y + 6, 1}, 10, .06f, paper);
                Floor(p, .06f, 6, paper);
                Floor({p.x + 10, p.y, 1}, .06f, 6, paper);
                Floor({p.x + 5, p.y, 1}, .06f, 6, paper);
            },
            &root);
        Label("storage/name", {21, 3, 75}, "봉인된 체육 창고", blue, &root);
    }
    graph.RetainSubtree(root);
}

void EnterStep(bool retry = false)
{
    Performance::Scope scope("cpu.quest_two.step_enter_ms");
    encounter = MainQuest::State{};
    encounter.active = true;
    encounter.progress = story.progress;
    encounter.step = story.step == Step::Shadows ? MainQuest::Step::Shadows
                     : story.step == Step::Elite ? MainQuest::Step::Boss
                                                 : MainQuest::Step::Complete;
    combat.Enter(encounter);
    if (story.step == Step::Elite && !combat.enemies.empty())
    {
        combat.enemies.front().health = 220;
    }
    if (retry || builtGym != story.Gym())
    {
        player = {3, 9};
        camera = player;
    }
    if (graph.Find("school") == nullptr || builtGym != story.Gym())
    {
        BuildScene();
    }
    keys.fill(false);
    moving = false;
    saveOk = Save();
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
            saveOk = Save();
        }
        return;
    }
    if (combat.actions.height > 0)
    {
        return;
    }
    if (story.step == Step::Investigate)
    {
        for (unsigned i = 0; i < 3; ++i)
        {
            const auto& site = Investigations()[i];
            if (!(story.clues & (1u << i)) && Distance(player, {site.x, site.y}) < 1.8f)
            {
                pendingClue = int(i);
                Talk(site.name, site.text);
                return;
            }
        }
    }
    const auto& goal = story.Current();
    if (story.step != Step::Complete && story.step != Step::Investigate &&
        goal.event == MainQuest::Event::Interact && Distance(player, {goal.x, goal.y}) < 1.8f)
    {
        pendingAdvance = true;
        Talk(goal.speaker, goal.dialogue);
        return;
    }
    if (story.step == Step::Complete && Distance(player, {21, 4}) < 1.8f && story.TakeTreasure())
    {
        saveOk = Save();
        Talk(
            "창고의 작은 보상",
            "정화 마법으로 열린 길! 마력석 2개와 마나 물약 2개를 얻었다. 다음 목적지는 미아의 가게다.");
        return;
    }
    // Repeatable, capped supplies prevent an empty inventory from blocking the story.
    if (Distance(player, {2, 9}) < 1.5f)
    {
        story.progress.potions = (std::max)(3, story.progress.potions);
        story.progress.manaPotions = (std::max)(3, story.progress.manaPotions);
        encounter.progress = story.progress;
        saveOk = Save();
        Talk(
            "루미의 보급함",
            "체력 물약과 마나 물약을 각각 최소 3개까지 보충했어. Z는 체력, X는 마나! 보급함은 다시 이용할 수 있어.");
        return;
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

void Meter(float x, const char* name, float value, float maximum, Color color, char key, int count)
{
    float ratio = (std::max)(0.f, (std::min)(1.f, value / (std::max)(1.f, maximum)));
    Text(x - 7, 231, name, 18);
    Panel(x, 243, 22, 280, {.2f, .22f, .3f});
    Panel(x, 243 + 280 * (1 - ratio), 22, 280 * ratio, color);
    Text(x - 10, 542, std::to_string(int(value)) + "/" + std::to_string(int(maximum)), 14);
    Text(x - 14, 560, std::string("[") + key + "] " + std::to_string(count) + "개", 15);
}

void DrawHud()
{
    Performance::Scope scope("cpu.quest_two.hud_ms");
    Panel(22, 20, 1000, 150);
    Text(42, 52, "메인 퀘스트 2 · 학교에 숨어든 것", 25, pink);
    if (!available)
    {
        Text(42, 90, "F3에서 퀘스트 1과 다음 날 교실의 마지막 대화를 완료하세요.", 20, gold);
        return;
    }
    if (loadError)
    {
        Text(42, 90, "퀘스트 2 저장 파일을 읽지 못했습니다. 기존 파일을 덮어쓰지 않았습니다.", 18,
             gold);
        return;
    }
    Text(42, 88, story.Current().text, 19, gold);
    std::string detail = "조사 " + std::to_string(story.ClueCount()) + "/3 · 검은 조각 " +
                         std::to_string(story.fragments) + " · 마력석 " +
                         std::to_string(story.progress.crystals);
    if (story.Fighting())
    {
        int alive = 0;
        for (const auto& enemy : combat.enemies)
        {
            alive += enemy.health > 0 ? 1 : 0;
        }
        detail += " · 남은 적 " + std::to_string(alive);
    }
    Text(42, 120, detail, 17);
    Text(42, 150,
         story.step == Step::Complete ? "다음 퀘스트: 미아를 찾아서 (#20은 아직 미구현)"
                                      : "[E] 조사/대화 · 물약이 부족하면 입구의 루미 보급함 이용",
         16, blue);
    Panel(22, 212, 152, 354);
    Meter(48, "체력", combat.health, float(story.progress.stats.maxHealth), pink, 'Z',
          story.progress.potions);
    Meter(112, "마나", combat.mana, float(story.progress.stats.maxMana), blue, 'X',
          story.progress.manaPotions);
    Panel(22, 710, 1236, 76);
    Text(42, 740,
         "WASD 이동 · Shift 달리기 · Space 점프 · Q 공격 · 1 별빛 폭발 · Z/X 물약 · E 조사", 17);
    Text(42, 770,
         "ESC 메뉴 · N 전투 재시도 · F1 튜토리얼 · F2 사냥터 · F3 이전 이야기 · F4 퀘스트2", 16,
         pink);
    if (!paused && !combat.Dead() && dialogue.empty() && !story.Fighting() &&
        !(story.step == Step::Complete && story.treasureTaken))
    {
        const auto& objective = story.Current();
        Point target{objective.x, objective.y};
        if (story.step == Step::Investigate)
        {
            float nearest = 1000;
            for (unsigned i = 0; i < 3; ++i)
            {
                const auto& site = Investigations()[i];
                const float distance = Distance(player, {site.x, site.y});
                if (!(story.clues & (1u << i)) && distance < nearest)
                {
                    nearest = distance;
                    target = {site.x, site.y};
                }
            }
        }
        if (story.step == Step::Complete)
        {
            target = story.sealOpened ? Point{21, 4} : Point{19, 5};
        }
        Point p = Project({target.x, target.y, 85});
        const Point low = Screen(240, 205), high = Screen(1200, 660);
        p.x = (std::max)(low.x, (std::min)(high.x, p.x));
        p.y = (std::max)(low.y, (std::min)(high.y, p.y));
        graphics->Nameplate(p.x, p.y,
                            story.step == Step::Complete && !story.sealOpened ? "목표 · [2] 정화"
                                                                              : "목표 · [E] 조사",
                            18 * Scale(), gold.r, gold.g, gold.b);
    }
    if (!saveOk)
    {
        Text(250, 688, "저장 실패: 현재 진행은 유지되지만 재실행 시 복원되지 않을 수 있습니다.", 17,
             gold);
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
    if (combat.Dead() || paused)
    {
        Panel(300, 270, 700, 160);
        Text(330, 318, combat.Dead() ? "쓰러졌습니다" : "메인 퀘스트 2 · 일시정지", 27, pink);
        Text(330, 363, "[N] 현재 단계 재시도 · 전투 처치 수 초기화", 21);
        Text(330, 401, "[ESC] 계속하기 · 보급함에서 물약을 다시 받을 수 있습니다", 17);
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
    loadError = false;
    if (!Load())
    {
        if (loadError)
        {
            return;
        }
        story = StoryState{};
        story.progress = inherited;
        story.fragments = fragments;
    }
    loaded = true;
    EnterStep(true);
}

void Pause()
{
    paused = true;
    moving = false;
    keys.fill(false);
}

void Key(unsigned char key, bool down)
{
    if (key >= 'A' && key <= 'Z')
    {
        key += 'a' - 'A';
    }
    bool fresh = down && !keys[key];
    keys[key] = down;
    if (!fresh || !available || !loaded || loadError)
    {
        return;
    }
    if (key == 27)
    {
        paused = !paused;
        keys.fill(false);
        return;
    }
    if (key == 'n' && (paused || combat.Dead()))
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
    if (key == ' ')
    {
        combat.actions.Jump();
    }
    if (key == '1' && story.Fighting())
    {
        combat.Burst(player, encounter);
    }
    if (key == '2' && Distance(player, {19, 5}) < 3 && story.OpenSeal())
    {
        saveOk = Save();
        Performance::Profiler::Get().Count("quest_two.seals_opened");
        Talk("별빛 정화", "봉인이 사라졌다! 창고 안의 빛나는 마력석을 E로 조사하자.");
    }
    if (key == 'z' && combat.UsePotion(encounter))
    {
        story.progress = encounter.progress;
        saveOk = Save();
    }
    if (key == 'x' && combat.actions.UseManaPotion(combat.mana, float(story.progress.stats.maxMana),
                                                   story.progress.manaPotions))
    {
        encounter.progress = story.progress;
        saveOk = Save();
    }
}

void Update(float dt)
{
    Performance::Scope scope("cpu.quest_two.update_ms");
    if (!available || !loaded || loadError || paused || !dialogue.empty() || combat.Dead())
    {
        return;
    }
    clockTime += dt;
    float horizontal = float(keys['d']) - float(keys['a']);
    float vertical = float(keys['s']) - float(keys['w']);
    float length = std::hypot(horizontal, vertical);
    moving = false;
    if (length > 0)
    {
        bool running =
            GetActiveWindow() == GetForegroundWindow() && (GetAsyncKeyState(VK_SHIFT) & 0x8000);
        Point direction{(horizontal + vertical) / (length * 1.41421356f),
                        (vertical - horizontal) / (length * 1.41421356f)};
        Point before = player;
        MainQuest::Combat::Move(player, direction, (running ? 5.f : 2.8f) * dt, Walkable);
        moving = Distance(before, player) > .0001f;
        if (horizontal != 0)
        {
            mirror = horizontal < 0;
        }
        walkFrame += dt * (running ? 13 : 9);
    }
    combat.Update(dt, player, encounter, Walkable);
    if (story.Fighting() && !combat.Dead())
    {
        bool alive = false;
        for (const auto& enemy : combat.enemies)
        {
            alive = alive || enemy.health > 0;
        }
        if (!alive)
        {
            story.Advance(MainQuest::Event::Defeat);
            EnterStep();
        }
        else if (keys['q'])
        {
            combat.Attack(player, mirror, encounter, combat.actions.height);
        }
    }
    float follow = 1 - std::exp(-8 * dt);
    camera.x += (player.x - camera.x) * follow;
    camera.y += (player.y - camera.y) * follow;
    Performance::Profiler::Get().Gauge("quest_two.step_index", int(story.step));
}

void Draw()
{
    Performance::Scope scope("cpu.quest_two.draw_ms");
    graph.BeginSync();
    if (available && loaded && !loadError)
    {
        auto& hero = graph.Place("hero", {player.x, player.y}, RenderLayer::World);
        Model("hero/shadow", {}, ModelId::Shadow, &hero, false, RenderLayer::Shadow);
        bool magical = story.step >= Step::Shadows;
        ModelId model =
            magical
                ? static_cast<ModelId>(CharacterVisual::FrameIndex(combat.cast, moving, walkFrame))
                : static_cast<ModelId>(int(ModelId::StudentIdle) +
                                       (moving ? 1 + int(walkFrame) % 4 : 0));
        bool castMirror = combat.cast > 0 ? combat.castMirror : mirror;
        Model("hero/body", {0, 0, combat.actions.height}, model, &hero, castMirror);
        if (magical && combat.cast > 0)
        {
            Transform socket = CharacterVisual::WandSocket(true, castMirror, 40);
            socket.z += combat.actions.height;
            Glow("hero/wand", socket, 24, &hero);
        }
        auto& lumi = Model("lumi", {3, 8, 28 + 4 * std::sin(clockTime * 3)}, ModelId::Lumi);
        Label("lumi/name", {0, 0, 60}, "루미", paper, &lumi);
        Model("supplies", {2, 9}, ModelId::Potion);
        Label("supplies/name", {2, 9, 50}, "루미 보급함 [E]", blue);
        if (story.step == Step::Investigate)
        {
            for (unsigned i = 0; i < 3; ++i)
            {
                if (story.clues & (1u << i))
                {
                    continue;
                }
                const auto& site = Investigations()[i];
                Glow("clue/" + std::to_string(i), {site.x, site.y, 22}, 45);
                Label("clue/name/" + std::to_string(i), {site.x, site.y, 60},
                      std::string(site.name) + " [E]", gold);
            }
        }
        if (story.step == Step::Fragment)
        {
            Model("fragment", {15, 8, 15}, ModelId::BlackFragment);
            Glow("fragment/glow", {15, 8, 30}, 50);
        }
        if (story.Gym() && !story.sealOpened)
        {
            graph.Place("storage/seal", {19, 3}, RenderLayer::World,
                        [](const Actor&, const Transform& p)
                        {
                            Box(p, .1f, 3, 70, {.55f, .35f, .90f, .65f});
                            Box({19, 6}, 4, .1f, 70, {.55f, .35f, .90f, .65f});
                        });
            Label("storage/hint", {19, 5, 85},
                  story.step == Step::Complete ? "[2] 별빛 정화" : "강한 봉인 · 아직 해제 불가",
                  gold);
        }
        if (story.Gym() && !story.treasureTaken)
        {
            Model("treasure", {21, 4, 15}, ModelId::Crystal);
            Label("treasure/name", {21, 4, 55}, "숨겨진 보상 [E]", gold);
        }
        for (size_t i = 0; i < combat.enemies.size(); ++i)
        {
            const auto enemy = combat.enemies[i];
            if (enemy.health <= 0)
            {
                continue;
            }
            std::string id = "enemy/" + std::to_string(i);
            auto& actor =
                Model(id, {enemy.position.x, enemy.position.y, 0, enemy.boss ? 1.3f : 1.f},
                      enemy.boss ? ModelId::ShadowEater : ModelId::Shade0);
            Model(id + "/shadow", {}, ModelId::Shadow, &actor, false, RenderLayer::Shadow);
            Label(id + "/hp", {0, 0, enemy.boss ? 140.f : 75.f},
                  std::string(enemy.boss ? "강화 그림자 " : "그림자 ") +
                      std::to_string(int(enemy.health)),
                  pink, &actor);
            if (enemy.boss && enemy.phase == 1)
            {
                graph.Place(
                    id + "/warning", {}, RenderLayer::Shadow,
                    [enemy](const Actor&, const Transform&)
                    {
                        Point target = enemy.pattern == 0 ? enemy.position : enemy.aim;
                        const float radius = enemy.pattern == 0 ? 2.4f : 2.f;
                        for (int j = 0; j < 32; ++j)
                        {
                            float a = j * 6.2831853f / 32, b = (j + 1) * 6.2831853f / 32;
                            Triangle(Project({target.x, target.y, 2}),
                                     Project({target.x + std::cos(a) * radius,
                                              target.y + std::sin(a) * radius, 2}),
                                     Project({target.x + std::cos(b) * radius,
                                              target.y + std::sin(b) * radius, 2}),
                                     {.95f, .12f, .22f, .45f});
                        }
                    },
                    &actor);
            }
        }
        for (const auto& shot : combat.shots)
        {
            auto pose = CharacterVisual::SpellPose(shot.position.x, shot.position.y, shot.age,
                                                   shot.mirror, 40);
            pose.z += shot.launchHeight * (1 - (std::min)(1.f, shot.age / .18f));
            Glow("shot/" + std::to_string(shot.id), pose, CharacterVisual::SpellSize);
        }
        if (combat.burst > 0)
        {
            Glow("burst", {combat.burstOrigin.x, combat.burstOrigin.y, 25}, 150);
        }
    }
    graph.Place("hud", {}, RenderLayer::UserInterface,
                [](const Actor&, const Transform&)
                {
                    DrawHud();
                });
    graph.EndSync();
    graphics->BeginWorld();
    if (available && loaded && !loadError)
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
    snapshot.paused = paused || !available || loadError || !dialogue.empty();
    snapshot.objective = !available  ? "퀘스트 1 완료 대기"
                         : loadError ? "저장 파일 오류"
                                     : story.Current().text;
    if (!loaded || !available || loadError)
    {
        return snapshot;
    }
    snapshot.npcs.push_back({"NPC_LUMI", "루미",
                             !dialogue.empty() && speaker == "루미" ? "대화 중" : "안내 대기", 3, 8,
                             28, true});
    snapshot.npcs.push_back({"NPC_YUNA", "유나",
                             !dialogue.empty() && speaker == "유나" ? "대화 중" : "수업 대기", 3,
                             11, 0, !story.Gym()});
    snapshot.npcs.push_back({"NPC_JISOO", "지수",
                             !dialogue.empty() && speaker == "지수" ? "대화 중" : "수업 대기", 7,
                             11, 0, !story.Gym()});
    for (const auto& enemy : combat.enemies)
    {
        enemy.health > 0 ? ++snapshot.mobsAlive : ++snapshot.mobsDead;
    }
    snapshot.towns.push_back(
        {"school",
         story.Gym() ? "학교 체육관" : "학교 복도",
         story.step == Step::Complete ? "사건 해결 · 일상 회복" : "학교 안에서 이상 현상 발생",
         {"사물함, 도서관, 방송실의 흔적이 체육관으로 이어진다."}});
    return snapshot;
}
} // namespace QuestTwo
