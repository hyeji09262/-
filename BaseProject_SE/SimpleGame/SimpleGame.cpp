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
#include "Tutorial.h"
#include "LevelOne.h"
#include "Profiler.h"
#include "RuntimeFiles.h"
#include "Prologue.h"
#include "QuestTwo.h"

static Renderer* g_Renderer = nullptr;
static int previousTime = 0;
enum class GameScene
{
    Prologue,
    Hunting,
    QuestTwo,
    World
};

static GameScene activeScene = GameScene::Prologue;

void OpenWorld()
{
    Prologue::Pause();
    QuestTwo::Pause();
    LevelOne::Pause();
    Tutorial::SetExploration(true, Prologue::TransformationUnlocked());
    Rpg::Progress progress;
    int fragments = 0;
    Tutorial::regionsUnlocked = Prologue::QuestTwoStart(progress, fragments);
    if (Tutorial::regionsUnlocked)
    {
        QuestTwo::Open(true, progress, fragments);
        QuestTwo::Pause();
    }
    Tutorial::shopOpen = false;
    Rpg::Progress inventory;
    if (QuestTwo::ShopProgress(inventory))
    {
        Tutorial::potions = inventory.potions;
        Tutorial::manaPotions = inventory.manaPotions;
    }
    activeScene = GameScene::World;
    SetWindowTextW(GetActiveWindow(), L"마법소녀 - 세 마을 탐험");
}

void OpenQuestTwo()
{
    Rpg::Progress inherited;
    int fragments = 4;
    const bool unlocked = Prologue::QuestTwoStart(inherited, fragments);
    Prologue::Pause();
    QuestTwo::Open(unlocked, inherited, fragments);
    activeScene = GameScene::QuestTwo;
    SetWindowTextW(GetActiveWindow(), L"마법소녀 - 메인 퀘스트 2: 학교에 숨어든 것");
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
    else
    {
        QuestTwo::Draw();
    }
    g_Renderer->EndFrame();
    {
        Performance::Scope scope("cpu.present.swap_wait_ms");
        glutSwapBuffers();
    }
    auto& profiler = Performance::Profiler::Get();
    profiler.Sample("cpu.frame_callback_ms", Performance::Milliseconds(start));
    const char* sceneName = activeScene == GameScene::Prologue  ? "prologue"
                            : activeScene == GameScene::World   ? "world"
                            : activeScene == GameScene::Hunting ? "level_one"
                                                                : "quest_two";
    profiler.Count(activeScene == GameScene::Prologue  ? "frame.scene.prologue"
                   : activeScene == GameScene::World   ? "frame.scene.world"
                   : activeScene == GameScene::Hunting ? "frame.scene.level_one"
                                                       : "frame.scene.quest_two");
    profiler.Frame(interval, sceneName, g_Renderer->LastFrameStats().frame);
}

void Tick(int)
{
    Performance::Scope scope("cpu.gameplay.tick_callback_ms");
    int now = glutGet(GLUT_ELAPSED_TIME);
    Performance::Profiler::Get().Sample("simulation.tick_interval_ms", now - previousTime);
    if (now - previousTime > 50)
    {
        Performance::Profiler::Get().Count("simulation.delta_time_clamped");
    }
    float dt = (std::min)((now - previousTime) / 1000.f, .05f);
    previousTime = now;

    if (activeScene == GameScene::Prologue)
    {
        Prologue::Update(dt);
        if (Prologue::ConsumeWorldRequest())
        {
            OpenWorld();
            Tutorial::LeaveBuilding();
            const auto& home = CoastalWorld::Buildings()[0];
            Tutorial::player = {CoastalWorld::DoorX(home), CoastalWorld::DoorY(home)};
            Tutorial::Say("학교에 가는 길입니다. 별빛 학교의 노란 출입구에서 E를 눌러 들어가세요.");
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
                    Prologue::ResumeFromWorld();
                    activeScene = GameScene::Prologue;
                    SetWindowTextW(GetActiveWindow(), L"마법소녀 - 메인 스토리");
                }
            }
            else if (request == 3)
            {
                const int room = Tutorial::interiorRoom;
                const auto position = Tutorial::player;
                OpenQuestTwo();
                QuestTwo::EnterFromSchool(room, position.x, position.y);
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
            OpenWorld();
            Tutorial::LeaveBuilding();
            Tutorial::Say("마을로 돌아왔습니다.");
        }
    }
    else if (activeScene == GameScene::QuestTwo)
    {
        QuestTwo::Update(dt);
        if (QuestTwo::ConsumeWorldRequest())
        {
            OpenWorld();
            Tutorial::LeaveBuilding();
            const auto& school = CoastalWorld::Buildings()[1];
            Tutorial::player = {CoastalWorld::DoorX(school), CoastalWorld::DoorY(school) + 1};
            Tutorial::Say(
                QuestTwo::Finished()
                    ? "학교 사건 해결! 노란 목표를 따라 편의점에 가서 직원에게 E로 물약을 구입하세요."
                    : "학교에서 나왔습니다. 노란 목표를 따라 조사를 이어갈 수 있습니다.");
        }
    }

    if (WorldMonitor::Due())
    {
        Performance::Scope monitorScope("cpu.monitor.collect_ms");
        const char* active = activeScene == GameScene::World      ? "world"
                             : activeScene == GameScene::Hunting  ? "hunting"
                             : activeScene == GameScene::Prologue ? "story"
                                                                  : "quest_two";
        WorldMonitor::Publish(active,
                              {Tutorial::MonitorSnapshot(), LevelOne::MonitorSnapshot(),
                               Prologue::MonitorSnapshot(), QuestTwo::MonitorSnapshot()},
                              g_Renderer->LastFrameStats().Total());
    }
    glutPostRedisplay();
    glutTimerFunc(16, Tick, 0);
}

void Resize(int w, int h)
{
    Tutorial::width = (std::max)(w, 1);
    Tutorial::height = (std::max)(h, 1);
    LevelOne::Resize(w, h);
    Prologue::Resize(w, h);
    QuestTwo::Resize(w, h);
    if (g_Renderer)
        g_Renderer->Resize(w, h);
}

void KeyDown(unsigned char k, int, int)
{
    if (activeScene == GameScene::Prologue)
    {
        Prologue::Key(k, true);
    }
    else if (activeScene == GameScene::World)
    {
        Tutorial::Key(k, true);
    }
    else if (activeScene == GameScene::Hunting)
    {
        LevelOne::Key(k, true);
    }
    else
    {
        QuestTwo::Key(k, true);
    }
}

void Mouse(int button, int state, int x, int y)
{
    if (activeScene == GameScene::QuestTwo && button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        QuestTwo::Mouse(x, y);
    }
}

void KeyUp(unsigned char k, int, int)
{
    if (activeScene == GameScene::Prologue)
    {
        Prologue::Key(k, false);
    }
    else if (activeScene == GameScene::World)
    {
        Tutorial::Key(k, false);
    }
    else if (activeScene == GameScene::Hunting)
    {
        LevelOne::Key(k, false);
    }
    else
    {
        QuestTwo::Key(k, false);
    }
}

void SpecialKey(int key, int, int)
{
    if (key == GLUT_KEY_F6 || key == GLUT_KEY_F1)
    {
        OpenWorld();
        return;
    }
    if (key == GLUT_KEY_F3)
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
    if (key == GLUT_KEY_F5)
    {
        Performance::Profiler::Get().Toggle();
        return;
    }
    if (key != GLUT_KEY_F1 && key != GLUT_KEY_F2 && key != GLUT_KEY_F3 && key != GLUT_KEY_F4)
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
    activeScene = key == GLUT_KEY_F2   ? GameScene::Hunting
                  : key == GLUT_KEY_F3 ? GameScene::Prologue
                                       : GameScene::QuestTwo;
    if (activeScene == GameScene::QuestTwo)
    {
        OpenQuestTwo();
    }
    SetWindowTextW(GetActiveWindow(), activeScene == GameScene::Prologue
                                          ? L"마법소녀 - 프롤로그: 평범한 아침"
                                      : activeScene == GameScene::Hunting
                                          ? L"마법소녀 - 방과 후 첫 임무"
                                          : L"마법소녀 - 메인 퀘스트 2: 학교에 숨어든 것");
}

void SpecialKeyUp(int key, int, int)
{
    Prologue::CameraKey(key, false);
}

void Visibility(int state)
{
    if (state != GLUT_VISIBLE)
    {
        std::fill(Tutorial::keys, Tutorial::keys + 256, false);
        Tutorial::paused = true;
        LevelOne::Pause();
        Prologue::Pause();
        QuestTwo::Pause();
    }
}

void Close()
{
    LevelOne::Save();
    Performance::Profiler::Get().FlushWindow();
    delete g_Renderer;
    g_Renderer = nullptr;
}

int main(int argc, char** argv)
{
    SetConsoleOutputCP(CP_UTF8);

    glutInit(&argc, argv);
    glutInitContextVersion(3, 3);
    // Keep the existing compatibility context; text now uses a Unicode texture atlas.
    glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(1280, 800);
    glutCreateWindow("Emberwick");
    SetWindowTextW(GetActiveWindow(), L"마법소녀 - 프롤로그: 평범한 아침");
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

    if (glewInit() != GLEW_OK || !GLEW_VERSION_3_3)
    {
        std::cerr << "OpenGL 3.3 초기화에 실패했습니다.\n";
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
    g_Renderer = new Renderer(1280, 800);
    if (!g_Renderer->IsInitialized())
    {
        Close();
        return 1;
    }
    Tutorial::renderer = g_Renderer;
    Tutorial::VisualModels();
    Tutorial::Reset();
    LevelOne::Initialize(g_Renderer);
    Prologue::Initialize(g_Renderer);
    QuestTwo::Initialize(g_Renderer);
    float objectiveX, objectiveY;
    int objectivePerson;
    std::string objectiveText;
    if (Prologue::WorldObjective(objectiveX, objectiveY, objectivePerson, objectiveText))
    {
        OpenWorld();
    }
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glutIgnoreKeyRepeat(1);
    glutDisplayFunc(RenderScene);
    glutReshapeFunc(Resize);
    glutKeyboardFunc(KeyDown);
    glutMouseFunc(Mouse);
    glutKeyboardUpFunc(KeyUp);
    glutSpecialFunc(SpecialKey);
    glutSpecialUpFunc(SpecialKeyUp);
    glutVisibilityFunc(Visibility);
    glutCloseFunc(Close);
    previousTime = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, Tick, 0);
    glutMainLoop();
    return 0;
}
