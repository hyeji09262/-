#include "stdafx.h"
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <unordered_map>
#include "Renderer.h"

struct Renderer::Font {
    struct Glyph { int slot; float advance; };
    HDC dc=nullptr; HBITMAP bitmap=nullptr; HFONT font=nullptr;
    HGDIOBJ oldBitmap=nullptr,oldFont=nullptr;
    void* pixels=nullptr;
    GLuint texture=0;
    std::unordered_map<wchar_t,Glyph> glyphs;
    static const int Cell=40, Atlas=2048, Columns=Atlas/Cell;
    bool Initialize() {
        dc=CreateCompatibleDC(nullptr);
        if(!dc)return false;
        BITMAPINFO info={};
        info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth=Cell;info.bmiHeader.biHeight=-Cell;
        info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
        bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0);
        font=CreateFontW(-28,0,0,0,FW_MEDIUM,FALSE,FALSE,FALSE,HANGUL_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"맑은 고딕");
        if(!bitmap||!font)return false;
        oldBitmap=SelectObject(dc,bitmap);oldFont=SelectObject(dc,font);
        SetTextColor(dc,RGB(255,255,255));SetBkColor(dc,RGB(0,0,0));
        glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
        std::vector<unsigned char> empty(Atlas*Atlas*4,0);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,Atlas,Atlas,0,GL_RGBA,GL_UNSIGNED_BYTE,empty.data());
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        return true;
    }
    Glyph Get(wchar_t c) {
        auto found=glyphs.find(c);if(found!=glyphs.end())return found->second;
        if(glyphs.size()>=Columns*Columns)return glyphs.begin()->second;
        GdiFlush();ZeroMemory(pixels,Cell*Cell*4);
        TextOutW(dc,2,2,&c,1);GdiFlush();
        SIZE size={};GetTextExtentPoint32W(dc,&c,1,&size);
        unsigned char* src=static_cast<unsigned char*>(pixels);
        std::vector<unsigned char> rgba(Cell*Cell*4);
        for(int i=0;i<Cell*Cell;++i) {
            rgba[i*4]=rgba[i*4+1]=rgba[i*4+2]=255;
            rgba[i*4+3]=(std::max)(src[i*4],(std::max)(src[i*4+1],src[i*4+2]));
        }
        Glyph g={static_cast<int>(glyphs.size()),static_cast<float>(size.cx)};
        glBindTexture(GL_TEXTURE_2D,texture);
        glTexSubImage2D(GL_TEXTURE_2D,0,(g.slot%Columns)*Cell,(g.slot/Columns)*Cell,
            Cell,Cell,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
        glyphs.emplace(c,g);return g;
    }
    ~Font() {
        if(oldBitmap)SelectObject(dc,oldBitmap);
        if(oldFont)SelectObject(dc,oldFont);
        if(bitmap)DeleteObject(bitmap);if(font)DeleteObject(font);if(dc)DeleteDC(dc);
        if(texture)glDeleteTextures(1,&texture);
    }
};
Renderer::Renderer(int w,int h) {
    m_Shader=CompileShaders("Shaders/SolidRect.vs","Shaders/SolidRect.fs");
    m_Post=CompileShaders("Shaders/Screen.vs","Shaders/Post.fs");
    if(!m_Shader||!m_Post)return;
    glGenVertexArrays(1,&m_VAO);glBindVertexArray(m_VAO);
    glGenBuffers(1,&m_Buffer);
    m_Textured=glGetUniformLocation(m_Shader,"u_Textured");
    glUseProgram(m_Shader);glUniform1i(glGetUniformLocation(m_Shader,"u_Atlas"),0);
    glUseProgram(m_Post);glUniform1i(glGetUniformLocation(m_Post,"u_Scene"),0);
    m_PostTime=glGetUniformLocation(m_Post,"u_Time");m_PostTexel=glGetUniformLocation(m_Post,"u_Texel");
    m_Font=new Font();
    if(!m_Font->Initialize()){std::cerr<<"한글 글꼴 초기화 실패\n";return;}
    glGenFramebuffers(1,&m_Framebuffer);glGenTextures(1,&m_Scene);
    Resize(w,h);m_Initialized=m_TargetValid;
    m_Vertices.reserve(32768);
}
Renderer::~Renderer() {
    delete m_Font;
    glDeleteTextures(1,&m_Scene);glDeleteFramebuffers(1,&m_Framebuffer);
    glDeleteBuffers(1,&m_Buffer);glDeleteVertexArrays(1,&m_VAO);
    if(m_Shader)glDeleteProgram(m_Shader);if(m_Post)glDeleteProgram(m_Post);
}
bool Renderer::ReadFile(const char* name,std::string& out) {
    std::ifstream file(name);
    if(!file) {
        wchar_t module[32768]={};GetModuleFileNameW(nullptr,module,32768);
        std::wstring path(module);path=path.substr(0,path.find_last_of(L"\\/")+1);
        while(*name)path+=static_cast<wchar_t>(*name++);
        file.clear();file.open(path.c_str());
    }
    if(!file)return false;
    out.assign(std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>());return true;
}
GLuint Renderer::CompileShaders(const char* vn,const char* fn) {
    std::string sources[2];
    if(!ReadFile(vn,sources[0])||!ReadFile(fn,sources[1])){std::cerr<<"셰이더 파일을 찾을 수 없습니다: "<<vn<<" / "<<fn<<"\n";return 0;}
    GLuint program=glCreateProgram();
    for(int i=0;i<2;++i) {
        GLuint shader=glCreateShader(i==0?GL_VERTEX_SHADER:GL_FRAGMENT_SHADER);
        const char* src=sources[i].c_str();glShaderSource(shader,1,&src,nullptr);glCompileShader(shader);
        GLint ok=0;glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
        if(!ok) {
            char log[4096]={};glGetShaderInfoLog(shader,sizeof(log),nullptr,log);
            std::cerr<<log;glDeleteShader(shader);glDeleteProgram(program);return 0;
        }
        glAttachShader(program,shader);glDeleteShader(shader);
    }
    glLinkProgram(program);GLint ok=0;glGetProgramiv(program,GL_LINK_STATUS,&ok);
    if(!ok) {
        char log[4096]={};glGetProgramInfoLog(program,sizeof(log),nullptr,log);
        std::cerr<<log;glDeleteProgram(program);return 0;
    }
    return program;
}
void Renderer::Resize(int w,int h) {
    Flush();m_Width=(std::max)(1,w);m_Height=(std::max)(1,h);
    glBindTexture(GL_TEXTURE_2D,m_Scene);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,m_Width,m_Height,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_FRAMEBUFFER,m_Framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,m_Scene,0);
    m_TargetValid=glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE;
    if(!m_TargetValid)std::cerr<<"후처리 프레임버퍼 생성 실패\n";
    glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,m_Width,m_Height);
}
void Renderer::BeginWorld() {
    Flush();glBindFramebuffer(GL_FRAMEBUFFER,m_TargetValid?m_Framebuffer:0);
    glViewport(0,0,m_Width,m_Height);glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(.08f,.09f,.14f,1);glClear(GL_COLOR_BUFFER_BIT);
}
void Renderer::EndWorld(float time) {
    Flush();if(!m_TargetValid)return;
    glBindFramebuffer(GL_FRAMEBUFFER,0);glDisable(GL_BLEND);
    glUseProgram(m_Post);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,m_Scene);
    glUniform1f(m_PostTime,time);glUniform2f(m_PostTexel,1.f/m_Width,1.f/m_Height);
    glBindVertexArray(m_VAO);glDrawArrays(GL_TRIANGLES,0,3);glEnable(GL_BLEND);
}
void Renderer::Upload(const std::vector<Vertex>& v,bool textured) {
    if(v.empty())return;
    glUseProgram(m_Shader);glUniform1i(m_Textured,textured?1:0);
    glBindVertexArray(m_VAO);glBindBuffer(GL_ARRAY_BUFFER,m_Buffer);
    glBufferData(GL_ARRAY_BUFFER,v.size()*sizeof(Vertex),v.data(),GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);
    glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(2*sizeof(float)));
    glEnableVertexAttribArray(2);glVertexAttribPointer(2,4,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(4*sizeof(float)));
    glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(v.size()));
}
void Renderer::Flush() { Upload(m_Vertices,false);m_Vertices.clear(); }
void Renderer::Triangle(float x1,float y1,float x2,float y2,float x3,float y3,float r,float g,float b,float a) {
    m_Vertices.push_back({x1*2/m_Width-1,1-y1*2/m_Height,0,0,r,g,b,a});
    m_Vertices.push_back({x2*2/m_Width-1,1-y2*2/m_Height,0,0,r,g,b,a});
    m_Vertices.push_back({x3*2/m_Width-1,1-y3*2/m_Height,0,0,r,g,b,a});
    if(m_Vertices.size()>60000)Flush();
}
void Renderer::DrawSolidRect(float x,float y,float z,float size,float r,float g,float b,float a) {
    (void)z;float sx=m_Width*.5f+x,sy=m_Height*.5f-y,h=size*.5f;
    Triangle(sx-h,sy-h,sx+h,sy-h,sx+h,sy+h,r,g,b,a);Triangle(sx-h,sy-h,sx+h,sy+h,sx-h,sy+h,r,g,b,a);
}
void Renderer::Text(float x,float baseline,const std::string& utf8,float size,float r,float g,float b,float a) {
    Flush();if(!m_Font||!m_Font->texture||utf8.empty())return;
    int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,utf8.data(),static_cast<int>(utf8.size()),nullptr,0);
    if(count<=0)return;
    std::wstring text(count,L' ');MultiByteToWideChar(CP_UTF8,0,utf8.data(),static_cast<int>(utf8.size()),&text[0],count);
    std::vector<Vertex> vertices;vertices.reserve(count*6);
    float scale=size/28.f,start=x;
    for(wchar_t ch:text) {
        if(ch==L'\n'){x=start;baseline+=size*1.5f;continue;}
        auto glyph=m_Font->Get(ch);
        float u=float(glyph.slot%Font::Columns*Font::Cell)/Font::Atlas;
        float v=float(glyph.slot/Font::Columns*Font::Cell)/Font::Atlas;
        float span=float(Font::Cell)/Font::Atlas;
        float left=(x-2*scale)*2/m_Width-1,right=(x+38*scale)*2/m_Width-1;
        float top=1-(baseline-size-2*scale)*2/m_Height,bottom=top-40*scale*2/m_Height;
        Vertex q[4]={{left,top,u,v,r,g,b,a},{right,top,u+span,v,r,g,b,a},
            {right,bottom,u+span,v+span,r,g,b,a},{left,bottom,u,v+span,r,g,b,a}};
        for(int i:{0,1,2,0,2,3})vertices.push_back(q[i]);
        x+=(glyph.advance+1)*scale;
    }
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,m_Font->texture);Upload(vertices,true);
}
