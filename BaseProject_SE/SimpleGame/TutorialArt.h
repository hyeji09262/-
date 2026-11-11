#pragma once

// Resource meshes share the existing quarter-view renderer and persistent scene graph.
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

inline void FacingNpc(Hunting::ModelId model, Vec position, float scale = 1)
{
    const Vec screen = VisualPoint(Project(position));
    const Vec look = {player.x - position.x, player.y - position.y};
    renderer->DrawModel(VisualModels().GetFacing(model, look.x, look.y), screen.x * width / 1280,
                        screen.y * height / 800, scale * width / 1280, scale * height / 800);
}

inline void Person(Vec p, Color cloak, bool wizard = false, int variant = 0)
{
    Vec s = Project(p);
    if (!Visible(s))
        return;
    if (wizard)
    {
        s.y -= actions.height;
        int frame =
            MagicUnlocked()
                ? CharacterVisual::FrameIndex(castPose, moving, walkCycle / 1.570796f)
                : int(Hunting::ModelId::StudentIdle) + (moving ? 1 + int(walkCycle) % 4 : 0);
        const Vec look = castPose > 0 ? castDirection : facingDirection;
        const auto model = static_cast<Hunting::ModelId>(frame);
        const Vec screen = VisualPoint(s);
        const float scale = visualActive ? visualWorld.scale : 1.f;
        const float mirror = VisualModels().loadedFromResource ? 1.f
                             : look.x - look.y < 0             ? -1.f
                                                               : 1.f;
        renderer->DrawModel(VisualModels().GetFacing(model, look.x, look.y),
                            screen.x * width / 1280, screen.y * height / 800,
                            scale * width / 1280 * mirror, scale * height / 800);
        if (hurtCD > 0)
        {
            Glow({s.x, s.y - 28}, 20, 32, {1, .65f, .85f, .22f});
        }
        return;
    }

    const Hunting::ModelId people[] = {Hunting::ModelId::Jisoo,      Hunting::ModelId::Yuna,
                                       Hunting::ModelId::Jisoo,      Hunting::ModelId::StudentIdle,
                                       Hunting::ModelId::StoreClerk, Hunting::ModelId::SchoolGuard,
                                       Hunting::ModelId::StoreClerk, Hunting::ModelId::StudentIdle,
                                       Hunting::ModelId::StoreClerk, Hunting::ModelId::Yuna};
    FacingNpc(
        static_cast<Hunting::ModelId>(int(Hunting::ModelId::Villager_01View0) + (variant % 10) * 8),
        p);
}

inline void Building(const Prop& p)
{
    Vec s = Project(p.p);
    CachedArt(Hunting::ModelId::Shelter, s, p.kind == 1 ? 1.2f : .85f);
}

inline void DrawProp(const Prop& p)
{
    using Hunting::ModelId;
    if (p.kind == 0 || p.kind == 9)
    {
        const auto model =
            p.kind == 0 ? NatureVisuals::Tree(p.p.x, p.p.y) : NatureVisuals::Rock(p.p.x, p.p.y);
        CachedArt(model, Project(p.p), NatureVisuals::Scale(p.p.x, p.p.y));
        return;
    }
    const ModelId models[] = {
        ModelId::Tree,    ModelId::VillageSchool, ModelId::VillageAcademy, ModelId::VillageStore,
        ModelId::Planter, ModelId::Cabinet,       ModelId::Bench,          ModelId::StreetLamp,
        ModelId::Planter, ModelId::Rock};
    CachedArt(models[(std::max)(0, (std::min)(9, p.kind))], Project(p.p));
}

inline void MonsterArt(const Monster& m)
{
    const Vec s = Project(m.p);
    CachedArt(
        MonsterAnimation::FacingModel(m.kind, m.phase, m.attackMotion, m.facing.x, m.facing.y), s);
    if (m.attackMotion > MonsterAnimation::ImpactRemaining)
    {
        Ellipse({s.x, s.y + 2}, 22, 11, {1.f, .25f, .35f, .22f}, 16);
    }
    if (m.hp < 3)
    {
        Box(s.x - 17, s.y - 74, 34, 4, ink);
        Box(s.x - 17, s.y - 74, 34 * m.hp / 3.f, 4, {.9f, .34f, .35f, 1});
    }
}

inline void BossArt()
{
    CachedArt(Hunting::ModelId::ShadowEater, Project(boss), 1.4f);
}

inline void DrawSchoolMinimap()
{
    const auto& room = SchoolLayout::Get(interiorRoom);
    const float scale = (std::min)(220.f / room.width, 140.f / room.height);
    const float x = 1000, y = 120;
    Box(980, 88, 270, 196, {ink.r, ink.g, ink.b, .94f});
    Text(995, 110, interiorRoom == 0 ? "중앙 복도 · 노란 점: 출입문" : room.name, gold, 15);
    Box(x, y, room.width * scale, room.height * scale, {.31f, .33f, .42f, 1});
    for (const auto& fixture : SchoolLayout::Fixtures(interiorRoom))
    {
        if (fixture.radius > 0)
        {
            Box(x + fixture.x * scale - 2, y + fixture.y * scale - 2, 4, 4, {.65f, .61f, .69f, 1});
        }
    }
    if (interiorRoom == 0)
    {
        for (const auto& door : SchoolLayout::Doors())
        {
            Diamond({x + door.x * scale, y + door.y * scale}, 3, 3, gold);
        }
    }
    Diamond({x + room.width * .5f * scale, y + (room.height - 1.5f) * scale}, 4, 4, gold);
    Ellipse({x + player.x * scale, y + player.y * scale}, 3, 3, teal, 10);
    Text(995, 275, "청록: 내 위치 · E로 출입", teal, 14);
}

inline void DrawMinimap()
{
    Performance::Scope scope("cpu.ui.village_minimap_ms");
    const int town = CurrentTown();
    const float left = 0, top = CoastalWorld::TownCenterY(town) - 35;
    const float scale = 2.7f, x = 1044, y = 120;
    Box(1005, 85, 240, 240, {ink.r, ink.g, ink.b, .94f});
    Text(1020, 110, CoastalWorld::TownName(town), gold, 16);
    Box(x, y, 60 * scale, 70 * scale, {.19f, .32f, .28f, 1});
    auto inside = [top](Vec p)
    {
        return p.x >= 0 && p.x < 60 && p.y >= top && p.y < top + 70;
    };
    auto map = [=](Vec p)
    {
        return Vec{x + (p.x - left) * scale, y + (p.y - top) * scale};
    };
    static int cachedTown = -1;
    static std::vector<Vec> roadCells;
    if (cachedTown != town)
    {
        Performance::Scope rebuildScope("cpu.ui.village_minimap_rebuild_ms");
        roadCells.clear();
        for (int row = 0; row < 70; ++row)
        {
            for (int col = 0; col < 60; ++col)
            {
                if (CoastalWorld::Paved(col, int(top) + row))
                {
                    roadCells.push_back({float(col), float(row)});
                }
            }
        }
        cachedTown = town;
    }
    for (const auto& cell : roadCells)
    {
        Box(x + cell.x * scale, y + cell.y * scale, scale, scale, {.48f, .48f, .49f, 1});
    }
    for (const auto& building : CoastalWorld::Buildings())
    {
        if (building.town != town)
            continue;
        const Vec p = map({building.x, building.y});
        Box(p.x - 3, p.y - 3, 6, 6,
            building.interior == 5 ? Color{.36f, .7f, .4f, 1} : Color{.85f, .65f, .88f, 1});
    }
    for (const auto& npc : villagers)
    {
        if (!inside(npc.p))
            continue;
        const Vec p = map(npc.p);
        Box(p.x - 1, p.y - 1, 2, 2, {.8f, .8f, .8f, 1});
    }
    float tx, ty;
    int person;
    std::string objective;
    const int destination = MiaQuestDestination();
    if (destination < 0 && Prologue::WorldObjective(tx, ty, person, objective) && inside({tx, ty}))
    {
        Diamond(map({tx, ty}), 4, 4, gold);
    }
    else
    {
        int room;
        if (destination < 0 &&
            QuestTwo::SchoolObjective(0, player.x, player.y, room, tx, ty, objective))
        {
            const auto& school = CoastalWorld::Buildings()[1];
            const Vec p{CoastalWorld::DoorX(school), CoastalWorld::DoorY(school)};
            if (inside(p))
            {
                Diamond(map(p), 4, 4, gold);
            }
        }
        else if (destination >= 0)
        {
            const auto& building = CoastalWorld::Buildings()[destination];
            const Vec p{CoastalWorld::DoorX(building), CoastalWorld::DoorY(building)};
            if (inside(p))
            {
                Diamond(map(p), 4, 4, gold);
            }
        }
        else if (QuestTwo::NeedsPotionShop())
        {
            const auto& shop = CoastalWorld::Buildings()[2];
            const Vec p{CoastalWorld::DoorX(shop), CoastalWorld::DoorY(shop)};
            if (inside(p))
            {
                Diamond(map(p), 4, 4, gold);
            }
        }
    }
    if (inside(player))
        Ellipse(map(player), 3, 3, teal, 10);
    if (HuntingEntranceOpen() && inside(HuntingEntrance))
    {
        Diamond(map(HuntingEntrance), 4, 4, {.62f, .93f, .52f, 1});
    }
    Text(1020, 318, HuntingEntranceOpen() ? "청록: 나 · 노랑: 목표 · 연두: 숲"
                                         : "청록: 나 · 노랑: 현재 목표",
         teal, 13);
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

inline void InteriorLabel(const std::string& id, Vec position, const std::string& text,
                          bool objective = false, float heightOffset = 112)
{
    const auto layer = objective ? Game::RenderLayer::Objective : Game::RenderLayer::Overlay;
    scene.Place(id, {position.x, position.y, heightOffset}, layer,
                [text, objective](const Game::Actor&, const Game::Transform& world)
                {
                    Vec p = Project({world.x, world.y}, world.z);
                    if (objective)
                    {
                        p.x = (std::max)(125.f, (std::min)(1155.f, p.x));
                        p.y = (std::max)(175.f, (std::min)(670.f, p.y));
                    }
                    renderer->Nameplate(p.x * width / 1280, p.y * height / 800, text,
                                        16.f * height / 800, 1, .8f, .35f, objective);
                });
}

inline void ObjectiveLabel(const std::string& id, Vec position, const std::string& nearText,
                           const std::string& distantText = "목표", float heightOffset = 140)
{
    InteriorLabel(id, position, Dist(player, position) <= 2.3f ? nearText : distantText, true,
                  heightOffset);
}

inline void BuildingObjectiveLabel(const std::string& id, const CoastalWorld::Building& building,
                                   Vec target, const std::string& nearText)
{
    const float labelHeight = BuildingVisuals::Get(building.style).labelHeight + 28.f;
    InteriorLabel(id, {building.x, building.y}, Dist(player, target) <= 2.3f ? nearText : "목표",
                  true, labelHeight);
}

inline void EntranceMarker(const std::string& id, Vec entrance, float range, const char* name,
                           const char* action)
{
    const bool ready = Dist(player, entrance) < range && actions.height <= 0;
    PlaceArt(id + "/ground", {entrance.x, entrance.y}, Game::RenderLayer::Shadow,
             [entrance, ready]()
             {
                 const Vec center = Project(entrance);
                 const float pulse = 2.f * std::sin(clockTime * 4.f);
                 Diamond(center, 23.f + pulse, 13.f + pulse * .5f, {.22f, .17f, .25f, .95f});
                 Diamond(center, 19.f, 9.f, ready ? Color{1.f, .9f, .61f, 1.f} : gold);
                 Diamond(center, 5.f, 3.f, {.22f, .17f, .25f, 1.f});
             });
    InteriorLabel(id + "/label", entrance, ready ? action : name, true, 50.f);
}

inline void DrawLumi()
{
    if (!exploration || !storyMagicUnlocked)
    {
        return;
    }
    const Vec look = castPose > 0 ? castDirection : facingDirection;
    const Vec position = CompanionFollow::RightOf(player, look);
    scene.Place("lumi", {position.x, position.y, 26 + 4 * std::sin(clockTime * 3)},
                Game::RenderLayer::World,
                [look](const Game::Actor&, const Game::Transform& world)
                {
                    const Vec screen = Project({world.x, world.y}, world.z);
                    const auto& mesh = VisualModels().GetFacing(Hunting::ModelId::Lumi,
                                                               look.x, look.y);
                    renderer->DrawModel(mesh, screen.x * width / 1280, screen.y * height / 800,
                                        world.scale * width / 1280, world.scale * height / 800);
                });
    InteriorLabel("lumi/name", position, "루미 [F]", false, 86.f);
}

inline void DrawStaff()
{
    Performance::Scope scope("cpu.world.staff_sync_ms");
    const int kind = interior < 0 ? -1 : CoastalWorld::Buildings()[interior].interior;
    for (size_t i = 0; i < TownPopulation::StaffMembers().size(); ++i)
    {
        const auto staff = TownPopulation::StaffMembers()[i];
        if (staff.kind != kind || (kind == 1 && staff.room != interiorRoom))
            continue;
        const Vec p = {staff.x, staff.y + (kind < 0 ? CurrentTown() * 70.f : 0.f)};
        PlaceArt("staff/" + std::to_string(i), {p.x, p.y}, Game::RenderLayer::World,
                 [staff, p]()
                 {
                     FacingNpc(staff.model, p);
                 });
        if (Dist(player, p) < 5)
            InteriorLabel("staff/name/" + std::to_string(i), p,
                          IsMia(staff, CurrentTown()) ? "미아" : staff.name);
    }
}

inline void DrawRecovery(Game::Actor& hero)
{
    CombatEffects::Recovery(
        scene, *renderer, hero, actions, actions.height, (std::min)(width / 1280.f, height / 800.f),
        [](const Game::Transform& world)
        {
            const Vec p = Project({world.x, world.y}, world.z);
            return Vec{p.x * width / 1280, p.y * height / 800};
        });
}

inline void DrawInterior()
{
    using Game::RenderLayer;
    using Hunting::ModelId;
    Performance::Scope scope("cpu.world.interior_draw_ms");
    const auto& building = CoastalWorld::Buildings()[interior];
    if (building.interior == 1)
    {
        const auto& room = SchoolLayout::Get(interiorRoom);
        if (!scene.Find("school/interior"))
        {
            SchoolScene::Build(scene, interiorRoom,
                               [](ModelId model, const Game::Transform& world)
                               {
                                   CachedArt(model, Project({world.x, world.y}, world.z),
                                             world.scale);
                               },
                               *renderer,
                               [](const Game::Transform& world)
                               {
                                   const Vec p = Project({world.x, world.y}, world.z);
                                   return Vec{p.x * width / 1280, p.y * height / 800};
                               });
        }
        int targetRoom;
        float targetX, targetY;
        std::string targetLabel;
        const bool hasSchoolTarget =
            exploration && building.town == 0 &&
            QuestTwo::SchoolObjective(interiorRoom, player.x, player.y, targetRoom, targetX,
                                      targetY, targetLabel);
        if (!hasSchoolTarget || interiorRoom == 0 || targetRoom == interiorRoom)
        {
            InteriorLabel("inside/exit", {room.width * .5f, room.height - 1.5f},
                          interiorRoom ? "[E] 중앙 복도로" : "[E] 학교 밖으로");
        }
        if (interiorRoom == 0)
        {
            for (const auto& door : SchoolLayout::Doors())
            {
                if ((!hasSchoolTarget || door.room != targetRoom) &&
                    Dist(player, {door.x, door.y}) < 2.f)
                {
                    InteriorLabel("inside/room/" + std::to_string(door.room), {door.x, door.y},
                                  std::string("[E] ") + SchoolLayout::Get(door.room).name);
                }
            }
        }
        else if (!hasSchoolTarget || targetRoom != interiorRoom)
        {
            InteriorLabel("inside/examine", {room.width * .5f, 4},
                          std::string("[E] ") + room.name + " 조사");
        }
        if (interiorRoom == 2)
        {
            PlaceArt("inside/court", {16, 13}, RenderLayer::Ground,
                     []()
                     {
                         Line(Project({7, 6}), Project({25, 6}), 2, {1, .94f, .85f, 1});
                         Line(Project({7, 21}), Project({25, 21}), 2, {1, .94f, .85f, 1});
                         Line(Project({7, 6}), Project({7, 21}), 2, {1, .94f, .85f, 1});
                         Line(Project({25, 6}), Project({25, 21}), 2, {1, .94f, .85f, 1});
                         Line(Project({7, 13}), Project({25, 13}), 2, {1, .94f, .85f, 1});
                     });
        }
        if (hasSchoolTarget)
        {
            Vec target{targetX, targetY};
            if (targetRoom != interiorRoom)
            {
                target = {room.width * .5f, room.height - 1.5f};
                if (interiorRoom == 0)
                {
                    for (const auto& door : SchoolLayout::Doors())
                    {
                        if (door.room == targetRoom)
                        {
                            target = {door.x, door.y};
                        }
                    }
                }
            }
            if (targetRoom == interiorRoom)
            {
                ObjectiveLabel("inside/quest-target", target, "[E] 조사");
            }
            else
            {
                const std::string roomName = SchoolLayout::Get(targetRoom).name;
                const std::string action = interiorRoom == 0 ? roomName + " [E]" : "복도로 [E]";
                ObjectiveLabel("inside/quest-target", target, action, "목표 · " + roomName);
            }
        }
    }
    else
    {
        if (!scene.Find("inside/floor"))
        {
            VillageInterior::Build(scene, building.interior,
                                   [](ModelId model, const Game::Transform& world)
                                   {
                                       CachedArt(model, Project({world.x, world.y}, world.z),
                                                 world.scale);
                                   });
        }
        InteriorLabel("inside/exit", {10, 16.5f}, "[E] 마을로 나가기");
        float storyX, storyY;
        int storyPerson;
        std::string storyText;
        const bool clerkGoal = exploration && building.interior == 2 && building.town == 0 &&
                               Prologue::WorldObjective(storyX, storyY, storyPerson, storyText) &&
                               storyPerson == 3;
        const bool potionGoal = building.interior == 2 && QuestTwo::NeedsPotionShop();
        const bool miaGoal = interior == StoryMall && MiaQuestDestination() == StoryMall;
        if (!clerkGoal && !potionGoal && !miaGoal)
        {
            InteriorLabel("inside/examine", {14, VillageInterior::ServiceY(building.interior)},
                          building.interior == 2   ? "[E] 직원 대화 / 물약 구매"
                          : building.interior == 4 ? "[E] 휴식/회복"
                                                   : "[E] 조사");
        }
        if (building.interior == 2)
        {
            if (clerkGoal || potionGoal)
            {
                const bool nearCounter = Dist(player, {14, VillageInterior::ServiceY(2)}) < 2.3f;
                const char* goal = clerkGoal ? "목표 · 편의점 직원" : "목표 · 물약 구매";
                const char* action = clerkGoal ? "나루에게 묻기 [E]" : "물약 구매 [E]";
                InteriorLabel("inside/store-goal", {14, 2}, nearCounter ? action : goal, true,
                              140.f);
            }
            InteriorLabel("inside/store-drinks", {6, 2.4f}, "냉장 음료");
            InteriorLabel("inside/store-snacks", {6, 7}, "간식 · 식품");
            InteriorLabel("inside/store-goods", {6, 11}, "생활용품 · 물약");
            InteriorLabel("inside/store-seat", {16.5f, 11}, "취식 공간");
        }
        else if (building.interior == 4)
        {
            InteriorLabel("inside/hospital-sign", {4, 3}, "진료 · 병상");
            InteriorLabel("inside/reception", {14, 5}, "접수 · 회복");
        }
        else if (building.interior == 6)
        {
            InteriorLabel("inside/station-sign", {5, 3}, "승차권 발매기");
        }
        else if (building.interior == 7)
        {
            InteriorLabel("inside/mall-sign", {5, 3}, "상점가 · 상품 진열");
            if (miaGoal)
            {
                const bool nearMia = Dist(player, MiaCounter) < MiaInteractionRange ||
                                     Dist(player, MiaPosition) < 1.8f;
                const char* action = miaQuest == MiaQuest::ShowFragment ? "검은 조각 보여 주기 [E]"
                                     : miaQuest == MiaQuest::ReportPark ? "미아에게 보고 [E]"
                                                                       : "미아와 대화 [E]";
                InteriorLabel("inside/mia-goal", MiaPosition, nearMia ? action : "목표 · 미아", true,
                              140.f);
            }
        }
        if (interior == 0 && exploration)
        {
            InteriorLabel("inside/home_story", {VillageInterior::HomeTvX, VillageInterior::HomeTvY},
                          "[E] TV · 이야기");
        }
    }
    DrawStaff();
    auto& actor = PlaceArt("inside/player", {player.x, player.y}, RenderLayer::World,
                           []()
                           {
                               Person(player, teal, true);
                           });
    PlaceArt(
        "inside/player/shadow", {}, RenderLayer::Shadow,
        []()
        {
            Shadow({0, 0}, 13, 5, 18);
        },
        &actor);
    DrawRecovery(actor);
}

inline void DrawWorld()
{
    using Game::RenderLayer;
    DrawLumi();
    if (interior >= 0)
    {
        DrawInterior();
        return;
    }
    auto& terrain = scene.Place("terrain", {}, RenderLayer::Ground);
    auto& scenery = scene.Place("scenery", {}, RenderLayer::World);
    auto& characters = scene.Place("characters", {}, RenderLayer::World);
    auto& effects = scene.Place("effects", {}, RenderLayer::Overlay);
    PlaceArt("background", {}, RenderLayer::Background,
             []()
             {
                 Box(0, 0, 1280, 800, {.10f, .14f, .18f, 1});
             });
    // Stream retained 32x32 chunks around the camera; avoid scanning the full expanded terrain.
    Performance::Scope terrainScope("cpu.world.outdoor_actor_sync_ms");
    for (int cy = 0; cy < (MapHeight + 31) / 32; ++cy)
    {
        for (int cx = 0; cx < (MapWidth + 31) / 32; ++cx)
        {
            const std::string chunkId = "chunk/" + std::to_string(cy * 2 + cx);
            const bool nearby = player.x + 24 >= cx * 32 && player.x - 24 < (cx + 1) * 32 &&
                                player.y + 24 >= cy * 32 && player.y - 24 < (cy + 1) * 32;
            if (!nearby)
            {
                if (scene.Find(chunkId))
                {
                    scene.Remove(chunkId);
                    Performance::Profiler::Get().Count("world.chunks_unloaded");
                }
                continue;
            }
            if (scene.Find(chunkId))
            {
                continue;
            }
            auto& chunk = scene.Place(chunkId, {}, RenderLayer::Ground, {}, &terrain);
            for (int y = cy * 32; y < (std::min)(MapHeight, (cy + 1) * 32); ++y)
            {
                for (int x = cx * 32; x < (std::min)(MapWidth, (cx + 1) * 32); ++x)
                {
                    const bool paved = Path(x, y) || CoastalWorld::SchoolGround(float(x), float(y));
                    const auto model =
                        paved ? Hunting::ModelId::FloorStone : Hunting::ModelId::TerrainGrass;
                    const bool vegetation = !paved && x < LandWidth - 2 && (x + y) % 3 == 0;
                    PlaceArt(
                        "tile/" + std::to_string(y * MapWidth + x), {float(x), float(y)},
                        RenderLayer::Ground,
                        [x, y, model, vegetation]()
                        {
                            Vec s = Project({float(x), float(y)});
                            if (!Visible(s, 40))
                                return;
                            CachedArt(model, s, 1.01f);
                            if (x >= LandWidth)
                            {
                                float wave = std::sin(clockTime * 1.6f + x * .6f + y * .8f);
                                Line({s.x - 18, s.y + wave * 3}, {s.x + 8, s.y + wave * 3 - 2},
                                     1.4f, {.53f, .74f, .77f, .15f + wave * .06f});
                                if (x == LandWidth)
                                {
                                    float tide = std::sin(clockTime * 1.2f + y * .5f) * 4;
                                    Line({s.x - 30 + tide, s.y}, {s.x + tide, s.y - 15}, 2.4f,
                                         {.80f, .86f, .79f, .50f});
                                    Line({s.x - 27 + tide, s.y + 3}, {s.x + 3 + tide, s.y - 12}, 1,
                                         {.80f, .86f, .79f, .18f});
                                }
                            }
                            else if (vegetation)
                            {
                                CachedArt(NatureVisuals::Grass(float(x), float(y)), s,
                                          NatureVisuals::Scale(float(x), float(y)));
                            }
                        },
                        &chunk);
                }
            }
            scene.RetainSubtree(chunk);
            Performance::Profiler::Get().Count("world.chunks_loaded");
        }
    }
    if (!scene.Find("prop/0"))
    {
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
            if (prop.kind >= 1 && prop.kind <= 3)
            {
                const char* label = prop.kind == 1   ? "별빛 학교"
                                    : prop.kind == 2 ? "도서관"
                                                     : "동아리관";
                float roof = 125.f * (prop.kind == 1 ? 1.2f : .85f);
                scene.Place(
                    name + "/label", {0, 0, roof + 12}, RenderLayer::Overlay,
                    [label](const Game::Actor&, const Game::Transform& world)
                    {
                        if (Dist(player, {world.x, world.y}) > 5.5f)
                        {
                            return;
                        }
                        Vec screen = Project({world.x, world.y}, world.z);
                        if (Visible(screen, 0))
                        {
                            renderer->Nameplate(screen.x * width / 1280, screen.y * height / 800,
                                                label, 17.f * height / 800);
                        }
                    },
                    &actor);
            }
            PlaceArt(
                name + "/shadow", {}, RenderLayer::Shadow,
                [prop]()
                {
                    float size = prop.kind >= 1 && prop.kind <= 3 ? 42.f
                                 : prop.kind == 0                 ? 19.f
                                                                  : 12.f;
                    Shadow({0, 0}, size, size * .30f, prop.kind <= 3 ? 43.f : 12.f);
                },
                &actor);
        }
        scene.RetainSubtree(scenery);
    }
    if (!scene.Find("building/0"))
    {
        for (size_t i = 0; i < CoastalWorld::Buildings().size(); ++i)
        {
            const auto building = CoastalWorld::Buildings()[i];
            auto& actor = PlaceArt(
                "building/" + std::to_string(i), {building.x, building.y}, RenderLayer::World,
                [building]()
                {
                    if (building.interior == 5)
                    {
                        return;
                    }
                    CachedArt(static_cast<Hunting::ModelId>(
                                  int(Hunting::ModelId::VillageHouseLilac) + building.style),
                              Project({building.x, building.y}));
                },
                &scenery);
            PlaceArt(
                "building/shadow/" + std::to_string(i), {}, RenderLayer::Shadow,
                [building]()
                {
                    if (building.interior == 5)
                    {
                        return;
                    }
                    const float radius = BuildingVisuals::Get(building.style).shadowRadius;
                    Shadow({0, 0}, radius, radius * .30f, radius * .65f);
                },
                &actor);
            scene.Place(
                "building/name/" + std::to_string(i),
                {0, 0, BuildingVisuals::Get(building.style).labelHeight}, RenderLayer::Overlay,
                [building](const Game::Actor&, const Game::Transform& world)
                {
                    if (Dist(player, {building.x, building.y}) > 7)
                    {
                        return;
                    }
                    Vec p = Project({world.x, world.y}, world.z);
                    renderer->Nameplate(p.x * width / 1280, p.y * height / 800,
                                        std::string(building.name) + " [E]", 17.f * height / 800);
                },
                &actor);
        }
        scene.RetainSubtree(scenery);
    }
    for (int town = 0; town < CoastalWorld::TownCount; ++town)
    {
        const Vec position = {TownPopulation::GuardX, TownPopulation::GuardY +
                                                          CoastalWorld::TownCenterY(town) -
                                                          CoastalWorld::TownCenterY(0)};
        const std::string id = "world/guard/" + std::to_string(town);
        auto& guard = PlaceArt(
            id, {position.x, position.y}, RenderLayer::World,
            [position]()
            {
                FacingNpc(Hunting::ModelId::Security_GuardView0, position);
            },
            &characters);
        PlaceArt(
            id + "/shadow", {}, RenderLayer::Shadow,
            []()
            {
                Shadow({0, 0}, 12, 5, 15);
            },
            &guard);
        if (Dist(player, position) < 5.f)
        {
            InteriorLabel(id + "/name", position, "경비원 단");
        }
    }
    float objectiveX, objectiveY;
    int objectivePerson;
    std::string objectiveText;
    const int destination = MiaQuestDestination();
    if (destination < 0 && exploration &&
        Prologue::WorldObjective(objectiveX, objectiveY, objectivePerson, objectiveText))
    {
        const Hunting::ModelId people[] = {Hunting::ModelId::Yuna, Hunting::ModelId::Jisoo,
                                           Hunting::ModelId::SchoolGuard,
                                           Hunting::ModelId::StoreClerk};
        if (objectivePerson < 2)
        {
            PlaceArt("world/story-person", {objectiveX, objectiveY}, RenderLayer::World,
                     [objectiveX, objectiveY, objectivePerson]()
                     {
                         const Hunting::ModelId models[] = {
                             Hunting::ModelId::Yuna, Hunting::ModelId::Jisoo,
                             Hunting::ModelId::SchoolGuard, Hunting::ModelId::StoreClerk};
                         FacingNpc(models[objectivePerson], {objectiveX, objectiveY});
                     });
        }
        const Vec target{objectiveX, objectiveY};
        if (objectivePerson == 3)
        {
            BuildingObjectiveLabel("world/story-objective", CoastalWorld::Buildings()[2], target,
                                   "편의점 안 직원에게");
        }
        else if (objectivePerson == 4)
        {
            EntranceMarker("world/alley-entrance", target, WorldStoryInteractionRange,
                           "학교 뒤 골목 입구", "골목 들어가기 [E]");
        }
        else
        {
            ObjectiveLabel("world/story-objective", target, "[E] 이야기");
        }
    }
    if (destination < 0 && exploration)
    {
        int room;
        float x, y;
        std::string label;
        if (QuestTwo::SchoolObjective(0, player.x, player.y, room, x, y, label))
        {
            const auto& school = CoastalWorld::Buildings()[1];
            const Vec target{CoastalWorld::DoorX(school), CoastalWorld::DoorY(school)};
            BuildingObjectiveLabel("world/quest-two-school", school, target, "별빛 학교 [E]");
        }
    }
    if (destination >= 0)
    {
        const auto& building = CoastalWorld::Buildings()[destination];
        const Vec target{CoastalWorld::DoorX(building), CoastalWorld::DoorY(building)};
        if (destination == StoryPark)
        {
            EntranceMarker("world/park-entrance", target, BuildingEntryRange,
                           "별빛 중앙공원 입구", "공원으로 이동 [E]");
        }
        else
        {
            BuildingObjectiveLabel("world/mia-quest-goal", building, target, "별빛 쇼핑몰 [E]");
        }
    }
    else if (exploration && QuestTwo::NeedsPotionShop())
    {
        const auto& shop = CoastalWorld::Buildings()[2];
        const Vec target{CoastalWorld::DoorX(shop), CoastalWorld::DoorY(shop)};
        BuildingObjectiveLabel("world/potion-shop-goal", shop, target, "편의점 [E]");
    }
    if (destination != StoryPark && MiaQuestUnlocked() && miaQuest >= MiaQuest::ParkIncident)
    {
        const auto& park = CoastalWorld::Buildings()[StoryPark];
        const Vec entrance{CoastalWorld::DoorX(park), CoastalWorld::DoorY(park)};
        if (Dist(player, entrance) < 5.f)
        {
            EntranceMarker("world/park-entrance", entrance, BuildingEntryRange,
                           "별빛 중앙공원 입구", "공원으로 이동 [E]");
        }
    }
    if (HuntingEntranceOpen())
    {
        EntranceMarker("world/hunting-entrance", HuntingEntrance, BuildingEntryRange,
                       "별빛 숲 입구", "사냥터로 이동 [E]");
    }
    else if (exploration)
    {
        InteriorLabel("world/region-gate", HuntingEntrance, "학교 사건 해결 후 별빛 숲 개방");
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
    DrawRecovery(heroine);
    const Vec look = castPose > 0 ? castDirection : facingDirection;
    scene.Place("player/wand", CharacterVisual::FacingSocket(castPose > 0, look.x, look.y),
                RenderLayer::World, {}, &heroine);
    scene.Find("player/wand")->local.z += actions.height;
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
    if (!exploration && interior < 0 && MagicUnlocked() && stage < 2)
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
    auto drawSpell = [](float size)
    {
        return [size](const Game::Actor&, const Game::Transform& world)
        {
            Vec screen = Project({world.x, world.y}, world.z);
            if (!Visible(screen))
            {
                return;
            }
            renderer->Effect((screen.x - size * .5f) * width / 1280,
                             (screen.y - size * .5f) * height / 800, size * width / 1280,
                             size * height / 800, clockTime, 2);
        };
    };
    if (castPose > 0)
    {
        scene.Place("player/wand/flash", {}, RenderLayer::World, drawSpell(24),
                    scene.Find("player/wand"));
    }
    for (const auto& spell : spells)
    {
        auto pose = CharacterVisual::FacingSpellPose(
            spell.origin.x + spell.direction.x * CharacterVisual::SpellSpeed * spell.age,
            spell.origin.y + spell.direction.y * CharacterVisual::SpellSpeed * spell.age, spell.age,
            spell.direction.x, spell.direction.y);
        pose.z += spell.launchHeight * (1 - (std::min)(1.f, spell.age / .18f));
        scene.Place("spell/" + std::to_string(spell.id), pose, RenderLayer::World,
                    drawSpell(CharacterVisual::SpellSize));
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
                for (int i = 0; i < 3; ++i)
                {
                    const float distance = .25f + i * .15f;
                    Vec s = Project({-facingDirection.x * distance, -facingDirection.y * distance});
                    Ellipse({s.x, s.y + 3 + i}, 3 + i * 2.f, 2, {.65f, .55f, .38f, .06f}, 10);
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
