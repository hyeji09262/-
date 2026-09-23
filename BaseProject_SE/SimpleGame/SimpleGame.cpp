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

static Renderer* g_Renderer = nullptr;
static int previousTime = 0;
static bool showTutorial = false;

void RenderScene()
{
    static auto previousFrame = Performance::Clock::now();
    auto start = Performance::Clock::now();
    double interval = Performance::Milliseconds(previousFrame, start);
    previousFrame = start;
    g_Renderer->BeginFrame();
    if (showTutorial)
    {
        Tutorial::Draw();
    }
    else
    {
        LevelOne::Draw();
    }
    g_Renderer->EndFrame();
    {
        Performance::Scope scope("cpu.present.swap_wait_ms");
        glutSwapBuffers();
    }
    auto& profiler = Performance::Profiler::Get();
    profiler.Sample("cpu.frame_callback_ms", Performance::Milliseconds(start));
    profiler.Count(showTutorial ? "frame.scene.tutorial" : "frame.scene.level_one");
    profiler.Frame(interval, showTutorial ? "tutorial" : "level_one",
                   g_Renderer->LastFrameStats().frame);
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

    if (showTutorial)
    {
        Tutorial::Update(dt);
    }
    else
    {
        LevelOne::Update(dt);
    }

    glutPostRedisplay();
    glutTimerFunc(16, Tick, 0);
}

void Resize(int w, int h)
{
    Tutorial::width = (std::max)(w, 1);
    Tutorial::height = (std::max)(h, 1);
    LevelOne::Resize(w, h);
    if (g_Renderer)
        g_Renderer->Resize(w, h);
}

void KeyDown(unsigned char k, int, int)
{
    if (showTutorial)
    {
        Tutorial::Key(k, true);
    }
    else
    {
        LevelOne::Key(k, true);
    }
}

void KeyUp(unsigned char k, int, int)
{
    if (showTutorial)
    {
        Tutorial::Key(k, false);
    }
    else
    {
        LevelOne::Key(k, false);
    }
}

void SpecialKey(int key, int, int)
{
    if (key == GLUT_KEY_F3)
    {
        Performance::Profiler::Get().Toggle();
        return;
    }
    if (key != GLUT_KEY_F1 && key != GLUT_KEY_F2)
    {
        return;
    }
    LevelOne::Pause();
    std::fill(Tutorial::keys, Tutorial::keys + 256, false);
    Tutorial::paused = true;
    showTutorial = key == GLUT_KEY_F1;
    SetWindowTextW(GetActiveWindow(),
                   showTutorial ? L"별빛 학교 - 마법소녀의 첫날" : L"마법소녀 - 방과 후 첫 임무");
}

void Visibility(int state)
{
    if (state != GLUT_VISIBLE)
    {
        std::fill(Tutorial::keys, Tutorial::keys + 256, false);
        Tutorial::paused = true;
        LevelOne::Pause();
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
    SetWindowTextW(GetActiveWindow(), L"마법소녀 - 방과 후 첫 임무");
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
    Tutorial::Reset();
    LevelOne::Initialize(g_Renderer);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glutIgnoreKeyRepeat(1);
    glutDisplayFunc(RenderScene);
    glutReshapeFunc(Resize);
    glutKeyboardFunc(KeyDown);
    glutKeyboardUpFunc(KeyUp);
    glutSpecialFunc(SpecialKey);
    glutVisibilityFunc(Visibility);
    glutCloseFunc(Close);
    previousTime = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, Tick, 0);
    glutMainLoop();
    return 0;
}
