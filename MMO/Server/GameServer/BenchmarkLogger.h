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

	}

	void Begin(const std::string& name)
	{
		double now = GetTimeMs();

		_startTimes[name] = now;
	}

	void End(const std::string& name)
	{
		double now = GetTimeMs();

		auto it = _startTimes.find(name);
		if (it == _startTimes.end())
			return;

		double duration = now - it->second;
		_records[name].push_back(duration);
		_startTimes.erase(it);
	}
	
	void AddData(const std::string& name, int32 count)
	{
		_records[name].push_back(count);
	}

	void SendData(int32 roomId)
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
	std::unordered_map<std::string, double> _startTimes;
	std::unordered_map<std::string, std::vector<double>> _records;
};