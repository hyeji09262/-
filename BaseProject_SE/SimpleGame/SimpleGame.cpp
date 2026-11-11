/*
Copyright 2022 Lee Taek Hee (Tech University of Korea)
This program is free software: you can redistribute it and/or modify
it under the terms of the What The Hell License. Do it plz.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY.
*/
#include "stdafx.h"
#define NOMINMAX
#include <windows.h>
#include <iostream>
#include "Renderer.h"
#include "GameInput.h"
#include "Tutorial.h"
#include "LevelOne.h"
#include "Profiler.h"
#include "RuntimeFiles.h"
#include "Prologue.h"
#include "QuestTwo.h"
#include "ParkQuest.h"

static Renderer* g_Renderer = nullptr;
static GLFWwindow* window = nullptr;
static double previousTime = 0;
enum class GameScene
{
    Prologue,
    Hunting,
    QuestTwo,
    Park,
    World
};

static GameScene activeScene = GameScene::Prologue;
static GameScene recoveryScene = GameScene::World;
static Rpg::Progress huntingRecoveryShop;
static bool huntingFromVillage = false;

void LeavePark()
{
    ParkQuest::Pause();
    const bool rescued = ParkQuest::RescuedDog() &&
                         Tutorial::miaQuest == Tutorial::MiaQuest::ParkIncident;
    QuestTwo::StoreParkProgress(ParkQuest::Progress(), rescued);
    if (rescued)
    {
        Tutorial::miaQuest = Tutorial::MiaQuest::ReportPark;
    }
}

void OpenWorld()
{
    Prologue::Pause();
    QuestTwo::Pause();
    LevelOne::Pause();
    Tutorial::SetExploration(true, Prologue::TransformationUnlocked());
    Rpg::Progress progress;
    int fragments = 0;
    if (Prologue::QuestTwoStart(progress, fragments))
    {
        QuestTwo::Open(true, progress, fragments);
        QuestTwo::Pause();
    }
    Tutorial::shopOpen = false;
    Rpg::Progress inventory;
    if ((activeScene != GameScene::World || recoveryScene == GameScene::World) &&
        QuestTwo::ShopProgress(inventory))
    {
        Tutorial::potions = inventory.potions;
        Tutorial::manaPotions = inventory.manaPotions;
    }
    activeScene = GameScene::World;
    glfwSetWindowTitle(window, "마법소녀 - 별빛 마을");
}

void OpenRecoveryWorld(GameScene source, const Rpg::Progress& progress)
{
    const int healthPotions = progress.potions, manaPotions = progress.manaPotions;
    OpenWorld();
    Tutorial::LeaveBuilding();
    const auto& shop = CoastalWorld::Buildings()[2];
    Tutorial::player = {CoastalWorld::DoorX(shop), CoastalWorld::DoorY(shop)};
    Tutorial::hp = (std::max)(1, Tutorial::hp);
    Tutorial::actions.height = Tutorial::actions.velocity = 0;
    Tutorial::spells.clear();
    Tutorial::particles.clear();
    std::fill(Tutorial::keys, Tutorial::keys + 256, false);
    Tutorial::potions = healthPotions;
    Tutorial::manaPotions = manaPotions;
    recoveryScene = source;
    Tutorial::recoveryShop = nullptr;
    if (source == GameScene::Hunting)
    {
        huntingRecoveryShop = progress;
        Tutorial::recoveryShop = &huntingRecoveryShop;
    }
    const char* next = source == GameScene::Prologue ? "정비 후 [F3] 골목 전투"
                       : source == GameScene::QuestTwo ? "정비 후 [F4] 학교 전투"
                       : source == GameScene::Park ? "정비 후 중앙공원 입구 [E]"
                                                  : "정비 후 [F2] 사냥터";
    Tutorial::Say(std::string("별빛 편의점 입장 [E] · ") + next);
}

void ApplyRestock(GameScene target)
{
    if (recoveryScene != target)
    {
        return;
    }
    switch (target)
    {
    case GameScene::Prologue:
        Prologue::Restock(Tutorial::potions, Tutorial::manaPotions);
        break;
    case GameScene::QuestTwo:
        QuestTwo::Restock(Tutorial::potions, Tutorial::manaPotions);
        break;
    case GameScene::Park:
        ParkQuest::Restock(Tutorial::potions, Tutorial::manaPotions);
        break;
    case GameScene::Hunting:
        LevelOne::Restock(huntingRecoveryShop);
        break;
    case GameScene::World:
        break;
    }
    recoveryScene = GameScene::World;
    Tutorial::recoveryShop = nullptr;
}

void OpenQuestTwo()
{
    Rpg::Progress inherited;
    int fragments = 4;
    const bool unlocked = Prologue::QuestTwoStart(inherited, fragments);
    Prologue::Pause();
    QuestTwo::Open(unlocked, inherited, fragments);
    if (unlocked)
    {
        ApplyRestock(GameScene::QuestTwo);
    }
    activeScene = GameScene::QuestTwo;
    glfwSetWindowTitle(window, "마법소녀 - 메인 퀘스트 2: 학교에 숨어든 것");
}

void RenderScene()
{
    static auto previousFrame = Performance::Clock::now();
    auto start = Performance::Clock::now();
    double interval = Performance::Milliseconds(previousFrame, start);
    previousFrame = start;
    g_Renderer->BeginFrame();
    if (activeScene == GameScene::Prologue)
    {
        Prologue::Draw();
    }
    else if (activeScene == GameScene::World)
    {
        Tutorial::Draw();
    }
    else if (activeScene == GameScene::Hunting)
    {
        LevelOne::Draw();
    }
    else if (activeScene == GameScene::Park)
    {
        ParkQuest::Draw();
    }
    else
    {
        QuestTwo::Draw();
    }
    g_Renderer->EndFrame();
    {
        Performance::Scope scope("cpu.present.swap_wait_ms");
        glfwSwapBuffers(window);
    }
    auto& profiler = Performance::Profiler::Get();
    profiler.Sample("cpu.frame_callback_ms", Performance::Milliseconds(start));
    const char* sceneName = activeScene == GameScene::Prologue  ? "prologue"
                            : activeScene == GameScene::World   ? "world"
                            : activeScene == GameScene::Hunting ? "level_one"
                            : activeScene == GameScene::Park    ? "park"
                                                                : "quest_two";
    profiler.Count(activeScene == GameScene::Prologue  ? "frame.scene.prologue"
                   : activeScene == GameScene::World   ? "frame.scene.world"
                   : activeScene == GameScene::Hunting ? "frame.scene.level_one"
                   : activeScene == GameScene::Park    ? "frame.scene.park"
                                                       : "frame.scene.quest_two");
    profiler.Frame(interval, sceneName, g_Renderer->LastFrameStats().frame);
}

void Tick(double now)
{
    Performance::Scope scope("cpu.gameplay.tick_callback_ms");
    const double elapsed = now - previousTime;
    Performance::Profiler::Get().Sample("simulation.tick_interval_ms", elapsed * 1000);
    if (elapsed > .05)
    {
        Performance::Profiler::Get().Count("simulation.delta_time_clamped");
    }
    const float dt = static_cast<float>((std::min)(elapsed, .05));
    previousTime = now;

    if (activeScene == GameScene::Prologue)
    {
        Prologue::Update(dt);
        if (Prologue::ConsumeWorldRequest())
        {
            if (Prologue::Defeated())
            {
                OpenRecoveryWorld(GameScene::Prologue, Prologue::Progress());
            }
            else
            {
                OpenWorld();
                Tutorial::LeaveBuilding();
                const auto& home = CoastalWorld::Buildings()[0];
                Tutorial::player = {CoastalWorld::DoorX(home), CoastalWorld::DoorY(home)};
                Tutorial::Say("학교에 가는 길입니다. 별빛 학교의 노란 출입구에서 E를 눌러 들어가세요.");
            }
        }
        if (Prologue::ConsumeQuestTwoRequest())
        {
            OpenQuestTwo();
        }
    }
    else if (activeScene == GameScene::World)
    {
        Tutorial::storyMagicUnlocked = Prologue::TransformationUnlocked();
        Tutorial::Update(dt);
        const int request = Tutorial::sceneRequest;
        Tutorial::sceneRequest = 0;
        if (request != 0)
        {
            Tutorial::paused = true;
            std::fill(Tutorial::keys, Tutorial::keys + 256, false);
            if (request == 1)
            {
                float x, y;
                int person;
                std::string objective;
                Rpg::Progress progress;
                int fragments = 0;
                if (Prologue::WorldObjective(x, y, person, objective))
                {
                    Tutorial::LeaveBuilding();
                    OpenWorld();
                    Tutorial::Say(objective);
                }
                else if (Tutorial::interior > 0 && Prologue::QuestTwoStart(progress, fragments))
                {
                    OpenQuestTwo();
                }
                else
                {
                    ApplyRestock(GameScene::Prologue);
                    Prologue::ResumeFromWorld();
                    activeScene = GameScene::Prologue;
                    glfwSetWindowTitle(window, "마법소녀 - 메인 스토리");
                }
            }
            else if (request == 3)
            {
                const int room = Tutorial::interiorRoom;
                const auto position = Tutorial::player;
                OpenQuestTwo();
                QuestTwo::EnterFromSchool(room, position.x, position.y);
            }
            else if (request == Tutorial::ParkSceneRequest)
            {
                Rpg::Progress progress = ParkQuest::Progress();
                QuestTwo::ShopProgress(progress);
                ParkQuest::Open(*g_Renderer, Tutorial::VisualModels(), progress);
                ApplyRestock(GameScene::Park);
                activeScene = GameScene::Park;
                glfwSetWindowTitle(window, "마법소녀 - 별빛 중앙공원");
            }
            else if (request == Tutorial::HuntingSceneRequest)
            {
                huntingFromVillage = true;
                LevelOne::Resume(true);
                ApplyRestock(GameScene::Hunting);
                activeScene = GameScene::Hunting;
                glfwSetWindowTitle(window, "마법소녀 - 별빛 숲 사냥터");
            }
            else
            {
                OpenQuestTwo();
            }
        }
    }
    else if (activeScene == GameScene::Hunting)
    {
        LevelOne::Update(dt);
        if (LevelOne::ConsumeWorldRequest())
        {
            if (LevelOne::Defeated())
            {
                OpenRecoveryWorld(GameScene::Hunting, LevelOne::Progress());
            }
            else
            {
                OpenWorld();
                Tutorial::LeaveBuilding();
                if (huntingFromVillage)
                {
                    Tutorial::player = Tutorial::HuntingEntrance;
                }
                Tutorial::Say("마을로 돌아왔습니다.");
            }
        }
    }
    else if (activeScene == GameScene::Park)
    {
        ParkQuest::Update(dt);
        if (ParkQuest::ConsumeExit())
        {
            LeavePark();
            if (ParkQuest::Defeated())
            {
                OpenRecoveryWorld(GameScene::Park, ParkQuest::Progress());
            }
            else
            {
                OpenWorld();
                const auto& park = CoastalWorld::Buildings()[Tutorial::StoryPark];
                Tutorial::player = {CoastalWorld::DoorX(park), CoastalWorld::DoorY(park)};
                Tutorial::Say(Tutorial::MiaQuestObjective());
            }
        }
    }
    else if (activeScene == GameScene::QuestTwo)
    {
        QuestTwo::Update(dt);
        if (QuestTwo::ConsumeWorldRequest())
        {
            if (QuestTwo::Defeated())
            {
                OpenRecoveryWorld(GameScene::QuestTwo, QuestTwo::Progress());
            }
            else
            {
                OpenWorld();
                Tutorial::LeaveBuilding();
                const auto& school = CoastalWorld::Buildings()[1];
                Tutorial::player = {CoastalWorld::DoorX(school), CoastalWorld::DoorY(school) + 1};
                Tutorial::Say(
                    QuestTwo::Finished()
                        ? "루미: 검은 조각을 아는 사람이 있어. 별빛 쇼핑몰 안내 데스크의 미아를 만나 보자!"
                        : "학교에서 나왔습니다. 노란 목표를 따라 조사를 이어갈 수 있습니다.");
            }
        }
    }

    if (WorldMonitor::Due())
    {
        Performance::Scope monitorScope("cpu.monitor.collect_ms");
        const char* active = activeScene == GameScene::World      ? "world"
                             : activeScene == GameScene::Hunting  ? "hunting"
                             : activeScene == GameScene::Park     ? "park"
                             : activeScene == GameScene::Prologue ? "story"
                                                                  : "quest_two";
        WorldMonitor::Publish(active,
                              {Tutorial::MonitorSnapshot(), LevelOne::MonitorSnapshot(),
                               Prologue::MonitorSnapshot(), QuestTwo::MonitorSnapshot(),
                               ParkQuest::MonitorSnapshot()},
                              g_Renderer->LastFrameStats().Total());
    }
}

void Resize(GLFWwindow*, int w, int h)
{
    if (w <= 0 || h <= 0)
    {
        return;
    }
    Tutorial::width = (std::max)(w, 1);
    Tutorial::height = (std::max)(h, 1);
    LevelOne::Resize(w, h);
    Prologue::Resize(w, h);
    QuestTwo::Resize(w, h);
    ParkQuest::Resize(w, h);
    if (g_Renderer)
        g_Renderer->Resize(w, h);
}

void GameKey(unsigned char k, bool pressed)
{
    if (activeScene == GameScene::Prologue)
    {
        Prologue::Key(k, pressed);
    }
    else if (activeScene == GameScene::World)
    {
        Tutorial::Key(k, pressed);
    }
    else if (activeScene == GameScene::Hunting)
    {
        LevelOne::Key(k, pressed);
    }
    else if (activeScene == GameScene::Park)
    {
        ParkQuest::Key(k, pressed);
    }
    else
    {
        QuestTwo::Key(k, pressed);
    }
}

void SpecialKey(int key)
{
    if (activeScene == GameScene::Park &&
        (key == GLFW_KEY_F1 || key == GLFW_KEY_F2 || key == GLFW_KEY_F3 || key == GLFW_KEY_F4 ||
         key == GLFW_KEY_F6 || key == GLFW_KEY_F7))
    {
        LeavePark();
    }
    if (key == GLFW_KEY_F7)
    {
        OpenWorld();
        Tutorial::LeaveBuilding();
        std::fill(Tutorial::keys, Tutorial::keys + 256, false);
        Tutorial::actions.height = Tutorial::actions.velocity = 0;
        Tutorial::spells.clear();
        Tutorial::particles.clear();
        Tutorial::miaQuestShortcut = true;
        const int destination = Tutorial::MiaQuestDestination();
        const auto& building =
            CoastalWorld::Buildings()[destination >= 0 ? destination : Tutorial::StoryPark];
        Tutorial::player = {CoastalWorld::DoorX(building), CoastalWorld::DoorY(building)};
        Tutorial::Say(Tutorial::MiaQuestObjective());
        return;
    }
    if (key == GLFW_KEY_F6 || key == GLFW_KEY_F1)
    {
        OpenWorld();
        return;
    }
    if (key == GLFW_KEY_F3)
    {
        float x, y;
        int person;
        std::string objective;
        if (Prologue::WorldObjective(x, y, person, objective))
        {
            OpenWorld();
            Tutorial::Say(objective);
            return;
        }
    }
    if (key == GLFW_KEY_F5)
    {
        Performance::Profiler::Get().Toggle();
        return;
    }
    if (key != GLFW_KEY_F1 && key != GLFW_KEY_F2 && key != GLFW_KEY_F3 && key != GLFW_KEY_F4)
    {
        if (activeScene == GameScene::Prologue)
        {
            Prologue::CameraKey(key, true);
        }
        return;
    }
    LevelOne::Pause();
    std::fill(Tutorial::keys, Tutorial::keys + 256, false);
    Tutorial::paused = true;
    Prologue::Pause();
    QuestTwo::Pause();
    activeScene = key == GLFW_KEY_F2   ? GameScene::Hunting
                  : key == GLFW_KEY_F3 ? GameScene::Prologue
                                       : GameScene::QuestTwo;
    if (activeScene == GameScene::QuestTwo)
    {
        OpenQuestTwo();
    }
    else if (activeScene == GameScene::Hunting)
    {
        huntingFromVillage = false;
        LevelOne::Resume(false);
        ApplyRestock(GameScene::Hunting);
    }
    else if (activeScene == GameScene::Prologue)
    {
        ApplyRestock(GameScene::Prologue);
        Prologue::ResumeFromWorld();
    }
    glfwSetWindowTitle(window, activeScene == GameScene::Prologue
                                  ? "마법소녀 - 프롤로그: 평범한 아침"
                              : activeScene == GameScene::Hunting
                                          ? "마법소녀 - 별빛 숲 사냥터"
                                  : "마법소녀 - 메인 퀘스트 2: 학교에 숨어든 것");
}

void KeyChanged(GLFWwindow*, int key, int, int action, int)
{
    if (action != GLFW_PRESS && action != GLFW_RELEASE)
    {
        return;
    }
    const bool pressed = action == GLFW_PRESS;
    const unsigned char character = GameInput::Character(key);
    if (character != 0)
    {
        GameKey(character, pressed);
    }
    else if (pressed)
    {
        SpecialKey(key);
    }
    else
    {
        Prologue::CameraKey(key, false);
    }
}

void Iconified(GLFWwindow*, int iconified)
{
    if (iconified)
    {
        std::fill(Tutorial::keys, Tutorial::keys + 256, false);
        Tutorial::paused = true;
        LevelOne::Pause();
        Prologue::Pause();
        QuestTwo::Pause();
        ParkQuest::Pause();
    }
}

void Close()
{
    Performance::Profiler::Get().FlushWindow();
    delete g_Renderer;
    g_Renderer = nullptr;
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);

    glfwSetErrorCallback(
        [](int code, const char* description)
        {
            std::cerr << "GLFW (" << code << "): " << description << '\n';
        });
    if (!glfwInit())
    {
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 0);
    glfwWindowHint(GLFW_STENCIL_BITS, 0);
    window = glfwCreateWindow(1280, 800, "마법소녀 - 프롤로그: 평범한 아침", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK || !GLEW_VERSION_3_3)
    {
        std::cerr << "OpenGL 3.3 초기화에 실패했습니다.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::wstring logName = L"profile_" + std::to_wstring(GetCurrentProcessId()) + L"_" +
                           std::to_wstring(GetTickCount64()) + L".jsonl";
    wchar_t profileSetting[8] = {};
    GetEnvironmentVariableW(L"EMBERWICK_PROFILE", profileSetting, 8);
    if (profileSetting[0] != L'0')
    {
        Performance::Profiler::Get().Initialize(
            RuntimeFiles::Path(logName.c_str()),
            reinterpret_cast<const char*>(glGetString(GL_RENDERER)),
            reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    }
    int framebufferWidth, framebufferHeight;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    g_Renderer = new Renderer(framebufferWidth, framebufferHeight);
    if (!g_Renderer->IsInitialized())
    {
        Close();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    Tutorial::renderer = g_Renderer;
    Tutorial::VisualModels();
    Tutorial::Reset();
    LevelOne::Initialize(g_Renderer);
    Prologue::Initialize(g_Renderer);
    QuestTwo::Initialize(g_Renderer);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    Resize(window, framebufferWidth, framebufferHeight);
    glfwSetFramebufferSizeCallback(window, Resize);
    glfwSetKeyCallback(window, KeyChanged);
    glfwSetWindowIconifyCallback(window, Iconified);

    constexpr double tickInterval = .016;
    previousTime = glfwGetTime();
    double nextTick = previousTime + tickInterval;
    RenderScene();
    while (!glfwWindowShouldClose(window))
    {
        const double remaining = nextTick - glfwGetTime();
        if (remaining > 0)
        {
            Performance::Scope scope("cpu.window.event_wait_ms");
            glfwWaitEventsTimeout(remaining);
        }
        else
        {
            Performance::Scope scope("cpu.window.event_poll_ms");
            glfwPollEvents();
        }
        if (glfwWindowShouldClose(window))
        {
            break;
        }
        const double now = glfwGetTime();
        if (now < nextTick)
        {
            continue;
        }

        Tick(now);
        nextTick = glfwGetTime() + tickInterval;
        if (!glfwGetWindowAttrib(window, GLFW_ICONIFIED))
        {
            RenderScene();
        }
    }
    Close();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
