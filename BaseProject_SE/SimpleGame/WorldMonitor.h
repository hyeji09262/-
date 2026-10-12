#pragma once
#include <chrono>
#include <fstream>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>
#include <vector>
#include "Profiler.h"
#include "RuntimeFiles.h"

namespace WorldMonitor
{
struct Npc
{
    std::string id, name, state;
    float x = 0, y = 0, z = 0;
    bool present = true;
};

struct Town
{
    std::string id, name, atmosphere;
    std::vector<std::string> rumors;
    // No faction/opinion simulation exists yet. Do not manufacture a measured value.
    std::string disposition = "미구현";
};

struct Scene
{
    std::string id, name, objective;
    bool paused = false;
    int mobsAlive = 0, mobsDead = 0;
    std::vector<Npc> npcs;
    std::vector<Town> towns;
};

inline bool Due()
{
    static auto next = Performance::Clock::time_point{};
    const auto now = Performance::Clock::now();
    if (now < next)
    {
        return false;
    }
    next = now + std::chrono::seconds(1);
    return true;
}

inline std::string Quote(const std::string& text)
{
    return "\"" + Performance::Profiler::Escape(text) + "\"";
}

inline void Publish(const std::string& active, const std::vector<Scene>& scenes,
                    unsigned long long drawCalls)
{
    Performance::Scope scope("cpu.monitor.publish_ms");
    static unsigned long long sequence = 0;
    static const auto session = std::chrono::system_clock::now().time_since_epoch().count();
    const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch()).count();
    std::ostringstream json;
    json.imbue(std::locale::classic());
    json << "{\"schema_version\":1,\"session_id\":" << Quote(std::to_string(session))
         << ",\"sequence\":" << ++sequence << ",\"timestamp_ms\":" << timestamp
         << ",\"active_scene\":" << Quote(active) << ",\"draw_calls_last_frame\":" << drawCalls
         << ",\"scenes\":[";
    bool firstScene = true;
    for (const auto& scene : scenes)
    {
        int population = 0;
        for (const auto& npc : scene.npcs)
        {
            population += npc.present ? 1 : 0;
        }
        if (!firstScene)
        {
            json << ',';
        }
        firstScene = false;
        json << "{\"id\":" << Quote(scene.id) << ",\"name\":" << Quote(scene.name)
             << ",\"objective\":" << Quote(scene.objective)
             << ",\"active\":" << (scene.id == active ? "true" : "false")
             << ",\"paused\":" << (scene.paused || scene.id != active ? "true" : "false")
             << ",\"npc_count\":" << population << ",\"mobs_alive\":" << scene.mobsAlive
             << ",\"mobs_dead\":" << scene.mobsDead << ",\"towns\":[";
        for (size_t i = 0; i < scene.towns.size(); ++i)
        {
            const auto& town = scene.towns[i];
            if (i)
            {
                json << ',';
            }
            json << "{\"id\":" << Quote(town.id) << ",\"name\":" << Quote(town.name)
                 << ",\"atmosphere\":" << Quote(town.atmosphere)
                 << ",\"disposition\":" << Quote(town.disposition)
                 << ",\"source\":\"scripted_not_simulated\",\"population\":" << population
                 << ",\"population_basis\":\"present_npcs_excluding_player\",\"rumors\":[";
            for (size_t r = 0; r < town.rumors.size(); ++r)
            {
                json << (r ? "," : "") << Quote(town.rumors[r]);
            }
            json << "]}";
        }
        json << "],\"npcs\":[";
        for (size_t i = 0; i < scene.npcs.size(); ++i)
        {
            const auto& npc = scene.npcs[i];
            json << (i ? "," : "") << "{\"id\":" << Quote(npc.id)
                 << ",\"name\":" << Quote(npc.name) << ",\"state\":" << Quote(npc.state)
                 << ",\"present\":" << (npc.present ? "true" : "false")
                 << ",\"position\":{\"x\":" << npc.x << ",\"y\":" << npc.y
                 << ",\"z\":" << npc.z << "},\"hp\":null}";
        }
        json << "]}";
    }
    json << "]}";
    const auto path = RuntimeFiles::Path(L"world_monitor.json");
    bool success = false;
    if (!path.empty())
    {
        const auto temporary = path + L".tmp";
        {
            std::ofstream file(temporary.c_str(), std::ios::binary | std::ios::trunc);
            file << json.str();
            file.flush();
            success = bool(file);
        }
        success = success && RuntimeFiles::Commit(temporary, path);
    }
    Performance::Profiler::Get().Count(success ? "monitor.snapshots_written" : "monitor.write_failures");
    Performance::Profiler::Get().Gauge("monitor.snapshot_bytes", double(json.str().size()));
    static bool reportedFailure = false;
    if (!success && !reportedFailure)
    {
        std::cerr << "World monitor: snapshot write failed; gameplay continues.\n";
    }
    reportedFailure = !success;
}
} // namespace WorldMonitor
