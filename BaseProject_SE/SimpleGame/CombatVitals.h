#pragma once

#include "Renderer.h"
#include "RpgProgress.h"

namespace CombatVitals
{
inline void Draw(Renderer& renderer, int width, int height, float health, float mana,
                 const Rpg::Progress& progress)
{
    const float scale = (std::min)(width / 1280.f, height / 800.f);
    const float offsetX = (width - 1280 * scale) * .5f;
    const float offsetY = (height - 800 * scale) * .5f;
    const auto panel = [&](float x, float y, float w, float h, float r, float g, float b, float a)
    {
        const float left = offsetX + x * scale, right = left + w * scale;
        const float top = offsetY + y * scale, bottom = top + h * scale;
        renderer.Triangle(left, top, right, top, right, bottom, r, g, b, a);
        renderer.Triangle(left, top, right, bottom, left, bottom, r, g, b, a);
    };
    const auto text = [&](float x, float y, const std::string& label, float size)
    {
        renderer.Text(offsetX + x * scale, offsetY + y * scale, label, size * scale,
                      .95f, .95f, 1.f, 1.f);
    };
    const auto meter = [&](float x, const char* name, float value, int maximum, char key, int count,
                           float r, float g, float b)
    {
        const float ratio = (std::max)(0.f, (std::min)(1.f, value / (std::max)(1, maximum)));
        text(x - 7, 231, name, 18);
        panel(x, 243, 22, 280, .2f, .22f, .3f, 1.f);
        panel(x, 243 + 280 * (1 - ratio), 22, 280 * ratio, r, g, b, 1.f);
        text(x - 10, 542, std::to_string(int(value)) + "/" + std::to_string(maximum), 14);
        text(x - 14, 560, std::string("[") + key + "] " + std::to_string(count) + "개", 15);
    };

    panel(22, 212, 152, 354, .06f, .08f, .15f, .96f);
    meter(48, "체력", health, progress.stats.maxHealth, 'U', progress.potions, .95f, .40f, .63f);
    meter(112, "마나", mana, progress.stats.maxMana, 'I', progress.manaPotions, .52f, .70f, 1.f);
}
} // namespace CombatVitals
