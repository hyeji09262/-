#include "stdafx.h"
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <unordered_map>
#include "Renderer.h"

struct Renderer::Font
{
    struct Glyph
    {
        int slot;
        float advance;
    };

    HDC dc = nullptr;
    HBITMAP bitmap = nullptr;
    HFONT font = nullptr;
    HGDIOBJ oldBitmap = nullptr, oldFont = nullptr;
    void* pixels = nullptr;
    GLuint texture = 0;
    std::unordered_map<wchar_t, Glyph> glyphs;
    static const int Cell = 40, Atlas = 2048, Columns = Atlas / Cell;

    bool Initialize()
    {
        dc = CreateCompatibleDC(nullptr);
        if (!dc)
            return false;

        BITMAPINFO info = {};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = Cell;
        info.bmiHeader.biHeight = -Cell;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
        font = CreateFontW(-28, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, HANGUL_CHARSET,
                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                           DEFAULT_PITCH, L"맑은 고딕");
        if (!bitmap || !font)
            return false;

        oldBitmap = SelectObject(dc, bitmap);
        oldFont = SelectObject(dc, font);
        SetTextColor(dc, RGB(255, 255, 255));
        SetBkColor(dc, RGB(0, 0, 0));

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        std::vector<unsigned char> empty(Atlas * Atlas * 4, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Atlas, Atlas, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     empty.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        return true;
    }

    Glyph Get(wchar_t c)
    {
        auto found = glyphs.find(c);
        if (found != glyphs.end())
            return found->second;
        if (glyphs.size() >= Columns * Columns)
        {
            Performance::Profiler::Get().Count("font.atlas_capacity_fallbacks");
            return glyphs.begin()->second;
        }
        Performance::Scope rasterScope("cpu.font.rasterize_ms");
        Performance::Profiler::Get().Count("font.glyph_cache_misses");
        Performance::Profiler::Get().Count("upload.font_bytes", Cell * Cell * 4);
        glActiveTexture(GL_TEXTURE0);
        GdiFlush();
        ZeroMemory(pixels, Cell * Cell * 4);
        TextOutW(dc, 2, 2, &c, 1);
        GdiFlush();
        SIZE size = {};
        GetTextExtentPoint32W(dc, &c, 1, &size);
        unsigned char* src = static_cast<unsigned char*>(pixels);
        std::vector<unsigned char> rgba(Cell * Cell * 4);
        for (int i = 0; i < Cell * Cell; ++i)
        {
            rgba[i * 4] = rgba[i * 4 + 1] = rgba[i * 4 + 2] = 255;
            rgba[i * 4 + 3] = (std::max)(src[i * 4], (std::max)(src[i * 4 + 1], src[i * 4 + 2]));
        }
        Glyph g = {static_cast<int>(glyphs.size()), static_cast<float>(size.cx)};
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, (g.slot % Columns) * Cell, (g.slot / Columns) * Cell,
                        Cell, Cell, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
        glyphs.emplace(c, g);
        return g;
    }

    ~Font()
    {
        if (oldBitmap)
            SelectObject(dc, oldBitmap);
        if (oldFont)
            SelectObject(dc, oldFont);
        if (bitmap)
            DeleteObject(bitmap);
        if (font)
            DeleteObject(font);
        if (dc)
            DeleteDC(dc);
        if (texture)
            glDeleteTextures(1, &texture);
    }
};

Renderer::Renderer(int w, int h)
{
    m_Shader = CompileShaders("Shaders/SolidRect.vs", "Shaders/SolidRect.fs");
    m_Post = CompileShaders("Shaders/Screen.vs", "Shaders/Post.fs");
    m_ModelShader = CompileShaders("Shaders/Model.vs", "Shaders/Model.fs");
    m_BloomShader = CompileShaders("Shaders/Screen.vs", "Shaders/Bloom.fs");
    if (!m_Shader || !m_Post || !m_ModelShader || !m_BloomShader)
        return;

    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);
    glGenBuffers(1, &m_Buffer);
    glGenVertexArrays(1, &m_ModelVAO);
    glGenBuffers(1, &m_ModelBuffer);
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &m_MaxModelTexels);
    glGenBuffers(1, &m_InstanceBuffer);
    glGenTextures(1, &m_ModelTexture);
    glUseProgram(m_ModelShader);
    glUniform1i(glGetUniformLocation(m_ModelShader, "u_ModelData"), 1);
    m_ModelViewport = glGetUniformLocation(m_ModelShader, "u_Viewport");

    glUseProgram(m_Shader);
    glUniform1i(glGetUniformLocation(m_Shader, "u_Atlas"), 0);
    glUseProgram(m_Post);
    glUniform1i(glGetUniformLocation(m_Post, "u_Scene"), 0);
    glUniform1i(glGetUniformLocation(m_Post, "u_Bloom"), 1);
    m_PostTime = glGetUniformLocation(m_Post, "u_Time");
    glUseProgram(m_BloomShader);
    glUniform1i(glGetUniformLocation(m_BloomShader, "u_Source"), 0);
    m_BloomPass = glGetUniformLocation(m_BloomShader, "u_Pass");
    m_BloomTexel = glGetUniformLocation(m_BloomShader, "u_Texel");
    glGenFramebuffers(2, m_BloomBuffers);
    glGenTextures(2, m_BloomTextures);
    for (auto& query : m_GpuQueries)
    {
        glGenQueries(4, query.timestamps);
    }

    m_Font = new Font();
    if (!m_Font->Initialize())
    {
        std::cerr << "한글 글꼴 초기화 실패\n";
        return;
    }

    glGenFramebuffers(1, &m_Framebuffer);
    glGenTextures(1, &m_Scene);
    Resize(w, h);
    m_Initialized = m_TargetValid;
    m_Vertices.reserve(65536);
    m_Instances.reserve(512);
}

Renderer::~Renderer()
{
    glDeleteProgram(m_ModelShader);
    glDeleteProgram(m_BloomShader);
    glDeleteBuffers(1, &m_ModelBuffer);
    glDeleteBuffers(1, &m_InstanceBuffer);
    glDeleteVertexArrays(1, &m_ModelVAO);
    glDeleteTextures(1, &m_ModelTexture);
    glDeleteTextures(2, m_BloomTextures);
    glDeleteFramebuffers(2, m_BloomBuffers);
    for (auto& query : m_GpuQueries)
    {
        glDeleteQueries(4, query.timestamps);
    }
    delete m_Font;
    glDeleteTextures(1, &m_Scene);
    glDeleteFramebuffers(1, &m_Framebuffer);
    glDeleteBuffers(1, &m_Buffer);
    glDeleteVertexArrays(1, &m_VAO);
    if (m_Shader)
        glDeleteProgram(m_Shader);
    if (m_Post)
        glDeleteProgram(m_Post);
}

bool Renderer::ReadFile(const char* name, std::string& out)
{
    std::ifstream file(name);
    if (!file)
    {
        wchar_t module[32768] = {};
        GetModuleFileNameW(nullptr, module, 32768);
        std::wstring path(module);
        path = path.substr(0, path.find_last_of(L"\\/") + 1);
        while (*name)
            path += static_cast<wchar_t>(*name++);
        file.clear();
        file.open(path.c_str());
    }
    if (!file)
        return false;
    out.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return true;
}

GLuint Renderer::CompileShaders(const char* vn, const char* fn)
{
    Performance::Scope scope("cpu.assets.shader_compile_link_ms");
    std::string sources[2];
    if (!ReadFile(vn, sources[0]) || !ReadFile(fn, sources[1]))
    {
        std::cerr << "셰이더 파일을 찾을 수 없습니다: " << vn << " / " << fn << "\n";
        return 0;
    }
    GLuint program = glCreateProgram();
    for (int i = 0; i < 2; ++i)
    {
        GLuint shader = glCreateShader(i == 0 ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
        const char* src = sources[i].c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        GLint ok = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            char log[4096] = {};
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            std::cerr << log;
            glDeleteShader(shader);
            glDeleteProgram(program);
            return 0;
        }
        glAttachShader(program, shader);
        glDeleteShader(shader);
    }
    glLinkProgram(program);
    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[4096] = {};
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << log;
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

void Renderer::Resize(int w, int h)
{
    Performance::Scope scope("cpu.render.resize_ms");
    Flush();
    glActiveTexture(GL_TEXTURE0);
    m_Width = (std::max)(1, w);
    m_Height = (std::max)(1, h);
    glBindTexture(GL_TEXTURE_2D, m_Scene);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_Scene, 0);
    m_TargetValid = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    m_BloomWidth = (std::max)(1, m_Width / 4);
    m_BloomHeight = (std::max)(1, m_Height / 4);
    for (int i = 0; i < 2; ++i)
    {
        glBindTexture(GL_TEXTURE_2D, m_BloomTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_BloomWidth, m_BloomHeight, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_FRAMEBUFFER, m_BloomBuffers[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               m_BloomTextures[i], 0);
        m_TargetValid =
            m_TargetValid && glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    }
    if (!m_TargetValid)
        std::cerr << "후처리 프레임버퍼 생성 실패\n";
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_Width, m_Height);
}

void Renderer::BeginFrame()
{
    m_FrameStats = {};
    m_FrameStats.frame = m_LastFrameStats.frame + 1;
    m_FrameActive = true;
    auto now = Performance::Clock::now();
    m_FrameIntervalMs = Performance::Milliseconds(m_PreviousFrameStart, now);
    m_PreviousFrameStart = now;
    PollGpuQueries();
    m_ActiveGpuQuery = -1;
    if (Performance::Profiler::Get().Enabled())
    {
        for (size_t i = 0; i < m_GpuQueries.size(); ++i)
        {
            if (!m_GpuQueries[i].pending)
            {
                m_ActiveGpuQuery = static_cast<int>(i);
                m_GpuQueries[i].frame = m_FrameStats.frame;
                break;
            }
        }
        if (m_ActiveGpuQuery < 0)
        {
            Performance::Profiler::Get().Count("gpu.query_ring_full_frames");
        }
    }
}

void Renderer::EndFrame()
{
    if (!m_FrameActive)
    {
        return;
    }
    Flush();
    GpuTimestamp(3);
    if (m_ActiveGpuQuery >= 0)
    {
        m_GpuQueries[m_ActiveGpuQuery].pending = true;
    }
    auto& profiler = Performance::Profiler::Get();
    profiler.Count("render.frames");
    profiler.Count("render.draw_calls.total", m_FrameStats.Total());
    profiler.Count("render.draw_calls.geometry", m_FrameStats.geometry);
    profiler.Count("render.draw_calls.text", m_FrameStats.text);
    profiler.Count("render.draw_calls.effects", m_FrameStats.effects);
    profiler.Count("render.draw_calls.mixed", m_FrameStats.mixed);
    profiler.Count("render.draw_calls.post", m_FrameStats.postProcess);
    profiler.Count("render.viewport_pixels", double(m_Width) * m_Height);
    profiler.Gauge("memory.model_cache_gpu_bytes", double(m_ModelData.size() * sizeof(float)));
    profiler.Gauge("memory.model_cache_cpu_capacity_bytes",
                   double(m_ModelData.capacity() * sizeof(float)));
    profiler.Gauge("memory.stream_cpu_capacity_bytes",
                   double(m_Vertices.capacity() * sizeof(Vertex)));
    profiler.Gauge("memory.instance_cpu_capacity_bytes",
                   double(m_Instances.capacity() * sizeof(ModelInstance)));
    profiler.Gauge("memory.font_atlas_gpu_bytes", Font::Atlas * Font::Atlas * 4.0);
    profiler.Gauge("memory.render_targets_gpu_bytes",
                   (double(m_Width) * m_Height + double(m_BloomWidth) * m_BloomHeight * 2) * 4);
    profiler.Gauge("render.model_cache_entries", double(m_ModelCache.size()));
    if (profiler.Enabled() && m_FrameStats.frame % 120 == 0)
    {
        for (int i = 0; i < 8; ++i)
        {
            GLenum error = glGetError();
            if (error == GL_NO_ERROR)
            {
                break;
            }
            profiler.Count("opengl.errors");
            profiler.Gauge("opengl.last_error_code", error);
        }
    }
    m_LastFrameStats = m_FrameStats;
    m_FrameActive = false;
}

void Renderer::SubmitDrawArrays(GLenum mode, GLint first, GLsizei count, DrawCategory category)
{
    // Count actual API submissions, not actors, triangles, or empty Flush() calls.
    glDrawArrays(mode, first, count);
    if (!m_FrameActive)
    {
        return;
    }
    switch (category)
    {
    case DrawCategory::Geometry:
        ++m_FrameStats.geometry;
        break;
    case DrawCategory::Text:
        ++m_FrameStats.text;
        break;
    case DrawCategory::Effect:
        ++m_FrameStats.effects;
        break;
    case DrawCategory::Mixed:
        ++m_FrameStats.mixed;
        break;
    case DrawCategory::PostProcess:
        ++m_FrameStats.postProcess;
        break;
    }
}

void Renderer::DrawFrameStats()
{
    // Show a completed frame so this overlay's own draw calls are included without prediction.
    float sx = m_Width / 1280.f;
    float sy = m_Height / 800.f;
    float left = 850 * sx, right = 1260 * sx, top = 238 * sy, bottom = 335 * sy;
    Triangle(left, top, right, top, right, bottom, .03f, .04f, .07f, .94f);
    Triangle(left, top, right, bottom, left, bottom, .03f, .04f, .07f, .94f);

    if (m_LastFrameStats.frame == 0)
    {
        Text(862 * sx, 266 * sy, "Draw call: 첫 프레임 집계 중", 17 * sy, 1, .85f, .5f);
        return;
    }
    const FrameStats& stats = m_LastFrameStats;
    std::string total = "이전 프레임 #" + std::to_string(stats.frame) +
                        " | Draw call: " + std::to_string(stats.Total());
    std::string detail = "도형 " + std::to_string(stats.geometry) + " / 글자 " +
                         std::to_string(stats.text) + " / 효과 " + std::to_string(stats.effects) +
                         " / 후처리 " + std::to_string(stats.postProcess);
    Text(862 * sx, 265 * sy, total, 16 * sy, 1, .85f, .5f);
    Text(862 * sx, 292 * sy, detail + " / 혼합 " + std::to_string(stats.mixed), 12 * sy, .86f, .90f,
         1);
    std::string performance =
        "FPS " + std::to_string(int(1000.0 / (std::max)(1.0, m_FrameIntervalMs))) + " | 로그 " +
        (Performance::Profiler::Get().Enabled() ? "ON" : "OFF") + " [F3]";
    Text(862 * sx, 321 * sy, performance, 14 * sy, .86f, .90f, 1);
}

void Renderer::Nameplate(float centerX, float baseline, const std::string& label, float size)
{
    if (!m_Font || label.empty())
    {
        return;
    }
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, label.data(),
                                    static_cast<int>(label.size()), nullptr, 0);
    if (count <= 0)
    {
        return;
    }
    std::wstring wide(count, L' ');
    MultiByteToWideChar(CP_UTF8, 0, label.data(), static_cast<int>(label.size()), &wide[0], count);
    float advance = 0;
    for (wchar_t letter : wide)
    {
        advance += (m_Font->Get(letter).advance + 1) * size / 28.f;
    }
    float left = centerX - advance * .5f - size * .5f;
    float right = centerX + advance * .5f + size * .5f;
    float top = baseline - size * 1.3f, bottom = baseline + size * .4f;
    Triangle(left, top, right, top, right, bottom, .04f, .04f, .09f, .94f);
    Triangle(left, top, right, bottom, left, bottom, .04f, .04f, .09f, .94f);
    Text(centerX - advance * .5f, baseline, label, size, 1, .95f, .86f);
}

void Renderer::BeginWorld()
{
    Flush();
    GpuTimestamp(0);
    glBindFramebuffer(GL_FRAMEBUFFER, m_TargetValid ? m_Framebuffer : 0);
    glViewport(0, 0, m_Width, m_Height);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(.08f, .09f, .14f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::EndWorld(float time)
{
    Flush();
    GpuTimestamp(1);
    Performance::Scope scope("cpu.render.post_ms");
    if (m_TargetValid)
    {
        glDisable(GL_BLEND);
        glBindVertexArray(m_VAO);
        glUseProgram(m_BloomShader);
        glActiveTexture(GL_TEXTURE0);
        glViewport(0, 0, m_BloomWidth, m_BloomHeight);
        for (int pass = 0; pass < 3; ++pass)
        {
            int destination = pass == 1 ? 1 : 0;
            GLuint source = pass == 0 ? m_Scene : m_BloomTextures[pass == 1 ? 0 : 1];
            glBindFramebuffer(GL_FRAMEBUFFER, m_BloomBuffers[destination]);
            glBindTexture(GL_TEXTURE_2D, source);
            glUniform1i(m_BloomPass, pass);
            glUniform2f(m_BloomTexel, 1.f / (pass == 0 ? m_Width : m_BloomWidth),
                        1.f / (pass == 0 ? m_Height : m_BloomHeight));
            SubmitDrawArrays(GL_TRIANGLES, 0, 3, DrawCategory::PostProcess);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, m_Width, m_Height);
        glUseProgram(m_Post);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_Scene);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_BloomTextures[0]);
        glUniform1f(m_PostTime, time);
        SubmitDrawArrays(GL_TRIANGLES, 0, 3, DrawCategory::PostProcess);
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_BLEND);
    }
    GpuTimestamp(2);
}

void Renderer::Upload(const std::vector<Vertex>& v)
{
    if (v.empty())
        return;
    Performance::Scope scope("cpu.render.stream_upload_submit_ms");
    Performance::Profiler::Get().Count("upload.stream_bytes", double(v.size() * sizeof(Vertex)));
    Performance::Profiler::Get().Count("render.stream_vertices", double(v.size()));
    glUseProgram(m_Shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_Font->texture);
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_Buffer);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(Vertex), v.data(), GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(4 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(8 * sizeof(float)));
    DrawCategory category = m_StreamKinds == 1   ? DrawCategory::Geometry
                            : m_StreamKinds == 2 ? DrawCategory::Text
                            : m_StreamKinds == 4 ? DrawCategory::Effect
                                                 : DrawCategory::Mixed;
    SubmitDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(v.size()), category);
}

void Renderer::Flush()
{
    if (!m_Vertices.empty())
    {
        Upload(m_Vertices);
        m_Vertices.clear();
        m_StreamKinds = 0;
    }
    FlushModels();
}

void Renderer::BeginStream()
{
    if (!m_Instances.empty())
    {
        Performance::Profiler::Get().Count("render.batch_break.model_to_stream");
        FlushModels();
    }
}

void Renderer::DrawModel(const std::vector<ModelVertex>& vertices, float x, float y, float scaleX,
                         float scaleY)
{
    if (vertices.empty())
    {
        return;
    }
    Performance::Scope scope("cpu.render.model_enqueue_ms");
    if (!m_Vertices.empty())
    {
        Performance::Profiler::Get().Count("render.batch_break.stream_to_model");
        Flush();
    }
    const void* key = vertices.data();
    auto found = m_ModelCache.find(key);
    if (found == m_ModelCache.end())
    {
        if (m_ModelData.size() / 4 + vertices.size() * 2 > size_t(m_MaxModelTexels))
        {
            // Respect the implementation's texture-buffer limit without dropping an object.
            BeginStream();
            m_StreamKinds |= 1;
            for (const auto& vertex : vertices)
            {
                m_Vertices.push_back({(x + vertex.x * scaleX) * 2.f / m_Width - 1.f,
                                      1.f - (y + vertex.y * scaleY) * 2.f / m_Height, 0, 0,
                                      vertex.r, vertex.g, vertex.b, vertex.a});
            }
            Performance::Profiler::Get().Count("render.model_cache_capacity_fallbacks");
            return;
        }
        MeshRange range = {static_cast<int>(m_ModelData.size() / 8),
                           static_cast<int>(vertices.size())};
        for (const auto& vertex : vertices)
        {
            const float data[] = {vertex.x, vertex.y, vertex.r, vertex.g, vertex.b, vertex.a, 0, 0};
            m_ModelData.insert(m_ModelData.end(), data, data + 8);
        }
        found = m_ModelCache.emplace(key, range).first;
        m_ModelDataDirty = true;
        Performance::Profiler::Get().Count("render.model_cache_misses");
    }
    else
    {
        Performance::Profiler::Get().Count("render.model_cache_hits");
    }
    const MeshRange& range = found->second;
    m_Instances.push_back({x, y, scaleX, scaleY, float(range.first), float(range.count)});
    m_MaxInstanceVertices = (std::max)(m_MaxInstanceVertices, range.count);
    Performance::Profiler::Get().Count("render.model_instances");
    Performance::Profiler::Get().Count("render.model_useful_vertices", range.count);
}

void Renderer::FlushModels()
{
    if (m_Instances.empty())
    {
        return;
    }
    Performance::Scope scope("cpu.render.model_upload_submit_ms");
    glUseProgram(m_ModelShader);
    glBindVertexArray(m_ModelVAO);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, m_ModelTexture);
    if (m_ModelDataDirty)
    {
        glBindBuffer(GL_TEXTURE_BUFFER, m_ModelBuffer);
        glBufferData(GL_TEXTURE_BUFFER, m_ModelData.size() * sizeof(float), m_ModelData.data(),
                     GL_STATIC_DRAW);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_ModelBuffer);
        Performance::Profiler::Get().Count("upload.model_cache_bytes",
                                           double(m_ModelData.size() * sizeof(float)));
        m_ModelDataDirty = false;
    }
    glBindBuffer(GL_ARRAY_BUFFER, m_InstanceBuffer);
    glBufferData(GL_ARRAY_BUFFER, m_Instances.size() * sizeof(ModelInstance), m_Instances.data(),
                 GL_STREAM_DRAW);
    Performance::Profiler::Get().Count("upload.instance_bytes",
                                       double(m_Instances.size() * sizeof(ModelInstance)));
    Performance::Profiler::Get().Count("render.model_submitted_vertices",
                                       double(m_MaxInstanceVertices) * m_Instances.size());
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(ModelInstance), nullptr);
    glVertexAttribDivisor(0, 1);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ModelInstance),
                          reinterpret_cast<void*>(4 * sizeof(float)));
    glVertexAttribDivisor(1, 1);
    glUniform2f(m_ModelViewport, float(m_Width), float(m_Height));
    glDrawArraysInstanced(GL_TRIANGLES, 0, m_MaxInstanceVertices,
                          static_cast<GLsizei>(m_Instances.size()));
    if (m_FrameActive)
    {
        ++m_FrameStats.geometry;
    }
    glActiveTexture(GL_TEXTURE0);
    m_Instances.clear();
    m_MaxInstanceVertices = 0;
}

void Renderer::Effect(float x, float y, float width, float height, float time, int kind,
                      float phase)
{
    BeginStream();
    m_StreamKinds |= 4;
    Performance::Profiler::Get().Count("render.effect_requests");
    float left = x * 2.f / m_Width - 1.f, top = 1.f - y * 2.f / m_Height;
    float w = width * 2.f / m_Width, h = -height * 2.f / m_Height;
    Vertex q[] = {{left, top, 0, 0, 1, 1, 1, 1, float(kind + 2), time, phase},
                  {left + w, top, 1, 0, 1, 1, 1, 1, float(kind + 2), time, phase},
                  {left + w, top + h, 1, 1, 1, 1, 1, 1, float(kind + 2), time, phase},
                  {left, top + h, 0, 1, 1, 1, 1, 1, float(kind + 2), time, phase}};
    for (int index : {0, 1, 2, 0, 2, 3})
    {
        m_Vertices.push_back(q[index]);
    }
    if (m_Vertices.size() >= 65536)
    {
        Performance::Profiler::Get().Count("render.batch_break.vertex_limit");
        Flush();
    }
}

void Renderer::GpuTimestamp(int index)
{
    if (m_ActiveGpuQuery >= 0)
    {
        glQueryCounter(m_GpuQueries[m_ActiveGpuQuery].timestamps[index], GL_TIMESTAMP);
    }
}

void Renderer::PollGpuQueries()
{
    Performance::Scope scope("cpu.profiler.gpu_poll_ms");
    for (auto& query : m_GpuQueries)
    {
        if (!query.pending)
        {
            continue;
        }
        GLint ready = 0;
        glGetQueryObjectiv(query.timestamps[3], GL_QUERY_RESULT_AVAILABLE, &ready);
        if (!ready)
        {
            continue;
        }
        GLuint64 stamps[4] = {};
        for (int i = 0; i < 4; ++i)
        {
            glGetQueryObjectui64v(query.timestamps[i], GL_QUERY_RESULT, &stamps[i]);
        }
        auto& profiler = Performance::Profiler::Get();
        profiler.Sample("gpu.timeline.world_ms", double(stamps[1] - stamps[0]) / 1000000.0);
        profiler.Sample("gpu.timeline.post_ms", double(stamps[2] - stamps[1]) / 1000000.0);
        profiler.Sample("gpu.timeline.ui_ms", double(stamps[3] - stamps[2]) / 1000000.0);
        profiler.Sample("gpu.timeline.frame_ms", double(stamps[3] - stamps[0]) / 1000000.0);
        profiler.Count("gpu.completed_samples");
        profiler.Gauge("gpu.last_sample_source_frame", double(query.frame));
        profiler.Count("gpu.sample_age_frames", double(m_FrameStats.frame - query.frame));
        query.pending = false;
    }
}

void Renderer::Triangle(float x1, float y1, float x2, float y2, float x3, float y3, float r,
                        float g, float b, float a)
{
    BeginStream();
    m_StreamKinds |= 1;
    m_Vertices.push_back({x1 * 2 / m_Width - 1, 1 - y1 * 2 / m_Height, 0, 0, r, g, b, a});
    m_Vertices.push_back({x2 * 2 / m_Width - 1, 1 - y2 * 2 / m_Height, 0, 0, r, g, b, a});
    m_Vertices.push_back({x3 * 2 / m_Width - 1, 1 - y3 * 2 / m_Height, 0, 0, r, g, b, a});
    if (m_Vertices.size() >= 65536)
    {
        Performance::Profiler::Get().Count("render.batch_break.vertex_limit");
        Flush();
    }
}

void Renderer::DrawSolidRect(float x, float y, float z, float size, float r, float g, float b,
                             float a)
{
    (void)z;
    float sx = m_Width * .5f + x, sy = m_Height * .5f - y, h = size * .5f;
    Triangle(sx - h, sy - h, sx + h, sy - h, sx + h, sy + h, r, g, b, a);
    Triangle(sx - h, sy - h, sx + h, sy + h, sx - h, sy + h, r, g, b, a);
}

void Renderer::Text(float x, float baseline, const std::string& utf8, float size, float r, float g,
                    float b, float a)
{
    Performance::Scope scope("cpu.render.text_layout_ms");
    if (!m_Font || !m_Font->texture || utf8.empty())
        return;
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(),
                                    static_cast<int>(utf8.size()), nullptr, 0);
    if (count <= 0)
        return;
    std::wstring text(count, L' ');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), &text[0], count);
    BeginStream();
    m_StreamKinds |= 2;
    Performance::Profiler::Get().Count("render.text_requests");
    Performance::Profiler::Get().Count("render.text_glyphs", count);
    float scale = size / 28.f, start = x;
    for (wchar_t ch : text)
    {
        if (ch == L'\n')
        {
            x = start;
            baseline += size * 1.5f;
            continue;
        }
        auto glyph = m_Font->Get(ch);
        float u = float(glyph.slot % Font::Columns * Font::Cell) / Font::Atlas;
        float v = float(glyph.slot / Font::Columns * Font::Cell) / Font::Atlas;
        float span = float(Font::Cell) / Font::Atlas;
        float left = (x - 2 * scale) * 2 / m_Width - 1, right = (x + 38 * scale) * 2 / m_Width - 1;
        float top = 1 - (baseline - size - 2 * scale) * 2 / m_Height,
              bottom = top - 40 * scale * 2 / m_Height;
        Vertex q[4] = {{left, top, u, v, r, g, b, a},
                       {right, top, u + span, v, r, g, b, a},
                       {right, bottom, u + span, v + span, r, g, b, a},
                       {left, bottom, u, v + span, r, g, b, a}};
        for (int i : {0, 1, 2, 0, 2, 3})
        {
            q[i].kind = 1;
            m_Vertices.push_back(q[i]);
        }
        x += (glyph.advance + 1) * scale;
    }
    if (m_Vertices.size() >= 65536)
    {
        Performance::Profiler::Get().Count("render.batch_break.vertex_limit");
        Flush();
    }
}
