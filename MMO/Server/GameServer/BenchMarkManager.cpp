#include "pch.h"
#include "BenchMarkManager.h"
#include <Psapi.h>
#include <iomanip>
#pragma comment(lib, "psapi.lib")

BenchMarkManager* GBenchMarkManager = new BenchMarkManager();

void BenchMarkManager::AddData(int32 roundCount, int32 roomId, unordered_map<string, vector<double>>&& data)
{
	WRITE_LOCK;

	RoundData& roundData = _roundData[roundCount];
	roundData._roomData[roomId] = std::move(data);
	roundData.received.set(roomId);

	for (auto& [name, samples] : roundData._roomData[roomId])
	{
		auto& total = _roundTotalData[roundCount].samples[name];

		total.insert(total.end(), samples.begin(), samples.end());
	}
}

void BenchMarkManager::AddWorkerData(int32 roundCount, int32 workerId, WorkerStats stats)
{
	WRITE_LOCK;

	auto& data = _workerRoundData[roundCount];
	data.workers[workerId] = std::move(stats);
	data.received.set(workerId);
}

bool BenchMarkManager::CheckRoundRoom()
{
	WRITE_LOCK;

	auto iter = _roundData.find(_roundCount);
	if (iter == _roundData.end())
		return false;

	return iter->second.received.all();
}

bool BenchMarkManager::CheckRoundWorker()
{
	WRITE_LOCK;

	auto iter = _workerRoundData.find(_roundCount);
	if (iter == _workerRoundData.end())
		return false;

	return iter->second.received.count() == LOGIC_WORKER_COUNT;
}

vector<BenchState> BenchMarkManager::CalculateStates(unordered_map<string, vector<double>>& records)
{
	vector<BenchState> result;

	for (auto& [name, samples] : records)
	{
		if (samples.empty())
			continue;

		sort(samples.begin(), samples.end());

		BenchState state;

		double sum = accumulate(samples.begin(), samples.end(), 0.0);

		state.Name = name;
		state.Samples = samples.size();
		state.Avg = sum / samples.size();
		state.Min = samples.front();
		state.Max = samples.back();

		state.p01 = samples[static_cast<size_t>(samples.size() * 0.01)];
		state.p99 = samples[static_cast<size_t>(samples.size() * 0.99)];

		double variance = 0;

		for (double v : samples)
			variance += (v - state.Avg) * (v - state.Avg);
		
		variance /= samples.size();

		state.StdDev = sqrt(variance);

		result.push_back(state);
	}

	return result;
}

void BenchMarkManager::CalculateData()
{
	_result = {};

	RoundData& round = _roundData[_roundCount];

	for (int32 roomId = 0; roomId < ROOM_COUNT; roomId++)
	{
		RoomBenchResult roomResult;
		roomResult.roomId = roomId;
		roomResult.states = CalculateStates(round._roomData[roomId]);

		_result.rooms.push_back(std::move(roomResult));
	}

	_result.totalStates = CalculateStates(_roundTotalData[_roundCount].samples);

	WorkerRoundData& workerRound = _workerRoundData[_roundCount];
	
	for (auto& w : workerRound.workers)
	{
		if (w.JobCounts == 0)
			continue;

		w.ActiveTimeMs = QpcToMilliseconds(w.ActiveTime);
		double roundTimeMs = static_cast<double>(_roundTime.endTick - _roundTime.startTick);
		w.ActiveRatio = (w.ActiveTimeMs / roundTimeMs) * 100.0;
	}
}

void BenchMarkManager::WriteCSV()
{
	ofstream file(fileName, ios::app);

	if (!file.is_open())
		return;


	file << "==============================\n";
	file << "Multi Room Benchmark\n";
	file << "Round : "
		<< _roundCount << "\n\n";


	for (auto& room : _result.rooms)
	{
		file << "========== RoomId : "
			<< room.roomId
			<< " ==========\n";

		file << "Name,Samples,Avg,Min,Max,p01,p99,StdDev\n";

		for (auto& state : room.states)
		{
			file
				<< state.Name << ","
				<< state.Samples << ","
				<< state.Avg << ","
				<< state.Min << ","
				<< state.Max << ","
				<< state.p01 << ","
				<< state.p99 << ","
				<< state.StdDev
				<< "\n";
		}

		file << "\n";
	}

	file << "========== TOTAL ==========\n";
	file << "Name,Samples,Avg,Min,Max,p01,p99,StdDev\n";

	for (auto& state : _result.totalStates)
	{
		file
			<< state.Name << ","
			<< state.Samples << ","
			<< state.Avg << ","
			<< state.Min << ","
			<< state.Max << ","
			<< state.p01 << ","
			<< state.p99 << ","
			<< state.StdDev
			<< "\n";
	}

	file << "========== LOGIC WORKER ==========\n";
	file << "Worker#,JobCounts,JobQueueCount,TimeSliceExceeded,ActiveTime,Utilization\n";

	int32 workerNum = 0;
	for (auto& w : _workerRoundData[_roundCount].workers)
	{
		if (w.JobCounts == 0)
			continue;

		file
			<< workerNum << ","
			<< w.JobCounts << ","
			<< w.ExecuteQueueCounts << ","
			<< w.TimeSliceExceeded << ","
			<< w.ActiveTimeMs << ","
			<< std::fixed << std::setprecision(2)
			<< w.ActiveRatio << "%"
			<< "\n";

		workerNum++;
	}
	file << std::defaultfloat;

	PROCESS_MEMORY_COUNTERS_EX pmc;
	// 현재 프로세스의 메모리 정보 가져오기
	if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
	{
		// WorkingSetSize: 현재 실제 물리 메모리 점유량 (RAM)
		// PrivateUsage: 이 프로세스에 할당된 가상 메모리 점유량 (Commit Charge)
		size_t physicalMem = pmc.WorkingSetSize;
		size_t virtualMem = pmc.PrivateUsage;

		file << "========================================\n";
		file << "[Memory Monitor]\n";
		file << "Physical Memory (RAM): " << std::fixed << std::setprecision(2)
			<< (double)physicalMem / (1024 * 1024) << " MB\n";
		file << "Virtual Memory (Commit): " << (double)virtualMem / (1024 * 1024) << " MB\n";

		file << "========================================\n";
	}

	file << "====================================\n\n";
	file.close();

	WRITE_LOCK;
	_roundData.erase(_roundCount);
	_roundTotalData.erase(_roundCount);
	_workerRoundData.erase(_roundCount);

	++_roundCount;
}