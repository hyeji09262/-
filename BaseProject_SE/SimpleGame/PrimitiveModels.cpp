#include "stdafx.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include "PrimitiveModels.h"
#include "RuntimeFiles.h"

namespace Hunting
{
namespace
{
using Mesh = std::vector<Renderer::ModelVertex>;

struct Color
{
    float r, g, b, a;
};

void Triangle(Mesh& mesh, float x1, float y1, float x2, float y2, float x3, float y3, Color c)
{
    mesh.push_back({x1, y1, c.r, c.g, c.b, c.a});
    mesh.push_back({x2, y2, c.r, c.g, c.b, c.a});
    mesh.push_back({x3, y3, c.r, c.g, c.b, c.a});
}

void Rect(Mesh& mesh, float x, float y, float width, float height, Color color)
{
    Triangle(mesh, x, y, x + width, y, x + width, y + height, color);
    Triangle(mesh, x, y, x + width, y + height, x, y + height, color);
}

void Ellipse(Mesh& mesh, float x, float y, float width, float height, Color color)
{
    for (int i = 0; i < 20; ++i)
    {
        float a = i * 6.2831853f / 20;
        float b = (i + 1) * 6.2831853f / 20;
        Triangle(mesh, x, y, x + std::cos(a) * width, y + std::sin(a) * height,
                 x + std::cos(b) * width, y + std::sin(b) * height, color);
    }
}

std::uint32_t Hash(std::uint32_t value, const void* data, size_t size)
{
    const auto* bytes = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < size; ++i)
    {
        value = (value ^ bytes[i]) * 16777619u;
    }
    return value;
}
} // namespace

const std::vector<Renderer::ModelVertex>& PrimitiveModels::Get(ModelId id) const
{
    return m_Models[static_cast<int>(id)];
}

void PrimitiveModels::Initialize()
{
    if (m_Initialized)
    {
        return;
    }
    loadedFromFile = Load();
    if (!loadedFromFile)
    {
        Generate();
        storedToFile = Save();
    }
    m_Initialized = true;
}

bool PrimitiveModels::Load()
{
    const auto path = RuntimeFiles::Path(L"primitive_models_v3.bin");
    if (path.empty())
    {
        return false;
    }
    std::ifstream file(path.c_str(), std::ios::binary);
    char magic[8] = {};
    file.read(magic, sizeof(magic));
    if (!file || std::memcmp(magic, "EWMDL003", 8) != 0)
    {
        return false;
    }

    decltype(m_Models) candidate;
    std::uint32_t hash = 2166136261u;
    for (auto& mesh : candidate)
    {
        std::uint32_t count = 0;
        file.read(reinterpret_cast<char*>(&count), sizeof(count));
        if (!file || count == 0 || count > 20000 || count % 3 != 0)
        {
            return false;
        }
        mesh.resize(count);
        file.read(reinterpret_cast<char*>(mesh.data()), count * sizeof(mesh[0]));
        if (!file)
        {
            return false;
        }
        for (const auto& v : mesh)
        {
            if (!std::isfinite(v.x) || !std::isfinite(v.y) || std::abs(v.x) > 300 ||
                std::abs(v.y) > 300 || !std::isfinite(v.r) || !std::isfinite(v.g) ||
                !std::isfinite(v.b) || !std::isfinite(v.a) || v.r < 0 || v.r > 1 || v.g < 0 ||
                v.g > 1 || v.b < 0 || v.b > 1 || v.a < 0 || v.a > 1)
            {
                return false;
            }
        }
        hash = Hash(hash, &count, sizeof(count));
        hash = Hash(hash, mesh.data(), mesh.size() * sizeof(mesh[0]));
    }
    std::uint32_t storedHash = 0;
    file.read(reinterpret_cast<char*>(&storedHash), sizeof(storedHash));
    if (!file || storedHash != hash || file.peek() != std::ifstream::traits_type::eof())
    {
        return false;
    }
    m_Models = std::move(candidate);
    return true;
}

bool PrimitiveModels::Save() const
{
    const auto path = RuntimeFiles::Path(L"primitive_models_v3.bin");
    if (path.empty())
    {
        return false;
    }
    const auto temporary = path + L".tmp";
    {
        std::ofstream file(temporary.c_str(), std::ios::binary | std::ios::trunc);
        file.write("EWMDL003", 8);
        std::uint32_t hash = 2166136261u;
        for (const auto& mesh : m_Models)
        {
            std::uint32_t count = static_cast<std::uint32_t>(mesh.size());
            file.write(reinterpret_cast<const char*>(&count), sizeof(count));
            file.write(reinterpret_cast<const char*>(mesh.data()), mesh.size() * sizeof(mesh[0]));
            hash = Hash(hash, &count, sizeof(count));
            hash = Hash(hash, mesh.data(), mesh.size() * sizeof(mesh[0]));
        }
        file.write(reinterpret_cast<const char*>(&hash), sizeof(hash));
        file.flush();
        if (!file)
        {
            return false;
        }
    }
    return RuntimeFiles::Commit(temporary, path);
}

void PrimitiveModels::Generate()
{
    const Color robe = {.40f, .29f, .64f, 1}, dark = {.19f, .15f, .28f, 1};
    const Color gold = {1, .76f, .35f, 1}, skin = {.83f, .63f, .46f, 1};
    for (int frame = 0; frame < 6; ++frame)
    {
        auto& mesh = m_Models[frame];
        float step = frame >= 1 && frame <= 4 ? std::sin((frame - 1) * 1.570796f) * 4 : 0;
        float hand = frame == 5 ? -40.f : -27.f;
        Rect(mesh, -8 + step, -14, 6, 14, dark);
        Rect(mesh, 3 - step, -14, 6, 14, dark);
        Triangle(mesh, -9, -38, 9, -38, 14, -10, robe);
        Triangle(mesh, -9, -38, 14, -10, -13, -10, robe);
        Triangle(mesh, -9, -38, -13, -10, -2, -10, dark);
        Rect(mesh, -11, -21, 22, 3, gold);
        Rect(mesh, 9, hand, 10, 6, robe);
        Ellipse(mesh, 18, hand + 3, 3, 3, skin);
        Ellipse(mesh, 0, -45, 8, 10, skin);
        Rect(mesh, 3, -46, 2, 2, dark);
        Ellipse(mesh, 0, -54, 15, 4, dark);
        Triangle(mesh, -10, -54, -2, -79, 10, -54, robe);
        Rect(mesh, -8, -57, 16, 3, gold);
        Rect(mesh, 19, hand - 22, 3, 52, {.47f, .31f, .19f, 1});
        Ellipse(mesh, 20, hand - 25, 4, 6, {.25f, .93f, .84f, 1});
    }

    for (int kind = 0; kind < 2; ++kind)
    {
        for (int frame = 0; frame < 4; ++frame)
        {
            auto& mesh = m_Models[static_cast<int>(ModelId::Slime0) + kind * 4 + frame];
            float bob = std::sin(frame * 1.570796f) * 2;
            if (kind == 0)
            {
                Ellipse(mesh, 0, -13 + bob, 18 + bob, 14 - bob, {.33f, .52f, .32f, 1});
                Ellipse(mesh, -6, -20 + bob, 6, 3, {.60f, .73f, .45f, .7f});
            }
            else
            {
                Triangle(mesh, -17, 0, 0, -43 + bob, 17, 0, {.40f, .31f, .48f, 1});
                Triangle(mesh, -13, -28, -17, -47 + bob, -1, -32, {.59f, .43f, .54f, 1});
                Rect(mesh, -14, -4 + bob, 5, 5, dark);
                Rect(mesh, 9, -4 - bob, 5, 5, dark);
            }
            Rect(mesh, -9, -18 + bob, 5, 3, gold);
            Rect(mesh, 4, -18 + bob, 5, 3, gold);
        }
    }

    auto& tree = m_Models[static_cast<int>(ModelId::Tree)];
    Rect(tree, -4, -45, 8, 45, {.32f, .23f, .16f, 1});
    for (int layer = 0; layer < 3; ++layer)
    {
        float w = 28.f - layer * 5;
        float y = -18.f - layer * 23;
        Triangle(tree, -w, y, 0, y - 55, w, y, {.14f, .32f, .28f, 1});
        Triangle(tree, 0, y - 55, w, y, 3, y - 4, {.10f, .23f, .25f, 1});
    }

    auto& rock = m_Models[static_cast<int>(ModelId::Rock)];
    Triangle(rock, -23, -3, -10, -26, 20, -3, {.37f, .42f, .45f, 1});
    Triangle(rock, -10, -26, 10, -24, 20, -3, {.52f, .55f, .53f, 1});

    auto& shelter = m_Models[static_cast<int>(ModelId::Shelter)];
    Rect(shelter, -39, -59, 78, 54, {.52f, .41f, .31f, 1});
    for (int y = -55; y < -5; y += 10)
    {
        Rect(shelter, -39, float(y), 78, 2, {.28f, .21f, .16f, 1});
    }
    Triangle(shelter, -50, -58, 0, -105, 50, -58, {.43f, .28f, .25f, 1});
    Triangle(shelter, -50, -58, 0, -105, -3, -58, {.58f, .40f, .29f, 1});
    Rect(shelter, -9, -33, 18, 28, dark);
    Rect(shelter, -31, -43, 14, 16, gold);
    Rect(shelter, 17, -43, 14, 16, gold);

    auto& fire = m_Models[static_cast<int>(ModelId::Campfire)];
    Ellipse(fire, 0, -1, 17, 8, {.35f, .35f, .35f, 1});
    Rect(fire, -12, -5, 24, 5, {.28f, .17f, .1f, 1});

    auto& coin = m_Models[static_cast<int>(ModelId::Coin)];
    Ellipse(coin, 0, -7, 7, 5, gold);
    Ellipse(coin, 0, -8, 3, 2, {.94f, .90f, .54f, 1});

    auto& potion = m_Models[static_cast<int>(ModelId::Potion)];
    Ellipse(potion, 0, -8, 7, 8, {.87f, .25f, .37f, 1});
    Rect(potion, -3, -20, 6, 6, {.65f, .45f, .24f, 1});
    Ellipse(potion, -3, -11, 2, 3, {1, .74f, .78f, .8f});

    auto& crystal = m_Models[static_cast<int>(ModelId::Crystal)];
    Triangle(crystal, -7, -8, 0, -22, 7, -8, {.30f, .95f, .83f, 1});
    Triangle(crystal, -7, -8, 7, -8, 0, -1, {.19f, .58f, .68f, 1});

    auto& shadow = m_Models[static_cast<int>(ModelId::Shadow)];
    for (int layer = 5; layer >= 1; --layer)
    {
        Ellipse(shadow, -6, 3, 12.f + layer * 2, 3.f + layer, {.01f, .02f, .04f, .04f});
    }
}
} // namespace Hunting
