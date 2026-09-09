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

static Renderer* g_Renderer = nullptr;
static int previousTime = 0;
void RenderScene() {
    Tutorial::Draw();
    glutSwapBuffers();
}
void Tick(int) {
    int now=glutGet(GLUT_ELAPSED_TIME);
    float dt=(std::min)((now-previousTime)/1000.f,.05f);
    previousTime=now;
    Tutorial::Update(dt);
    glutPostRedisplay();
    glutTimerFunc(16,Tick,0);
}
void Resize(int w,int h) {
    Tutorial::width=(std::max)(w,1); Tutorial::height=(std::max)(h,1);
    if(g_Renderer)g_Renderer->Resize(w,h);
}
void KeyDown(unsigned char k,int,int) { Tutorial::Key(k,true); }
void KeyUp(unsigned char k,int,int) { Tutorial::Key(k,false); }
void Visibility(int state) {
    if(state!=GLUT_VISIBLE) { std::fill(Tutorial::keys,Tutorial::keys+256,false);Tutorial::paused=true; }
}
void Close() {
    delete g_Renderer; g_Renderer=nullptr;
}
int main(int argc,char** argv) {
    SetConsoleOutputCP(CP_UTF8);
    glutInit(&argc,argv);
    glutInitContextVersion(3,3);
    // Keep the existing compatibility context; text now uses a Unicode texture atlas.
    glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA);
    glutInitWindowSize(1280,800);
    glutCreateWindow("Emberwick");
    SetWindowTextW(GetActiveWindow(),L"잔불 마을 - 황혼의 숲 튜토리얼");
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE,GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    if(glewInit()!=GLEW_OK || !GLEW_VERSION_3_3) {
        std::cerr<<"OpenGL 3.3 초기화에 실패했습니다.\n";return 1;
    }
    g_Renderer=new Renderer(1280,800);
    if(!g_Renderer->IsInitialized()) {Close();return 1;}
    Tutorial::renderer=g_Renderer;Tutorial::Reset();
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glutIgnoreKeyRepeat(1);
    glutDisplayFunc(RenderScene);glutReshapeFunc(Resize);
    glutKeyboardFunc(KeyDown);glutKeyboardUpFunc(KeyUp);
    glutVisibilityFunc(Visibility);glutCloseFunc(Close);
    previousTime=glutGet(GLUT_ELAPSED_TIME);glutTimerFunc(16,Tick,0);
    glutMainLoop();
    return 0;
}
