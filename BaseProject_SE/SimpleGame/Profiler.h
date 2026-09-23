#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <functional>
#include <iomanip>
#include <locale>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace Performance
{
using Clock = std::chrono::steady_clock;

inline double Milliseconds(Clock::time_point start, Clock::time_point end = Clock::now())
{
    return std::chrono::duration<double, std::milli>(end - start).count();
}

// One render thread; aggregate in memory, rotate three 16 MiB files per unique session.
class Profiler
{
  public:
    static Profiler& Get()
    {
        static Profiler instance;
        return instance;
    }

    static std::string Escape(const std::string& value)
    {
        std::ostringstream result;
        for (unsigned char c : value)
        {
            if (c == '"' || c == '\\')
            {
                result << '\\' << char(c);
            }
            else if (c < 32)
            {
                result << "\\u" << std::hex << std::setw(4) << std::setfill('0') << int(c);
            }
            else
            {
                result << char(c);
            }
        }
        return result.str();
    }

    void Initialize(const std::wstring& path, const std::string& gpu, const std::string& version)
    {
        if (path.empty())
        {
            return;
        }
        m_Path = path;
        m_File.open(path.c_str(), std::ios::out | std::ios::trunc);
        m_Enabled = m_File.good();
        m_WindowStart = Clock::now();
        if (m_Enabled)
        {
#ifdef _DEBUG
            const char* build = "debug";
#else
            const char* build = "release";
#endif
            m_Metadata =
                "{\"schema_version\":1,\"event\":\"session_start\",\"build\":\"" +
                std::string(build) + "\",\"gpu_renderer\":\"" + Escape(gpu) +
                "\",\"opengl_version\":\"" + Escape(version) +
                "\",\"cpu_scopes\":\"inclusive_ms\",\"gpu_timing\":\"async_timestamp_timeline_ms\"}";
            Write(m_Metadata);
        }
    }

    bool Enabled() const
    {
        return m_Enabled;
    }

    void Toggle()
    {
        if (m_Enabled)
        {
            FlushWindow();
            m_Enabled = false;
        }
        else if (m_File.is_open() && m_File.good())
        {
            m_Enabled = true;
            m_WindowStart = Clock::now();
        }
    }

    void Sample(const char* name, double milliseconds)
    {
        if (!m_Enabled || !std::isfinite(milliseconds) || milliseconds < 0)
        {
            return;
        }
        auto found = m_Timings.find(name);
        if (found == m_Timings.end())
        {
            found = m_Timings.emplace(name, Metric{}).first;
        }
        auto& metric = found->second;
        ++metric.count;
        metric.sum += milliseconds;
        metric.maximum = (std::max)(metric.maximum, milliseconds);
    }

    void Count(const char* name, double value = 1)
    {
        if (m_Enabled)
        {
            auto found = m_Counters.find(name);
            if (found == m_Counters.end())
            {
                found = m_Counters.emplace(name, 0.0).first;
            }
            found->second += value;
        }
    }

    void Gauge(const char* name, double value)
    {
        if (m_Enabled && std::isfinite(value))
        {
            auto found = m_Gauges.find(name);
            if (found == m_Gauges.end())
            {
                found = m_Gauges.emplace(name, 0.0).first;
            }
            found->second = value;
        }
    }

    void Frame(double intervalMs, const char* scene, unsigned long long frame)
    {
        if (!m_Enabled)
        {
            return;
        }
        m_Scene = scene;
        m_Frame = frame;
        if (frame > 1 && intervalMs > 0)
        {
            Sample("frame.interval_ms", intervalMs);
            if (m_Intervals.size() < 8192)
            {
                m_Intervals.push_back(intervalMs);
            }
        }
        if (intervalMs > 50)
        {
            Count("frame.over_50ms");
        }
        if (Milliseconds(m_WindowStart) >= 1000)
        {
            FlushWindow();
        }
    }

    void FlushWindow()
    {
        if (!m_Enabled || m_Timings.empty())
        {
            return;
        }
        auto start = Clock::now();
        double elapsed = Milliseconds(m_WindowStart, start);
        std::sort(m_Intervals.begin(), m_Intervals.end());
        double p95 =
            m_Intervals.empty() ? 0 : m_Intervals[(m_Intervals.size() * 95 + 99) / 100 - 1];
        std::ostringstream json;
        json.imbue(std::locale::classic());
        json << std::fixed << std::setprecision(4);
        auto unixMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
        json << "{\"schema_version\":1,\"event\":\"performance_window\",\"unix_ms\":" << unixMs
             << ",\"frame_end\":" << m_Frame << ",\"scene\":\"" << Escape(m_Scene)
             << "\",\"window_ms\":" << elapsed << ",\"frame_interval_p95_ms\":" << p95
             << ",\"timings_ms\":{";
        bool first = true;
        for (const auto& item : m_Timings)
        {
            const auto& metric = item.second;
            json << (first ? "" : ",") << '"' << Escape(item.first)
                 << "\":{\"samples\":" << metric.count << ",\"sum\":" << metric.sum
                 << ",\"mean\":" << metric.sum / metric.count << ",\"max\":" << metric.maximum
                 << '}';
            first = false;
        }
        json << "},\"counters_sum\":{";
        first = true;
        for (const auto& item : m_Counters)
        {
            json << (first ? "" : ",") << '"' << Escape(item.first) << "\":" << item.second;
            first = false;
        }
        json << "},\"gauges_last\":{";
        first = true;
        for (const auto& item : m_Gauges)
        {
            json << (first ? "" : ",") << '"' << Escape(item.first) << "\":" << item.second;
            first = false;
        }
        json << "}}";
        Write(json.str());
        m_File.flush();
        m_Timings.clear();
        m_Counters.clear();
        m_Gauges.clear();
        m_Intervals.clear();
        m_WindowStart = Clock::now();
        Sample("cpu.profiler.write_ms", Milliseconds(start));
    }

  private:
    struct Metric
    {
        unsigned long long count = 0;
        double sum = 0;
        double maximum = 0;
    };

    void Write(const std::string& json)
    {
        if (m_Bytes + json.size() + 1 > MaxBytes)
        {
            m_File.close();
            m_Part = (m_Part + 1) % 3;
            std::wstring partPath = m_Part == 0 ? m_Path
                                                : m_Path.substr(0, m_Path.size() - 6) + L"_part" +
                                                      std::to_wstring(m_Part) + L".jsonl";
            m_File.open(partPath.c_str(), std::ios::out | std::ios::trunc);
            if (!m_File.good())
            {
                m_Enabled = false;
                return;
            }
            const std::string marker = "{\"schema_version\":1,\"event\":\"log_rotated\"}\n";
            m_File << m_Metadata << '\n' << marker;
            m_Bytes = m_Metadata.size() + 1 + marker.size();
        }
        m_File << json << '\n';
        m_Bytes += json.size() + 1;
        if (!m_File.good())
        {
            m_Enabled = false;
        }
    }

    static constexpr size_t MaxBytes = 16 * 1024 * 1024;
    std::ofstream m_File;
    std::map<std::string, Metric, std::less<>> m_Timings;
    std::map<std::string, double, std::less<>> m_Counters;
    std::map<std::string, double, std::less<>> m_Gauges;
    std::vector<double> m_Intervals;
    Clock::time_point m_WindowStart = Clock::now();
    unsigned long long m_Frame = 0;
    size_t m_Bytes = 0;
    bool m_Enabled = false;
    std::string m_Scene = "startup";
    std::wstring m_Path;
    std::string m_Metadata;
    int m_Part = 0;
};

class Scope
{
  public:
    explicit Scope(const char* name) : m_Name(name), m_Active(Profiler::Get().Enabled())
    {
        if (m_Active)
        {
            m_Start = Clock::now();
        }
    }

    ~Scope()
    {
        if (m_Active)
        {
            Profiler::Get().Sample(m_Name, Milliseconds(m_Start));
        }
    }

  private:
    const char* m_Name;
    bool m_Active;
    Clock::time_point m_Start;
};
} // namespace Performance
