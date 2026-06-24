#pragma once
#include "pch.h"
#include "BenchMarkManager.h"
#include <numeric>

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
		//if (now - _programStartTime < _warmupMs)
		//    return;

		_startTimes[name] = now;
	}

	void End(const std::string& name)
	{
		double now = GetTimeMs();

		// 워밍업 구간 스킵
		//if (now - _programStartTime < _warmupMs)
		//	return;

		auto it = _startTimes.find(name);
		if (it == _startTimes.end())
			return;

		double duration = now - it->second;
		_records[name].push_back(duration);
		_startTimes.erase(it);
	}

	void SendData(int32 roomId, const string& benchWhat, const std::string& filename = "Benchmark_MemoryPooling.csv")
	{
		if (_records["Room"].size() < 100)
			return ;

		GBenchMarkManager->AddData(_roundCount, roomId, std::move(_records));
		_records.clear();
		++_roundCount;
	}

private:
	double GetTimeMs()
	{
		using namespace std::chrono;
		auto now = steady_clock::now();
		return duration_cast<microseconds>(now.time_since_epoch()).count() / 1000.0;
	}

	int32 _roundCount = 1;
	double _programStartTime;
	double _warmupMs = 30000.0;
	std::unordered_map<std::string, double> _startTimes;
	std::unordered_map<std::string, std::vector<double>> _records;
};