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

static Renderer* g_Renderer = nullptr;
static int previousTime = 0;
static bool showTutorial = false;

void RenderScene()
{
    if (showTutorial)
    {
        Tutorial::Draw();
    }
    else
    {
        LevelOne::Draw();
    }
    glutSwapBuffers();
}

void Tick(int)
{
    int now = glutGet(GLUT_ELAPSED_TIME);
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
    if (key != GLUT_KEY_F1 && key != GLUT_KEY_F2)
    {
        return;
    }
    LevelOne::Pause();
    std::fill(Tutorial::keys, Tutorial::keys + 256, false);
    Tutorial::paused = true;
    showTutorial = key == GLUT_KEY_F1;
    SetWindowTextW(GetActiveWindow(),
                   showTutorial ? L"잔불 마을 - 튜토리얼" : L"첫 번째 레벨 - 황혼의 사냥터");
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
    SetWindowTextW(GetActiveWindow(), L"첫 번째 레벨 - 황혼의 사냥터");
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

    if (glewInit() != GLEW_OK || !GLEW_VERSION_3_3)
    {
        std::cerr << "OpenGL 3.3 초기화에 실패했습니다.\n";
        return 1;
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
