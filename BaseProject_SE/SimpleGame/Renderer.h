#pragma once
#include <string>
#include <vector>
#include "Dependencies/glew.h"

// Batched 2D geometry, cached Unicode glyph atlas, and a world-only post pass.
class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    bool IsInitialized() const { return m_Initialized; }
    void Resize(int width, int height);
    void BeginWorld();
    void EndWorld(float time);
    void Flush();
    void Triangle(float x1,float y1,float x2,float y2,float x3,float y3,
        float r,float g,float b,float a=1.f);
    void DrawSolidRect(float x,float y,float z,float size,float r,float g,float b,float a);
    void Text(float x,float baseline,const std::string& utf8,float size,float r,float g,float b,float a=1.f);
private:
    struct Vertex { float x,y,u,v,r,g,b,a; };
    struct Font;
    Font* m_Font=nullptr;
    std::vector<Vertex> m_Vertices;
    int m_Width=1,m_Height=1;
    GLuint m_Buffer=0,m_VAO=0,m_Shader=0,m_Post=0,m_Framebuffer=0,m_Scene=0;
    GLint m_Textured=-1,m_PostTime=-1,m_PostTexel=-1;
    bool m_Initialized=false,m_TargetValid=false;
    bool ReadFile(const char* name,std::string& out);
    GLuint CompileShaders(const char* vertex,const char* fragment);
    void Upload(const std::vector<Vertex>& vertices,bool textured);
};
