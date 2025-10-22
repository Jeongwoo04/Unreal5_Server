#pragma once
#include "pch.h"

using namespace std;

class BenchmarkStat
{
public:
    void Begin(const std::string& name)
    {
        _startTimes[name] = GetTimeMs();
    }

    void End(const std::string& name, int32 roomId)
    {
        double now = GetTimeMs();
        auto it = _startTimes.find(name);
        if (it == _startTimes.end()) return;

        double duration = now - it->second;
        cout << name <<" No." << roomId << " Tick duration = " << duration << endl;
        _records[name].push_back(duration);
        _startTimes.erase(it);
    }

    void PrintAndSaveSummary(const string& benchWhat, const std::string& filename = "BenchmarkResult.csv")
    {
        std::ofstream file(filename, std::ios::app);
        if (!file.is_open())
        {
            std::cerr << "Failed to open benchmark log file: " << filename << std::endl;
            return;
        }

        std::cout << "\n========== " << benchWhat << " BENCHMARK SUMMARY ==========\n";
        file << "========== " << benchWhat << " BENCHMARK SUMMARY ==========\n";

        for (auto& [name, samples] : _records)
        {
            if (samples.size() < 3) continue;

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

            std::string line =
                name + "," +
                std::to_string(samples.size()) + "," +
                std::to_string(avg) + "," +
                std::to_string(min) + "," +
                std::to_string(max) + "," +
                std::to_string(p01) + "," +
                std::to_string(p99) + "," +
                std::to_string(stddev);

            std::cout
                << name << "\n"
                << "  Count: " << samples.size()
                << " | Avg: " << avg
                << " | Min: " << min
                << " | Max: " << max
                << " | 1%: " << p01
                << " | 99%: " << p99
                << " | StdDev: " << stddev << " (ms)\n";

            file << line << "\n";
        }

        std::cout << "======================================\n";
        file << "======================================\n";
        file.close();
    }

private:
    double GetTimeMs()
    {
        using namespace std::chrono;
        auto now = high_resolution_clock::now();
        return duration_cast<microseconds>(now.time_since_epoch()).count() / 1000.0;
    }

    std::unordered_map<std::string, double> _startTimes;
    std::unordered_map<std::string, std::vector<double>> _records;
};
