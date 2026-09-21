#include "stdafx.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include "PrimitiveModels.h"
#include "RuntimeFiles.h"
#include "CharacterVisual.h"

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

void Star(Mesh& mesh, float x, float y, float radius, Color color)
{
    for (int i = 0; i < 10; ++i)
    {
        float a = -1.5707963f + i * .6283185f;
        float b = a + .6283185f;
        float r1 = i % 2 == 0 ? radius : radius * .45f;
        float r2 = i % 2 == 0 ? radius * .45f : radius;
        Triangle(mesh, x, y, x + std::cos(a) * r1, y + std::sin(a) * r1, x + std::cos(b) * r2,
                 y + std::sin(b) * r2, color);
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
    const auto path = RuntimeFiles::Path(L"primitive_models_v5.bin");
    if (path.empty())
    {
        return false;
    }
    std::ifstream file(path.c_str(), std::ios::binary);
    char magic[8] = {};
    file.read(magic, sizeof(magic));
    if (!file || std::memcmp(magic, "EWMDL005", 8) != 0)
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
    const auto path = RuntimeFiles::Path(L"primitive_models_v5.bin");
    if (path.empty())
    {
        return false;
    }
    const auto temporary = path + L".tmp";
    {
        std::ofstream file(temporary.c_str(), std::ios::binary | std::ios::trunc);
        file.write("EWMDL005", 8);
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
    const Color robe = {.96f, .32f, .62f, 1}, dark = {.27f, .20f, .45f, 1};
    const Color gold = {1, .86f, .48f, 1}, skin = {1, .81f, .73f, 1};
    const Color white = {1, .95f, .98f, 1};
    const Color hair = {.57f, .32f, .68f, 1};
    for (int frame = 0; frame < 6; ++frame)
    {
        auto& mesh = m_Models[frame];
        float step = frame >= 1 && frame <= 4 ? std::sin((frame - 1) * 1.570796f) * 4 : 0;
        float hand = frame == 5 ? CharacterVisual::CastHandY : CharacterVisual::IdleHandY;
        // Twin tails, pleated skirt, ribbons and boots share all six cached poses.
        Ellipse(mesh, -13 - step * .3f, -38, 6, 18, hair);
        Ellipse(mesh, 13 + step * .3f, -38, 6, 18, hair);
        Ellipse(mesh, -14 - step * .3f, -41, 2, 12, {.78f, .51f, .83f, 1});
        Ellipse(mesh, 12 + step * .3f, -41, 2, 12, {.78f, .51f, .83f, 1});
        Rect(mesh, -8 + step, -14, 6, 14, white);
        Rect(mesh, 3 - step, -14, 6, 14, white);
        Rect(mesh, -8 + step, -8, 6, 2, robe);
        Rect(mesh, 3 - step, -8, 6, 2, robe);
        Ellipse(mesh, -6 + step, -2, 4, 2, dark);
        Ellipse(mesh, 6 - step, -2, 4, 2, dark);
        Triangle(mesh, -9, -38, 9, -38, 14, -10, robe);
        Triangle(mesh, -9, -38, 14, -10, -13, -10, robe);
        Rect(mesh, -8, -38, 16, 15, white);
        Triangle(mesh, -8, -24, -13, -10, -2, -10, {.73f, .23f, .53f, 1});
        for (int pleat = -6; pleat <= 6; pleat += 4)
        {
            Triangle(mesh, float(pleat), -24, float(pleat + 1), -11, float(pleat + 4), -11,
                     {1, .54f, .74f, 1});
        }
        Rect(mesh, -13, -12, 27, 3, white);
        for (int lace = -11; lace <= 11; lace += 4)
        {
            Ellipse(mesh, float(lace), -10, 2, 1.5f, white);
        }
        Triangle(mesh, -8, -38, 0, -30, -2, -38, {.67f, .66f, .86f, 1});
        Triangle(mesh, 8, -38, 0, -30, 2, -38, {.67f, .66f, .86f, 1});
        Rect(mesh, -8, -24, 16, 2, gold);
        Triangle(mesh, 0, -33, -9, -38, -9, -29, robe);
        Triangle(mesh, 0, -33, 9, -38, 9, -29, robe);
        Ellipse(mesh, 0, -33, 3, 3, gold);
        Rect(mesh, 9, hand, 10, 6, white);
        Rect(mesh, 14, hand, 2, 6, robe);
        Ellipse(mesh, -11, -32, 4, 5, white);
        Rect(mesh, -14, -29, 5, 8, skin);
        Rect(mesh, -14, -23, 5, 4, white);
        Ellipse(mesh, 18, hand + 3, 3, 3, skin);
        Ellipse(mesh, 0, -45, 8, 10, skin);
        Ellipse(mesh, 0, -53, 9, 5, hair);
        Ellipse(mesh, -3, -55, 5, 1.5f, {.83f, .60f, .88f, 1});
        Triangle(mesh, -9, -53, -2, -53, -7, -44, hair);
        Rect(mesh, -5, -46, 3, 4, dark);
        Rect(mesh, 3, -46, 3, 4, dark);
        Rect(mesh, -5, -46, 1, 1, white);
        Rect(mesh, 3, -46, 1, 1, white);
        Ellipse(mesh, -6, -41, 2, 1, {1, .57f, .66f, .7f});
        Ellipse(mesh, 6, -41, 2, 1, {1, .57f, .66f, .7f});
        Rect(mesh, -1, -39, 2, 1, {.66f, .33f, .44f, 1});
        Triangle(mesh, -12, -52, -20, -59, -20, -48, robe);
        Triangle(mesh, 12, -52, 20, -59, 20, -48, robe);
        Rect(mesh, 19, hand - 22, 3, 40, white);
        Rect(mesh, 19, hand - 4, 3, 4, robe);
        float tipY = hand + CharacterVisual::WandHeadOffset;
        Star(mesh, CharacterVisual::WandX, tipY, 11, {.66f, .40f, .24f, 1});
        Star(mesh, CharacterVisual::WandX, tipY, 9, gold);
        Ellipse(mesh, CharacterVisual::WandX, tipY, 3, 3, robe);
        Ellipse(mesh, CharacterVisual::WandX - 1, tipY - 1, 1, 1, white);
        Triangle(mesh, 19, hand - 15, 12, hand - 9, 17, hand - 3, robe);
        Triangle(mesh, 22, hand - 15, 29, hand - 10, 24, hand - 2, robe);
    }

    for (int kind = 0; kind < 2; ++kind)
    {
        for (int frame = 0; frame < 4; ++frame)
        {
            auto& mesh = m_Models[static_cast<int>(ModelId::Slime0) + kind * 4 + frame];
            float bob = std::sin(frame * 1.570796f) * 2;
            if (kind == 0)
            {
                Ellipse(mesh, 0, -13 + bob, 18 + bob, 14 - bob, {.48f, .39f, .72f, 1});
                Ellipse(mesh, -6, -20 + bob, 6, 3, {.86f, .68f, .94f, .7f});
            }
            else
            {
                Triangle(mesh, -17, 0, 0, -43 + bob, 17, 0, {.40f, .31f, .48f, 1});
                Triangle(mesh, -13, -28, -17, -47 + bob, -1, -32, {.59f, .43f, .54f, 1});
                Ellipse(mesh, -9, -39 + bob, 5, 17, hair);
                Ellipse(mesh, 9, -39 + bob, 5, 17, hair);
                Ellipse(mesh, 0, -21 + bob, 15, 16, dark);
                Rect(mesh, -14, -4 + bob, 5, 5, dark);
                Rect(mesh, 9, -4 - bob, 5, 5, dark);
            }
            Rect(mesh, -9, -18 + bob, 5, 3, gold);
            Rect(mesh, 4, -18 + bob, 5, 3, gold);
        }
    }

    auto& tree = m_Models[static_cast<int>(ModelId::Tree)];
    Rect(tree, -4, -45, 8, 45, {.32f, .23f, .16f, 1});
    Triangle(tree, -9, 0, -2, -24, 0, 0, {.43f, .28f, .24f, 1});
    Triangle(tree, 0, 0, 3, -22, 12, 0, {.27f, .19f, .22f, 1});
    Rect(tree, -2, -40, 2, 34, {.57f, .37f, .30f, 1});
    Triangle(tree, -2, -28, -20, -49, -13, -28, {.35f, .23f, .22f, 1});
    Triangle(tree, 2, -30, 21, -58, 13, -31, {.40f, .27f, .23f, 1});
    for (int layer = 0; layer < 3; ++layer)
    {
        float w = 28.f - layer * 5;
        float y = -18.f - layer * 23;
        Ellipse(tree, 0, y - 15, w, 20, {.67f, .32f, .53f, 1});
        for (int cluster = 0; cluster < 5; ++cluster)
        {
            float angle = cluster * 1.256637f;
            float cx = std::cos(angle) * w * .60f;
            float cy = y - 17 + std::sin(angle) * 9;
            Ellipse(tree, cx, cy, 12, 11, {.88f, .49f + layer * .04f, .68f, 1});
            Ellipse(tree, cx - 3, cy - 4, 8, 6, {1, .73f, .83f, 1});
            Star(tree, cx - 4, cy - 5, 2.5f, {1, .91f, .89f, 1});
        }
    }
    for (int petal = 0; petal < 7; ++petal)
    {
        Ellipse(tree, -15.f + petal * 5, float(petal % 3), 2, 1, {.94f, .62f, .74f, .8f});
    }

    auto& rock = m_Models[static_cast<int>(ModelId::Rock)];
    Triangle(rock, -23, -3, -10, -26, 20, -3, {.37f, .42f, .45f, 1});
    Triangle(rock, -10, -26, 10, -24, 20, -3, {.52f, .55f, .53f, 1});
    Triangle(rock, -23, -3, -10, -26, -6, -12, {.47f, .50f, .58f, 1});
    Triangle(rock, -10, -26, 10, -24, -6, -12, {.68f, .68f, .73f, 1});
    Triangle(rock, -6, -12, 10, -24, 8, -4, {.53f, .53f, .64f, 1});
    Triangle(rock, 10, -24, 20, -3, 8, -4, {.30f, .33f, .43f, 1});
    Triangle(rock, -23, -3, -6, -12, 8, -4, {.36f, .38f, .47f, 1});
    Triangle(rock, -4, -21, -7, -14, -5, -8, {.27f, .30f, .38f, 1});
    Ellipse(rock, -14, -5, 6, 2, {.36f, .51f, .40f, 1});
    Ellipse(rock, 6, -4, 4, 1.5f, {.45f, .61f, .44f, 1});
    Ellipse(rock, -25, 0, 4, 2, {.54f, .54f, .63f, 1});
    Ellipse(rock, 23, 1, 3, 2, {.42f, .44f, .52f, 1});

    auto& shelter = m_Models[static_cast<int>(ModelId::Shelter)];
    // Modern school facade with flat roof, classroom windows and entrance clock.
    Rect(shelter, -49, -95, 98, 90, {.89f, .83f, .79f, 1});
    Rect(shelter, -53, -100, 106, 7, {.39f, .48f, .68f, 1});
    for (int y = -82; y < -20; y += 25)
    {
        for (int x = -41; x <= 31; x += 24)
        {
            Rect(shelter, float(x), float(y), 16, 17, {.40f, .68f, .84f, 1});
            Rect(shelter, float(x + 7), float(y), 2, 17, white);
            Rect(shelter, float(x), float(y + 16), 18, 2, {.57f, .55f, .65f, 1});
            Triangle(shelter, float(x + 1), float(y + 1), float(x + 6), float(y + 1), float(x + 1),
                     float(y + 10), {.79f, .90f, .96f, 1});
        }
    }
    Rect(shelter, -11, -28, 22, 23, dark);
    Rect(shelter, -16, -32, 32, 5, robe);
    Rect(shelter, -15, -5, 30, 3, {.70f, .68f, .74f, 1});
    Rect(shelter, -19, -2, 38, 3, {.57f, .56f, .66f, 1});
    Ellipse(shelter, 0, -109, 11, 11, white);
    Rect(shelter, -1, -117, 2, 8, dark);
    Rect(shelter, 0, -110, 6, 2, dark);

    auto& fire = m_Models[static_cast<int>(ModelId::Campfire)];
    Ellipse(fire, 0, -1, 17, 8, {.35f, .35f, .35f, 1});
    Ellipse(fire, 0, -4, 13, 6, {.92f, .63f, .89f, 1});

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
