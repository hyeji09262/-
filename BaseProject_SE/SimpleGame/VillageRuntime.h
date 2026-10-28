#pragma once

// Included inside Tutorial after movement helpers; shared by training and story exploration.
struct VillageVisit
{
    Vec position{31, 26};
    Vec facing{.70710678f, -.70710678f};
    int health = 5;
    float magic = 30;
    int healthPotions = 5;
    int magicPotions = 5;
    int defeated = 0;
    float time = 0;
    std::array<int, 3> regionalKills{};
    std::vector<Monster> wildlife;
};

static std::array<VillageVisit, 2> visits;

inline int CurrentTown()
{
    return interior >= 0 ? CoastalWorld::Buildings()[interior].town
                         : CoastalWorld::NearestTown(player.y);
}

inline void LeaveBuilding()
{
    if (interior < 0)
    {
        return;
    }
    player = outdoorReturn;
    interior = -1;
    interiorRoom = 0;
    actions.height = actions.velocity = 0;
    spells.clear();
    particles.clear();
    std::fill(keys, keys + 256, false);
    scene.Clear();
}

inline void SetExploration(bool enabled, bool unlocked)
{
    if (enabled != exploration)
    {
        LeaveBuilding();
        auto& previous = visits[exploration ? 1 : 0];
        previous.position = player;
        previous.facing = facingDirection;
        previous.health = hp;
        previous.magic = mana;
        previous.healthPotions = potions;
        previous.magicPotions = manaPotions;
        previous.defeated = kills;
        previous.time = elapsed;
        previous.regionalKills = townKills;
        previous.wildlife = std::move(monsters);
        auto& next = visits[enabled ? 1 : 0];
        player = next.position;
        facingDirection = next.facing;
        castDirection = facingDirection;
        hp = next.health;
        mana = next.magic;
        potions = next.healthPotions;
        manaPotions = next.magicPotions;
        kills = next.defeated;
        elapsed = next.time;
        townKills = next.regionalKills;
        monsters = std::move(next.wildlife);
        spells.clear();
        particles.clear();
        actions = PlayerActions{};
        castPose = attackCD = hurtCD = 0;
        scene.Clear();
    }
    sceneRequest = 0;
    exploration = enabled;
    storyMagicUnlocked = unlocked;
    if (!MagicUnlocked())
    {
        monsters.clear();
    }
    paused = false;
    moving = false;
    std::fill(keys, keys + 256, false);
}

inline bool SchoolInteraction()
{
    const auto& room = SchoolLayout::Get(interiorRoom);
    if (Dist(player, {room.width * .5f, room.height - 1.5f}) < 2.f)
    {
        if (interiorRoom == 0)
        {
            LeaveBuilding();
        }
        else
        {
            for (const auto& door : SchoolLayout::Doors())
            {
                if (door.room == interiorRoom)
                {
                    player = {door.x, door.y + 2};
                    break;
                }
            }
            interiorRoom = 0;
            scene.Clear();
            Say("중앙 복도로 돌아왔습니다.");
        }
        return true;
    }
    if (interiorRoom == 0)
    {
        for (const auto& door : SchoolLayout::Doors())
        {
            if (Dist(player, {door.x, door.y}) < 2.f)
            {
                interiorRoom = door.room;
                const auto& next = SchoolLayout::Get(interiorRoom);
                player = {next.width * .5f, next.height - 3.f};
                int targetRoom;
                float targetX, targetY;
                std::string targetLabel;
                if (exploration && CoastalWorld::Buildings()[interior].town == 0 &&
                    QuestTwo::SchoolObjective(interiorRoom, player.x, player.y, targetRoom, targetX,
                                              targetY, targetLabel) &&
                    targetRoom == interiorRoom)
                {
                    sceneRequest = 3;
                    return true;
                }
                scene.Clear();
                Say(std::string(next.name) + " · 출입문에서 E를 누르면 복도로 돌아갑니다.");
                Performance::Profiler::Get().Count("school.room_entries_count");
                return true;
            }
        }
    }
    else if (Dist(player, {room.width * .5f, 4}) < 2)
    {
        const char* details[] = {
            "",
            "체육관 대출 기록에 검은 가루가 묻어 있습니다.",
            "체육관의 노란 목표를 따라 사건을 조사하세요.",
            "방송 장비에서 체육관 쪽 발소리가 들립니다.",
            "1학년 1반 · 오늘의 시간표: 국어, 수학, 마법 사고 예방.",
            "1학년 2반 · 칠판에는 '학교를 구해도 숙제는 제출'이라고 적혀 있습니다.",
            "과학실 · 보라색 시약은 마나 물약이 아닙니다. 마시지 마세요.",
            "미술실 · 학생들이 그린 별빛 마을 풍경이 놓여 있습니다."};
        Say(details[interiorRoom]);
    }
    return true;
}

inline bool BuildingInteraction()
{
    if (actions.height > 0)
    {
        return false;
    }
    if (interior < 0)
    {
        for (size_t i = 0; i < CoastalWorld::Buildings().size(); ++i)
        {
            const auto& building = CoastalWorld::Buildings()[i];
            if (Dist(player, {CoastalWorld::DoorX(building), CoastalWorld::DoorY(building)}) < 2.f)
            {
                if (building.interior == 5)
                {
                    Say("중앙공원 · 주민들의 쉼터입니다. 앞으로 이곳에서 마을 이야기가 이어집니다.");
                    return true;
                }
                outdoorReturn = {CoastalWorld::DoorX(building), CoastalWorld::DoorY(building)};
                interior = int(i);
                interiorRoom = 0;
                player = building.interior == 1 ? Vec{18, 10} : Vec{10, 15};
                spells.clear();
                particles.clear();
                scene.Clear();
                Say(std::string(building.name) + "에 들어왔습니다. 출입구 [E]로 나갈 수 있습니다.");
                int targetRoom;
                float targetX, targetY;
                std::string targetLabel;
                if (exploration && building.town == 0 && building.interior == 1 &&
                    QuestTwo::SchoolObjective(0, player.x, player.y, targetRoom, targetX, targetY,
                                              targetLabel) &&
                    targetRoom == 0)
                {
                    sceneRequest = 3;
                }
                Performance::Profiler::Get().Count("world.buildings_entered");
                return true;
            }
        }
        return false;
    }
    const auto& building = CoastalWorld::Buildings()[interior];
    for (const auto& staff : TownPopulation::StaffMembers())
    {
        if (staff.kind == building.interior && staff.kind != 2 &&
            (staff.kind != 1 || staff.room == interiorRoom) &&
            Dist(player, {staff.x, staff.y}) < 1.8f)
        {
            Say(std::string(staff.name) + ": " + staff.line);
            return true;
        }
    }
    if (building.interior == 1)
    {
        return SchoolInteraction();
    }
    if (Dist(player, {10, 16.5f}) < 2.f)
    {
        if (interiorRoom != 0)
        {
            interiorRoom = 0;
            player = {5, 6};
            scene.Clear();
            Say("학교 로비로 돌아왔습니다.");
        }
        else
        {
            LeaveBuilding();
        }
        return true;
    }
    if (building.interior == 0 && interior == 0 && exploration &&
        Dist(player, {VillageInterior::HomeTvX, VillageInterior::HomeTvY}) < 1.8f)
    {
        sceneRequest = 1;
        return true;
    }
    if (Dist(player, {14, VillageInterior::ServiceY(building.interior)}) < 2.3f)
    {
        if (building.interior == 2)
        {
            float x, y;
            int person;
            std::string objective;
            if (building.town == 0 && Prologue::WorldObjective(x, y, person, objective) &&
                person == 3)
            {
                Say(Prologue::InteractWorld());
                return true;
            }
            if (QuestTwo::Finished())
            {
                shopOpen = true;
                Say("편의점 물약 판매 · 1: 체력 20골드 / 2: 마나 25골드 / E 또는 ESC: 닫기");
            }
            else
            {
                potions = (std::max)(5, potions);
                manaPotions = (std::max)(5, manaPotions);
                Say("초보자 보급: 체력/마나 물약을 각각 최소 5개까지 채웠습니다.");
            }
        }
        else if (building.interior == 4)
        {
            hp = 5;
            mana = 30;
            Say("병원에서 치료받았습니다. 체력과 마나가 회복됐습니다.");
        }
        else if (building.interior == 6)
        {
            Say("지하철 안내: 현재 악몽 안전 점검으로 운행 중단. 다음 지역 이야기에 연결될 장소입니다.");
        }
        else if (building.interior == 7)
        {
            Say("쇼핑몰 안내: 상점가와 분실물 센터입니다. 이곳의 의뢰는 추후 개방됩니다.");
        }
        else
        {
            Say(interiorRoom == 1
                    ? "도서관: 체육관 관련 기록에 검은 가루가 묻어 있습니다. 별빛 학교 로비에서 메인 퀘스트 2를 이어가세요."
                : interiorRoom == 2
                    ? "체육관: 훈련 도구와 코트가 놓여 있습니다. 사건 전투는 별빛 학교 로비에서 이어집니다."
                : interiorRoom == 3 ? "방송실: 녹음 장치에서 이상한 잡음이 들립니다."
                : building.interior == 3
                    ? "학원: 오늘 수업은 마법 사고 없이 귀가하는 방법입니다."
                    : "집 안에는 주민들의 일상과 소중한 물건들이 남아 있습니다.");
        }
    }
    return true;
}

inline void BuildSpawnSites()
{
    Performance::Scope scope("cpu.world.spawn_sites_build_ms");
    spawnSites.clear();
    for (int y = 3; y < MapHeight - 3; y += 3)
    {
        for (int x = 3; x < LandWidth - 3; x += 3)
        {
            Vec p{float(x), float(y)};
            if (Walkable(p) && !Safe(p) && !Path(x, y) && Dist(p, boss) > 5 && Dist(p, shrine) > 4)
            {
                spawnSites.push_back(p);
            }
        }
    }
    Performance::Profiler::Get().Gauge("world.spawn_sites_count", double(spawnSites.size()));
}

inline bool Respawn(Monster& monster, bool initial)
{
    Performance::Scope scope("cpu.world.respawn_search_ms");
    for (size_t attempt = 0; attempt < spawnSites.size(); ++attempt)
    {
        const Vec p = spawnSites[spawnCursor++ % spawnSites.size()];
        if (Dist(p, player) < 10 || (!initial && Dist(p, monster.p) < 10))
        {
            continue;
        }
        bool occupied = false;
        for (const auto& other : monsters)
        {
            if (&other != &monster && other.hp > 0 && Dist(p, other.p) < 2)
            {
                occupied = true;
                break;
            }
        }
        if (occupied)
        {
            continue;
        }
        monster.p = monster.home = p;
        monster.hp = 3;
        monster.cooldown = 1;
        monster.attackMotion = 0;
        monster.respawn = 0;
        Performance::Profiler::Get().Count("world.monsters_spawned");
        return true;
    }
    monster.respawn = .5f;
    Performance::Profiler::Get().Count("world.spawn_retries");
    return false;
}

inline void SyncWildlife()
{
    Performance::Scope scope("cpu.world.spawn_gate_ms");
    if (!MagicUnlocked())
    {
        monsters.clear();
        return;
    }
    if (monsters.empty() && interior < 0)
    {
        monsters.resize(CoastalWorld::WildPopulation);
        for (size_t i = 0; i < monsters.size(); ++i)
        {
            auto& monster = monsters[i];
            monster.hp = 0;
            monster.kind = int(i % MonsterAnimation::WildKinds);
            monster.phase = float(i) * .7f;
            spawnCursor = unsigned(i * spawnSites.size() / monsters.size());
            Respawn(monster, true);
        }
    }
}

inline WorldMonitor::Scene MonitorSnapshot()
{
    WorldMonitor::Scene snapshot;
    snapshot.id = exploration ? "world" : "tutorial";
    snapshot.name = exploration ? "해안 마을 탐험" : "튜토리얼 · 해안 마을";
    snapshot.paused = paused || (!exploration && (stage == 2 || stage == 5));
    snapshot.objective = interior >= 0 ? CoastalWorld::Buildings()[interior].name
                         : exploration ? "3개 마을 탐험 · 학교와 집에서 스토리 진행"
                                       : "튜토리얼 단계 " + std::to_string(stage);
    for (int i = 0; i < NPCCount; ++i)
    {
        const auto& npc = villagers[i];
        snapshot.npcs.push_back(
            {"resident_" + std::to_string(i), npc.name,
             interior < 0 && Dist(player, npc.p) < 1.8f ? "상호작용 근처" : "마을 생활 중", npc.p.x,
             npc.p.y, 0, true});
    }
    for (int town = 0; town < 3; ++town)
    {
        for (size_t i = 0; i < TownPopulation::StaffMembers().size(); ++i)
        {
            const auto& staff = TownPopulation::StaffMembers()[i];
            float x = staff.x, y = staff.y + town * 70.f;
            if (staff.kind >= 0)
            {
                for (const auto& building : CoastalWorld::Buildings())
                {
                    if (building.town == town && building.interior == staff.kind)
                    {
                        x = building.x;
                        y = building.y;
                        break;
                    }
                }
            }
            snapshot.npcs.push_back(
                {"staff_" + std::to_string(town) + "_" + std::to_string(i), staff.name,
                 staff.kind < 0 ? "학교 정문 근무" : "시설 내부 근무", x, y, 0, true});
        }
        int threats = 0;
        for (const auto& monster : monsters)
        {
            if (monster.hp > 0 && CoastalWorld::NearestTown(monster.p.y) == town)
            {
                ++threats;
            }
        }
        WorldMonitor::Town state;
        state.id = "coast_town_" + std::to_string(town);
        state.name = CoastalWorld::TownName(town);
        state.population =
            CoastalWorld::ResidentsPerTown + int(TownPopulation::StaffMembers().size());
        state.threats = threats;
        state.defeated = townKills[town];
        state.safety = (std::max)(0, 100 - threats * 4);
        state.atmosphere = !MagicUnlocked()     ? "평온 · 변신 전 악몽 미출현"
                           : state.safety >= 60 ? "경계 · 외곽 순찰 필요"
                                                : "불안 · 숲의 악몽 증가";
        state.source = "derived_from_live_monsters";
        state.disposition = "주민 협조적 (설정값)";
        state.rumors = {villagers[town * 10 + 4].line, villagers[town * 10 + 5].line};
        snapshot.towns.push_back(state);
    }
    for (const auto& monster : monsters)
    {
        monster.hp > 0 ? ++snapshot.mobsAlive : ++snapshot.mobsDead;
    }
    return snapshot;
}
