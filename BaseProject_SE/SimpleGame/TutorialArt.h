#pragma once

// Included after Tutorial's world state. Art remains procedural and asset-independent.
namespace Tutorial
{
inline Color Tint(Color c, float factor)
{
    return {c.r * factor, c.g * factor, c.b * factor, c.a};
}

inline Hunting::PrimitiveModels& VisualModels()
{
    static Hunting::PrimitiveModels models;
    models.Initialize();
    return models;
}

inline void CachedArt(Hunting::ModelId id, Vec screen, float scale = 1.f, bool mirror = false)
{
    screen = VisualPoint(screen);
    scale *= visualActive ? visualWorld.scale : 1.f;
    renderer->DrawModel(VisualModels().Get(id), screen.x * width / 1280, screen.y * height / 800,
                        scale * width / 1280 * (mirror ? -1.f : 1.f), scale * height / 800);
}

inline void Person(Vec p, Color cloak, bool wizard = false, int variant = 0)
{
    Vec s = Project(p);
    if (!Visible(s))
        return;
    if (wizard)
    {
        int frame = castPose > 0 ? 5 : moving ? 1 + int(walkCycle / 1.570796f) % 4 : 0;
        CachedArt(static_cast<Hunting::ModelId>(frame), s, 1.f,
                  castPose > 0 ? castMirror : facing < 0);
        if (hurtCD > 0)
        {
            Glow({s.x, s.y - 28}, 20, 32, {1, .65f, .85f, .22f});
        }
        return;
    }

    float walk = wizard && moving ? std::sin(walkCycle) * 4 : 0;
    float breath = std::sin(clockTime * 2 + variant) * .8f;
    float aim = wizard ? facing : variant % 2 ? 1.f : -1.f;
    float cast = wizard && castPose > 0 ? 1.f : 0;
    Color dark = Tint(cloak, .52f), skin = {.79f, .60f, .43f, 1};
    // Articulated boots, legs, sleeves, hands, belt, face and layered cloak.
    Line({s.x - 5, s.y - 17}, {s.x - 6 + walk, s.y - 3}, 7, dark);
    Line({s.x + 5, s.y - 17}, {s.x + 6 - walk, s.y - 3}, 7, dark);
    Box(s.x - 10 + walk, s.y - 5, 9, 5, {.14f, .12f, .16f, 1});
    Box(s.x + 2 - walk, s.y - 5, 9, 5, {.14f, .12f, .16f, 1});
    Quad({s.x - 9, s.y - 36 + breath}, {s.x + 8, s.y - 36 + breath}, {s.x + 13, s.y - 12},
         {s.x - 12, s.y - 12}, cloak);
    Tri({s.x - 9, s.y - 36 + breath}, {s.x - 13, s.y - 12}, {s.x - 1, s.y - 12}, dark);
    Tri({s.x + 1, s.y - 33}, {s.x + 10, s.y - 13}, {s.x + 4, s.y - 14}, Tint(cloak, 1.23f));
    Box(s.x - 10, s.y - 19, 20, 3, {.24f, .16f, .12f, 1});
    Box(s.x - 2, s.y - 20, 4, 5, gold);
    Vec hand = {s.x + aim * (15 + cast * 10), s.y - 23 - cast * 13};
    Line({s.x + aim * 7, s.y - 33 + breath}, hand, 7, dark);
    Ellipse(hand, 3, 4, skin, 10);
    Line({s.x - aim * 8, s.y - 32}, {s.x - aim * 13, s.y - 22 + walk * .6f}, 6, cloak);
    Ellipse({s.x, s.y - 43 + breath}, 8, 10, skin, 12);
    Box(s.x - 8, s.y - 51 + breath, 16, 5, {.21f, .15f, .14f, 1});
    Box(s.x + aim * 3, s.y - 44 + breath, 2, 2, ink);
    // Modern uniform collar and neck ribbon.
    Tri({s.x - 7, s.y - 35}, {s.x, s.y - 29}, {s.x + 7, s.y - 35}, {1, .94f, .97f, 1});
    Box(s.x - 2, s.y - 32, 4, 10, {.94f, .37f, .58f, 1});
    Box(s.x + 4, s.y - 29, 4, 4, {.91f, .83f, .58f, 1});
    Box(s.x - 7, s.y - 18, 14, 1, Tint(cloak, .7f));
    Ellipse({s.x - 3, s.y - 50 + breath}, 4, 1, {.38f, .26f, .26f, 1}, 10);
}

inline void Building(const Prop& p)
{
    Vec s = Project(p.p);
    CachedArt(Hunting::ModelId::Shelter, s, p.kind == 1 ? 1.2f : .85f);
    const char* label = p.kind == 1 ? "별빛 학교" : p.kind == 2 ? "도서관" : "동아리관";
    Text(s.x - 29, s.y - 17, label, ink, 12);
}

inline void DrawProp(const Prop& p)
{
    Vec s = Project(p.p);
    if (!Visible(s))
        return;
    if (p.kind >= 1 && p.kind <= 3)
    {
        Building(p);
        return;
    }
    if (p.kind == 0)
    {
        CachedArt(Hunting::ModelId::Tree, s);
    }
    else if (p.kind == 4)
    {
        Box(s.x - 10, s.y - 25, 20, 25, {.43f, .29f, .19f, 1});
        Ellipse({s.x, s.y - 25}, 10, 4, {.58f, .4f, .25f, 1});
        Box(s.x - 11, s.y - 21, 22, 3, {.2f, .23f, .26f, 1});
        Box(s.x - 11, s.y - 7, 22, 3, {.2f, .23f, .26f, 1});
        for (int i = -6; i < 9; i += 5)
            Line({s.x + i, s.y - 22}, {s.x + i, s.y - 2}, 1, {.25f, .17f, .12f, 1});
    }
    else if (p.kind == 5)
    {
        Box(s.x - 13, s.y - 24, 26, 24, {.50f, .35f, .23f, 1});
        Line({s.x - 12, s.y - 23}, {s.x + 12, s.y - 1}, 3, {.72f, .53f, .34f, 1});
        Line({s.x + 12, s.y - 23}, {s.x - 12, s.y - 1}, 3, {.72f, .53f, .34f, 1});
    }
    else if (p.kind == 6)
    {
        Box(s.x - 24, s.y - 22, 48, 17, {.36f, .25f, .18f, 1});
        for (int i = -1; i <= 1; i += 2)
        {
            Ellipse({s.x + i * 18, s.y - 3}, 8, 9, {.18f, .15f, .17f, 1});
            Line({s.x + i * 18 - 6, s.y - 3}, {s.x + i * 18 + 6, s.y - 3}, 2,
                 {.58f, .40f, .25f, 1});
        }
        Line({s.x + 22, s.y - 12}, {s.x + 48, s.y - 4}, 3, {.54f, .34f, .2f, 1});
        for (int i = 0; i < 5; ++i)
            Ellipse({s.x - 16 + i * 7, s.y - 26}, 5, 6, {.68f, .43f, .22f, 1});
    }
    else if (p.kind == 7)
    {
        Box(s.x - 2, s.y - 68, 4, 68, {.23f, .20f, .21f, 1});
        Line({s.x, s.y - 66}, {s.x + 14, s.y - 66}, 3, {.25f, .24f, .25f, 1});
        Glow({s.x + 12, s.y - 55}, 37, 42, {1, .59f, .18f, .37f});
        Box(s.x + 7, s.y - 63, 11, 16, {.95f, .66f, .24f, 1});
        Box(s.x + 6, s.y - 64, 13, 3, ink);
        Line({s.x + 12, s.y - 63}, {s.x + 12, s.y - 47}, 1, ink);
    }
    else if (p.kind == 8)
    {
        Ellipse({s.x, s.y - 4}, 22, 12, {.40f, .42f, .42f, 1});
        Box(s.x - 22, s.y - 19, 44, 15, {.44f, .44f, .40f, 1});
        Ellipse({s.x, s.y - 20}, 22, 11, {.63f, .60f, .51f, 1});
        Ellipse({s.x, s.y - 20}, 15, 7, {.12f, .24f, .28f, 1});
        Box(s.x - 22, s.y - 55, 4, 40, {.39f, .25f, .15f, 1});
        Box(s.x + 18, s.y - 55, 4, 40, {.39f, .25f, .15f, 1});
        Box(s.x - 23, s.y - 56, 46, 5, {.47f, .31f, .18f, 1});
        Line({s.x, s.y - 52}, {s.x, s.y - 23}, 1, gold);
    }
    else
    {
        CachedArt(Hunting::ModelId::Rock, s);
    }
}

inline void MonsterArt(const Monster& m)
{
    Vec s = Project(m.p);
    if (!Visible(s))
        return;
    float bob = std::sin(m.phase * 6) * 2;
    Color c = m.kind == 0 ? Color{.40f, .48f, .27f, 1} : Color{.37f, .31f, .46f, 1};
    if (m.kind == 0)
    {
        Ellipse({s.x, s.y - 12 + bob}, 17, 14 - bob * .6f, c);
        Ellipse({s.x - 6, s.y - 17 + bob}, 5, 3, {.67f, .74f, .41f, .5f});
    }
    else
    {
        Line({s.x - 9, s.y - 7}, {s.x - 15, s.y + 1 + bob}, 4, Tint(c, .7f));
        Line({s.x + 9, s.y - 7}, {s.x + 15, s.y + 1 - bob}, 4, Tint(c, .7f));
        Tri({s.x - 17, s.y - 4}, {s.x, s.y - 40 + bob}, {s.x + 17, s.y - 4}, c);
        Tri({s.x - 10, s.y - 28}, {s.x - 15, s.y - 43 + bob}, {s.x - 1, s.y - 33}, Tint(c, 1.3f));
    }
    Box(s.x - 8, s.y - 17 + bob, 4, 3, gold);
    Box(s.x + 4, s.y - 17 + bob, 4, 3, gold);
    if (m.hp < 3)
    {
        Box(s.x - 17, s.y - 47, 34, 4, ink);
        Box(s.x - 17, s.y - 47, 34 * m.hp / 3.f, 4, {.9f, .34f, .35f, 1});
    }
}

inline void BossArt()
{
    Vec s = Project(boss);
    if (!Visible(s))
        return;
    float bob = std::sin(clockTime * 2) * 2;
    Tri({s.x - 28, s.y}, {s.x, s.y - 86 + bob}, {s.x + 28, s.y}, {.21f, .18f, .25f, 1});
    Tri({s.x - 28, s.y}, {s.x, s.y - 86 + bob}, {s.x - 4, s.y - 6}, {.32f, .29f, .34f, 1});
    for (int side : {-1, 1})
    {
        Line({s.x + side * 16, s.y - 68}, {s.x + side * 26, s.y - 100}, 4, {.54f, .43f, .30f, 1});
        Line({s.x + side * 24, s.y - 91}, {s.x + side * 39, s.y - 95}, 3, {.54f, .43f, .30f, 1});
        Line({s.x + side * 17, s.y - 53}, {s.x + side * 34, s.y - 28}, 7, {.29f, .25f, .27f, 1});
        Glow({s.x + side * 9, s.y - 56 + bob}, 10, 7, {1, .15f, .23f, .5f});
        Box(s.x + side * 9 - 3, s.y - 58 + bob, 6, 3, {1, .53f, .38f, 1});
    }
    Diamond({s.x, s.y - 29}, 5, 9, {.78f, .40f, .70f, 1});
    // The nightmare headmaster still wears a tie and an oversized pair of spectacles.
    Line({s.x - 15, s.y - 58 + bob}, {s.x + 15, s.y - 58 + bob}, 2, gold);
    Ellipse({s.x - 9, s.y - 56 + bob}, 7, 5, {.80f, .74f, .92f, .28f});
    Ellipse({s.x + 9, s.y - 56 + bob}, 7, 5, {.80f, .74f, .92f, .28f});
    Tri({s.x - 6, s.y - 47}, {s.x, s.y - 39}, {s.x + 6, s.y - 47}, {.91f, .88f, .96f, 1});
}

inline void DrawMinimap()
{
    const float x = 1090, y = 88, scale = 2.5f;
    Box(x - 10, y - 8, 170, 146, {ink.r, ink.g, ink.b, .90f});
    Box(x, y, MapWidth * scale, MapHeight * scale, {.13f, .25f, .27f, 1});
    Box(x + LandWidth * scale, y, (MapWidth - LandWidth) * scale, MapHeight * scale,
        {.22f, .35f, .49f, 1});
    for (int yy = 0; yy < MapHeight; ++yy)
        for (int xx = 0; xx < LandWidth; ++xx)
            if (Path(xx, yy))
                Box(x + xx * scale, y + yy * scale, scale, scale, {.49f, .41f, .30f, 1});
    for (const auto& n : villagers)
        Box(x + n.p.x * scale, y + n.p.y * scale, 2, 2, {.73f, .74f, .70f, 1});
    for (const auto& m : monsters)
        if (m.hp > 0)
            Box(x + m.p.x * scale, y + m.p.y * scale, 2, 2, {.91f, .36f, .35f, 1});
    Vec target = Target();
    Diamond({x + target.x * scale, y + target.y * scale}, 4, 4, gold);
    Ellipse({x + player.x * scale, y + player.y * scale}, 3, 3, teal, 10);
}

// Legacy procedural shapes remain renderer primitives inside one scene actor per object.
// The affine screen adapter applies the actor's inherited world translation, height and scale.
inline Game::Actor& PlaceArt(const std::string& name, Game::Transform source,
                             Game::RenderLayer layer, std::function<void()> draw,
                             Game::Actor* parent = nullptr)
{
    return scene.Place(
        name, source, layer,
        [source, layer, draw](const Game::Actor&, const Game::Transform& world)
        {
            Vec screen = Project({world.x, world.y}, world.z);
            if (layer != Game::RenderLayer::Background &&
                layer != Game::RenderLayer::UserInterface && !Visible(screen, 240 * world.scale))
            {
                return;
            }
            visualSource = source;
            visualWorld = world;
            visualActive = true;
            draw();
            visualActive = false;
        },
        parent);
}

inline void DrawWorld()
{
    using Game::RenderLayer;
    auto& terrain = scene.Place("terrain", {}, RenderLayer::Ground);
    auto& scenery = scene.Place("scenery", {}, RenderLayer::World);
    auto& characters = scene.Place("characters", {}, RenderLayer::World);
    auto& effects = scene.Place("effects", {}, RenderLayer::Overlay);
    PlaceArt("background", {}, RenderLayer::Background,
             []()
             {
                 Box(0, 0, 1280, 800, {.10f, .14f, .18f, 1});
             });
    for (int y = 0; y < MapHeight; ++y)
    {
        for (int x = 0; x < MapWidth; ++x)
        {
            PlaceArt(
                "tile/" + std::to_string(y * MapWidth + x), {float(x), float(y)},
                RenderLayer::Ground,
                [x, y]()
                {
                    Vec s = Project({float(x), float(y)});
                    if (!Visible(s, 40))
                        return;
                    float n = ((x * 7 + y * 3) % 5) * .009f;
                    Color c = {.35f + n, .49f + n, .40f + n, 1};
                    if (x >= LandWidth)
                        c = {.14f + n * .5f, .27f + n, .35f + n, 1};
                    else if (x >= LandWidth - 2)
                        c = {.53f + n, .47f + n, .36f + n, 1};
                    else if (Path(x, y))
                        c = {.68f + n, .62f + n, .65f + n, 1};
                    Diamond(s, 32.4f, 16.4f, c);
                    if (x >= LandWidth)
                    {
                        float wave = std::sin(clockTime * 1.6f + x * .6f + y * .8f);
                        Line({s.x - 18, s.y + wave * 3}, {s.x + 8, s.y + wave * 3 - 2}, 1.4f,
                             {.53f, .74f, .77f, .15f + wave * .06f});
                        if (x == LandWidth)
                        {
                            float tide = std::sin(clockTime * 1.2f + y * .5f) * 4;
                            Line({s.x - 30 + tide, s.y}, {s.x + tide, s.y - 15}, 2.4f,
                                 {.80f, .86f, .79f, .50f});
                            Line({s.x - 27 + tide, s.y + 3}, {s.x + 3 + tide, s.y - 12}, 1,
                                 {.80f, .86f, .79f, .18f});
                        }
                    }
                    else if (Path(x, y))
                    {
                        for (int k = 0; k < 3; ++k)
                        {
                            float ox = ((x * 13 + y * 7 + k * 19) % 35) - 17.f,
                                  oy = ((x * 3 + y * 11 + k * 7) % 13) - 6.f;
                            Diamond({s.x + ox, s.y + oy}, 5, 2,
                                    {.57f + n, .49f + n, .37f + n, .65f});
                        }
                    }
                    else if (x < LandWidth - 2 && (x + y) % 3 == 0)
                    {
                        float breeze = std::sin(clockTime * 1.6f + x + y) * 2;
                        for (int k = 0; k < 3; ++k)
                            Line({s.x + k * 5 - 5, s.y + 2},
                                 {s.x + k * 5 - 6 + breeze, s.y - 5 - k % 2 * 3}, 1,
                                 {.36f, .44f, .27f, .75f});
                        if ((x * 3 + y) % 17 == 0)
                            Ellipse({s.x + 7, s.y - 3}, 2, 2, {.78f, .55f, .37f, 1}, 8);
                    }
                },
                &terrain);
        }
    }
    for (size_t i = 0; i < props.size(); ++i)
    {
        Prop prop = props[i];
        std::string name = "prop/" + std::to_string(i);
        auto& actor = PlaceArt(
            name, {prop.p.x, prop.p.y}, RenderLayer::World,
            [prop]()
            {
                DrawProp(prop);
            },
            &scenery);
        PlaceArt(
            name + "/shadow", {}, RenderLayer::Shadow,
            [prop]()
            {
                float size = prop.kind >= 1 && prop.kind <= 3 ? 42.f : prop.kind == 0 ? 19.f : 12.f;
                Shadow({0, 0}, size, size * .30f, prop.kind <= 3 ? 43.f : 12.f);
            },
            &actor);
    }
    for (int i = 0; i < NPCCount; ++i)
    {
        Vec p = villagers[i].p;
        auto& actor = PlaceArt(
            "npc/" + std::to_string(i), {p.x, p.y}, RenderLayer::World,
            [p, i]()
            {
                Person(p, {.38f + (i % 4) * .09f, .34f + (i % 3) * .07f, .40f, 1}, false, i);
            },
            &characters);
        PlaceArt(
            "npc/" + std::to_string(i) + "/shadow", {}, RenderLayer::Shadow,
            []()
            {
                Shadow({0, 0}, 12, 5, 15);
            },
            &actor);
    }
    auto& heroine = PlaceArt(
        "player", {player.x, player.y}, RenderLayer::World,
        []()
        {
            Person(player, hurtCD > 0 ? gold : Color{.43f, .34f, .68f, 1}, true);
        },
        &characters);
    PlaceArt(
        "player/shadow", {}, RenderLayer::Shadow,
        []()
        {
            Shadow({0, 0}, 13, 5, 18);
        },
        &heroine);
    bool mirror = castPose > 0 ? castMirror : facing < 0;
    scene.Place("player/wand", CharacterVisual::WandSocket(castPose > 0, mirror),
                RenderLayer::World, {}, &heroine);
    for (size_t i = 0; i < monsters.size(); ++i)
    {
        Monster monster = monsters[i];
        if (monster.hp <= 0)
        {
            continue;
        }
        std::string name = "monster/" + std::to_string(i);
        auto& actor = PlaceArt(
            name, {monster.p.x, monster.p.y}, RenderLayer::World,
            [monster]()
            {
                MonsterArt(monster);
            },
            &characters);
        PlaceArt(
            name + "/shadow", {}, RenderLayer::Shadow,
            []()
            {
                Shadow({0, 0}, 15, 6, 13);
            },
            &actor);
    }
    if (stage < 2)
    {
        auto& actor = PlaceArt(
            "boss", {boss.x, boss.y}, RenderLayer::World,
            []()
            {
                BossArt();
            },
            &characters);
        PlaceArt(
            "boss/shadow", {}, RenderLayer::Shadow,
            []()
            {
                Shadow({0, 0}, 25, 8, 30);
            },
            &actor);
        if (stage == 1)
        {
            PlaceArt(
                "boss/warning", {}, RenderLayer::Shadow,
                []()
                {
                    Color warning = {.95f, .20f, .30f, pulse > 2.1f ? .27f : .06f};
                    Vec center = Project({0, 0});
                    for (int i = 0; i < 40; ++i)
                    {
                        float a = i * 6.283185f / 40, b = (i + 1) * 6.283185f / 40;
                        Vec p = Project({std::cos(a) * 3, std::sin(a) * 3});
                        Vec q = Project({std::cos(b) * 3, std::sin(b) * 3});
                        Tri(center, p, q, warning);
                        if (i / 40.f < pulse / 3.2f)
                        {
                            Line(p, q, 2, {1, .40f, .33f, .75f});
                        }
                    }
                },
                &actor);
        }
    }
    PlaceArt(
        "shrine", {shrine.x, shrine.y}, RenderLayer::World,
        []()
        {
            Vec s = Project(shrine);
            for (int i = 0; i < 3; ++i)
            {
                Diamond({s.x, s.y - i * 5}, 30 - i * 4, 15 - i * 2,
                        {.34f + i * .07f, .35f + i * .07f, .40f + i * .06f, 1});
            }
            Box(s.x - 14, s.y - 32, 28, 21, {.40f, .43f, .48f, 1});
            Glow({s.x, s.y - 49}, 40, 52,
                 stage >= 4 ? Color{1, .6f, .2f, .32f} : Color{.5f, .4f, 1, .3f});
            Diamond({s.x, s.y - 49 + std::sin(clockTime * 2) * 3}, 9, 16, stage >= 4 ? gold : teal);
            if (stage < 4)
            {
                for (int i = 0; i < 5; ++i)
                {
                    float a = clockTime + i * 1.256f;
                    Diamond({s.x + std::cos(a) * 27, s.y - 39 + std::sin(a) * 15}, 3, 5,
                            {.78f, .55f, 1, .8f});
                }
            }
        },
        &scenery);
    if (beam > 0)
    {
        // Endpoints are projected from the actual inherited wand socket, not the face.
        scene.Place(
            "beam", {}, RenderLayer::Overlay,
            [](const Game::Actor&, const Game::Transform& socket)
            {
                Vec a = Project({socket.x, socket.y}, socket.z);
                Vec b = Project(beamTarget, 24);
                Line(a, b, 12, {1, .35f, .75f, .12f});
                Line(a, b, 5, {1, .65f, .85f, .55f});
                Line(a, b, 1.7f, {.90f, 1, 1, 1});
                Glow(a, 12, 12, {1, .75f, .85f, .5f});
                Glow(b, 26, 20, {.25f, .95f, 1, .45f});
            },
            scene.Find("player/wand"));
    }
    for (size_t i = 0; i < particles.size(); ++i)
    {
        Particle particle = particles[i];
        PlaceArt(
            "particle/" + std::to_string(i), {particle.p.x, particle.p.y, particle.z},
            RenderLayer::Overlay,
            [particle]()
            {
                Color c = particle.color;
                c.a = particle.life / particle.total;
                Vec s = Project(particle.p, particle.z);
                Diamond(s, 2 + c.a * 2, 2 + c.a * 2, c);
            },
            &effects);
    }
    if (moving)
    {
        PlaceArt(
            "player/dust", {}, RenderLayer::Shadow,
            []()
            {
                Vec s = Project({0, 0});
                for (int i = 0; i < 3; ++i)
                {
                    Ellipse({s.x - facing * (8 + i * 5), s.y + 3 + i}, 3 + i * 2.f, 2,
                            {.65f, .55f, .38f, .06f}, 10);
                }
            },
            &heroine);
    }
    for (int i = 0; i < 55; ++i)
    {
        Vec p = {float((i * 13) % LandWidth), float((i * 19) % MapHeight)};
        PlaceArt(
            "mote/" + std::to_string(i), {p.x, p.y}, RenderLayer::Overlay,
            [p, i]()
            {
                Vec s = Project(p, 18 + std::sin(clockTime + i) * 8);
                if (Visible(s, 10))
                {
                    Glow(s, 4, 4, {.92f, .70f, .33f, .28f});
                }
            },
            &effects);
    }
}
} // namespace Tutorial
