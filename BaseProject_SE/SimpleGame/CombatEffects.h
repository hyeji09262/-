#pragma once

#include "PlayerActions.h"
#include "Renderer.h"
#include "SceneGraph.h"
#include <cmath>

namespace CombatEffects
{
struct Color
{
    float r, g, b;
};

template <typename Point>
void Spark(Renderer& renderer, Point p, float width, float height, Color color, float alpha)
{
    renderer.Triangle(p.x - width, p.y, p.x, p.y - height, p.x + width, p.y,
                      color.r, color.g, color.b, alpha);
    renderer.Triangle(p.x - width, p.y, p.x + width, p.y, p.x, p.y + height,
                      color.r, color.g, color.b, alpha);
}

template <typename Project>
void Recovery(Game::SceneGraph& scene, Renderer& renderer, Game::Actor& hero,
              const PlayerActions& actions, float elevation, float scale, Project project)
{
    if (actions.recoveryFlash <= 0)
    {
        return;
    }
    const float fade = actions.recoveryFlash / PlayerActions::RecoveryDuration;
    const Color tint = actions.manaRecovery ? Color{.46f, .80f, 1.f} : Color{.53f, 1.f, .67f};
    scene.Place("hero/recovery", {0, 0, elevation + 27}, Game::RenderLayer::Overlay,
                [&renderer, project, scale, fade, tint](const Game::Actor&, const Game::Transform& p)
                {
                    const auto center = project(p);
                    for (int i = 0; i < 8; ++i)
                    {
                        const float angle = i * .78539816f + (1.f - fade) * 1.8f;
                        auto point = center;
                        point.x += std::cos(angle) * 33 * scale;
                        point.y += (std::sin(angle) * 15 - (1.f - fade) * 38) * scale;
                        Spark(renderer, point, 5 * scale, 9 * scale, tint, fade);
                    }
                },
                &hero);
}

template <typename Project>
void LevelUp(Game::SceneGraph& scene, Renderer& renderer, Game::Actor& hero, float remaining,
             float duration, float elevation, float scale, Project project)
{
    if (remaining <= 0)
    {
        return;
    }

    const float age = 1.f - remaining / duration;
    const float fade = (std::min)(1.f, remaining / .35f);
    scene.Place("hero/level_up_stars", {0, 0, elevation + 32.f}, Game::RenderLayer::Overlay,
                [&renderer, project, scale, age, fade](const Game::Actor&,
                                                        const Game::Transform& p)
                {
                    const auto center = project(p);
                    const float radius = (24.f + age * 54.f) * scale;
                    const Color gold{1.f, .82f, .35f};
                    const Color lilac{.83f, .66f, 1.f};

                    // A quick central flash makes the level change readable immediately.
                    const float flash = (std::max)(0.f, 1.f - age * 5.f);
                    Spark(renderer, center, (14.f + flash * 23.f) * scale,
                          (18.f + flash * 27.f) * scale, gold, flash * .65f);

                    for (int i = 0; i < 12; ++i)
                    {
                        const float angle = i * .52359878f + age * .9f;
                        auto star = center;
                        star.x += std::cos(angle) * radius;
                        star.y += (std::sin(angle) * radius * .48f - age * 26.f * scale);
                        Spark(renderer, star, (i % 3 == 0 ? 7.f : 4.f) * scale,
                              (i % 3 == 0 ? 12.f : 7.f) * scale,
                              i % 2 == 0 ? gold : lilac, fade * (1.f - age * .35f));
                    }

                    for (int i = 0; i < 6; ++i)
                    {
                        auto rising = center;
                        rising.x += std::sin(i * 2.3999632f) * (12.f + i * 6.f) * scale;
                        rising.y -= (age * (66.f + i * 9.f) + i * 4.f) * scale;
                        Spark(renderer, rising, 3.f * scale, 6.f * scale, lilac, fade * .75f);
                    }
                },
                &hero);
}

template <typename Combat, typename Project>
void Spells(Game::SceneGraph& scene, Renderer& renderer, const Combat& combat, float scale,
            Project project)
{
    if (combat.burst > 0)
    {
        const float fade = combat.burst / .65f;
        scene.Place("magic/star_burst", {combat.burstOrigin.x, combat.burstOrigin.y, 2},
                    Game::RenderLayer::Shadow,
                    [&renderer, project, scale, fade](const Game::Actor&, const Game::Transform& p)
                    {
                        const float radius = 3.f - 2.f * fade;
                        const Color gold{1.f, .82f, .38f};
                        for (int i = 0; i < 16; ++i)
                        {
                            const float angle = i * .39269908f;
                            const auto point = project(Game::Transform{p.x + radius * std::cos(angle),
                                                                      p.y + radius * std::sin(angle),
                                                                      p.z});
                            Spark(renderer, point, 11 * scale, 17 * scale, gold, fade);
                        }
                    });
    }
    if (combat.starfall > 0)
    {
        const float fade = combat.starfall / .65f;
        scene.Place("magic/starfall", {combat.starfallOrigin.x, combat.starfallOrigin.y, 4},
                    Game::RenderLayer::Overlay,
                    [&renderer, project, scale, fade](const Game::Actor&, const Game::Transform& p)
                    {
                        const auto center = project(p);
                        const Color violet{.75f, .65f, 1.f};
                        Spark(renderer, center, 54 * scale, 24 * scale, violet, fade * .7f);
                        for (int i = -2; i <= 2; ++i)
                        {
                            auto falling = center;
                            falling.x += i * 19 * scale;
                            falling.y -= (fade * 115 + std::abs(i) * 9) * scale;
                            Spark(renderer, falling, 7 * scale, 23 * scale, violet, fade);
                        }
                    });
    }
}
} // namespace CombatEffects
