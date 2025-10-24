#pragma once
#include "pch.h"

using namespace std;

class BenchmarkStat
{
public:
    BenchmarkStat()
    {
        _programStartTime = GetTimeMs();
        _warmupMs = 30000.0; // 기본 30초 (원하면 SetWarmupTime으로 변경 가능)
    }

    void SetWarmupTime(double seconds)
    {
        _warmupMs = seconds * 1000.0;
    }

    void Begin(const std::string& name)
    {
        double now = GetTimeMs();

        // 워밍업 구간 스킵
        if (now - _programStartTime < _warmupMs)
            return;

        _startTimes[name] = now;
    }

    void End(const std::string& name)
    {
        double now = GetTimeMs();

        // 워밍업 구간 스킵
        if (now - _programStartTime < _warmupMs)
            return;

        auto it = _startTimes.find(name);
        if (it == _startTimes.end())
            return;

        double duration = now - it->second;
        _records[name].push_back(duration);
        _startTimes.erase(it);
    }

    void PrintAndSaveSummary(int32 roomId, const string& benchWhat, const std::string& filename = "BenchmarkResult.csv")
    {
        double now = GetTimeMs();

        // 워밍업 구간 스킵
        if (now - _programStartTime < _warmupMs)
            return;

        if (_records["Room"].size() % 100 != 0)
            return ;

        std::ofstream file(filename, std::ios::app);
        if (!file.is_open())
        {
            std::cerr << "Failed to open benchmark log file: " << filename << std::endl;
            return;
        }

        std::cout << "\n========== " << benchWhat << " BENCHMARK SUMMARY ==========\n";
        file << "========== RoomId : " << roomId << " " << benchWhat << " BENCHMARK SUMMARY ==========\n";
        file << "Name,Samples,Avg,Min,Max,p01,p99,StdDev\n";

        for (auto& [name, samples] : _records)
        {
            if ((samples.size() % 100 != 0))
                continue;
            std::sort(samples.begin(), samples.end());
            double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
            double avg = sum / samples.size();
            double min = samples.front();
            double max = samples.back();

            double p01 = samples[(int)(samples.size() * 0.01)];
            double p99 = samples[(int)(samples.size() * 0.99)];

            double variance = 0.0;
            for (double v : samples)
                variance += (v - avg) * (v - avg);
            variance /= samples.size();
            double stddev = std::sqrt(variance);

            // CSV 저장
            file << name << ","
                << samples.size() << ","
                << avg << ","
                << min << ","
                << max << ","
                << p01 << ","
                << p99 << ","
                << stddev << "\n";

            // 콘솔 출력
            std::cout
                << name << "\n"
                << "  Count: " << samples.size()
                << " | Avg: " << avg
                << " | Min: " << min
                << " | Max: " << max
                << " | 1%: " << p01
                << " | 99%: " << p99
                << " | StdDev: " << stddev << " (ms)\n";
        }

        std::cout << "======================================\n";
        file << "======================================\n";
        file.close();
        _records.clear();
    }

private:
    double GetTimeMs()
    {
        using namespace std::chrono;
        auto now = high_resolution_clock::now();
        return duration_cast<microseconds>(now.time_since_epoch()).count() / 1000.0;
    }

    double _programStartTime;
    double _warmupMs = 30000.0;
    std::unordered_map<std::string, double> _startTimes;
    std::unordered_map<std::string, std::vector<double>> _records;
};