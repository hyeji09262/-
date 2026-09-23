#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include <array>
#include <unordered_map>
#include "Profiler.h"
#include "Dependencies/glew.h"

// Batched 2D geometry, cached Unicode glyph atlas, and a world-only post pass.
class Renderer
{
  public:
    struct FrameStats
    {
        std::uint64_t frame = 0;
        std::uint64_t geometry = 0;
        std::uint64_t text = 0;
        std::uint64_t effects = 0;
        std::uint64_t postProcess = 0;
        std::uint64_t mixed = 0;

        std::uint64_t Total() const
        {
            return geometry + text + effects + postProcess + mixed;
        }
    };

    struct ModelVertex
    {
        float x, y, r, g, b, a;
    };

    Renderer(int width, int height);
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool IsInitialized() const
    {
        return m_Initialized;
    }

    void Resize(int width, int height);
    void BeginFrame();
    void EndFrame();
    void DrawFrameStats();
    void Nameplate(float centerX, float baseline, const std::string& label, float size);

    const FrameStats& LastFrameStats() const
    {
        return m_LastFrameStats;
    }

    void BeginWorld();
    void EndWorld(float time);
    void Flush();
    void DrawModel(const std::vector<ModelVertex>& vertices, float x, float y, float scaleX,
                   float scaleY);
    void Effect(float x, float y, float width, float height, float time, int kind,
                float phase = 0.f);
    void Triangle(float x1, float y1, float x2, float y2, float x3, float y3, float r, float g,
                  float b, float a = 1.f);
    void DrawSolidRect(float x, float y, float z, float size, float r, float g, float b, float a);
    void Text(float x, float baseline, const std::string& utf8, float size, float r, float g,
              float b, float a = 1.f);

  private:
    enum class DrawCategory
    {
        Geometry,
        Text,
        Effect,
        PostProcess,
        Mixed
    };

    FrameStats m_FrameStats;
    FrameStats m_LastFrameStats;
    bool m_FrameActive = false;
    void SubmitDrawArrays(GLenum mode, GLint first, GLsizei count, DrawCategory category);

    struct Vertex
    {
        float x, y, u, v, r, g, b, a;
        float kind = 0, time = 0, phase = 0;
    };
    struct Font;
    Font* m_Font = nullptr;
    std::vector<Vertex> m_Vertices;
    int m_Width = 1, m_Height = 1;
    GLuint m_Buffer = 0, m_VAO = 0, m_Shader = 0, m_Post = 0, m_Framebuffer = 0, m_Scene = 0;
    GLuint m_ModelShader = 0, m_ModelVAO = 0, m_ModelBuffer = 0, m_ModelTexture = 0;
    GLuint m_InstanceBuffer = 0, m_BloomShader = 0;
    GLuint m_BloomBuffers[2] = {}, m_BloomTextures[2] = {};
    GLint m_ModelViewport = -1, m_PostTime = -1;
    GLint m_BloomPass = -1, m_BloomTexel = -1;
    int m_BloomWidth = 1, m_BloomHeight = 1;

    struct MeshRange
    {
        int first;
        int count;
    };

    struct ModelInstance
    {
        float x, y, scaleX, scaleY, first, count;
    };

    std::unordered_map<const void*, MeshRange> m_ModelCache;
    std::vector<float> m_ModelData;
    std::vector<ModelInstance> m_Instances;
    int m_MaxInstanceVertices = 0;
    GLint m_MaxModelTexels = 0;
    bool m_ModelDataDirty = false;
    unsigned int m_StreamKinds = 0;

    struct GpuQuery
    {
        GLuint timestamps[4] = {};
        bool pending = false;
        std::uint64_t frame = 0;
    };

    std::array<GpuQuery, 8> m_GpuQueries;
    int m_ActiveGpuQuery = -1;
    double m_FrameIntervalMs = 0;
    Performance::Clock::time_point m_PreviousFrameStart = Performance::Clock::now();
    void BeginStream();
    void FlushModels();
    void PollGpuQueries();
    void GpuTimestamp(int index);
    bool m_Initialized = false, m_TargetValid = false;
    bool ReadFile(const char* name, std::string& out);
    GLuint CompileShaders(const char* vertex, const char* fragment);
    void Upload(const std::vector<Vertex>& vertices);
};
