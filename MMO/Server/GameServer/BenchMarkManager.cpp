#include "pch.h"
#include "BenchMarkManager.h"
#include <Psapi.h>
#include <iomanip>
#include "RoomManager.h"
#include "RoomDiagnostics.h"
#pragma comment(lib, "psapi.lib")

BenchMarkManager* GBenchMarkManager = new BenchMarkManager();

void BenchMarkManager::AddData(int32 roundCount, int32 roomId, unordered_map<string, vector<double>>&& data)
{
	WRITE_LOCK;

	RoundData& roundData = _roundData[roundCount];
	roundData._roomData[roomId] = std::move(data);
	roundData.received.set(roomId);
}

void BenchMarkManager::AddWorkerData(int32 roundCount, int32 workerId, WorkerStats stats)
{
	WRITE_LOCK;

	auto& data = _workerRoundData[roundCount];
	data.workers.push_back(std::move(stats));
}

void BenchMarkManager::AddIOData(const string& name, vector<double>&& data)
{
	WRITE_LOCK;

	_ioData.samples[name] = std::move(data);
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

	const ThreadConfig& config = ConfigManager::Instance().GetConfig()._threadConfig;

	return iter->second.workers.size() == (config.LOGIC + config.SEND);
}

vector<BenchState> BenchMarkManager::CalculateStates(unordered_map<string, vector<double>>& records, bool isTotal)
{
	vector<BenchState> result;
	
	vector<string> GLog = { "Room","Update","TickInterval","totalDelay","QueueingDelay","kernelDelivery" };
	unordered_map<string, double> temp;

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
		if (isTotal == true && find(GLog.begin(), GLog.end(), name) != GLog.end())
		{
			temp[name] = state.Avg;
		}
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

	if (isTotal)
	{
		std::shared_lock<std::shared_mutex> writeLock(GLogLock);
		std::swap(GRoundLog.totalRoom, temp);
	}

	return result;
}

void BenchMarkManager::AbstractData()
{
	_snapshot = {};

	{
		WRITE_LOCK;

		_snapshot.roundData = std::move(_roundData[_roundCount]);
		_snapshot.workerRoundData = std::move(_workerRoundData[_roundCount]);
		_snapshot.roundTime = _roundTime;
		_snapshot.ioData = std::move(_ioData);
		_ioData.samples.clear();
		_snapshot.GlobalIOPendingCounts = GIOPendingCounts.load();

		_roundData.erase(_roundCount);
		_workerRoundData.erase(_roundCount);
	}

	for (auto& data : _snapshot.roundData._roomData)
	{
		for (auto& [name, samples] : data)
		{
			auto& total = _snapshot.totalData.samples[name];

			total.insert(total.end(), samples.begin(), samples.end());
		}
	}

	for (auto& [name, samples] : _snapshot.ioData.samples)
	{
		auto& total = _snapshot.totalData.samples[name];

		total.insert(total.end(), samples.begin(), samples.end());
	}

	SetProcessState(ProcessState::CALCULATE);

	return ;
}

void BenchMarkManager::CalculateData()
{
	_result = {};
	_result.Round = _roundCount;

	for (int32 roomId = 0; roomId < ROOM_COUNT; roomId++)
	{
		RoomBenchResult roomResult;
		roomResult.roomId = roomId;
		roomResult.states = CalculateStates(_snapshot.roundData._roomData[roomId], false);

		_result.rooms.push_back(std::move(roomResult));
	}

	_result.totalStates = CalculateStates(_snapshot.totalData.samples, true);
	_result.globalIoPendingCounts = _snapshot.GlobalIOPendingCounts;
	GRoundLog.IOPending = _snapshot.GlobalIOPendingCounts;

	WorkerRoundData& workerRound = _snapshot.workerRoundData;
	
	vector<WorkerStats> temp;

	for (auto& w : workerRound.workers)
	{
		if (w.JobCounts == 0)
			continue;

		w.ActiveTimeMs = QpcToMilliseconds(w.ActiveTime);
		double roundTimeMs = static_cast<double>(_snapshot.roundTime.endTick - _snapshot.roundTime.startTick);
		w.ActiveRatio = (w.ActiveTimeMs / roundTimeMs) * 100.0;

		temp.push_back(w);
		_result.Workers.push_back(w);
	}

	{
		std::shared_lock<std::shared_mutex> writeLock(GLogLock);
		std::swap(GRoundLog.workers, temp);
		GRoundLog.round = _roundCount++;
	}

	/* TEMP TypeChunk Utilization 측정 시 재확인
	PROCESS_MEMORY_COUNTERS_EX pmc;
	// 현재 프로세스의 메모리 정보 가져오기
	if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
	{
		// WorkingSetSize: 현재 실제 물리 메모리 점유량 (RAM)
		// PrivateUsage: 이 프로세스에 할당된 가상 메모리 점유량 (Commit Charge)
		size_t physicalMem = pmc.WorkingSetSize;
		size_t virtualMem = pmc.PrivateUsage;

		_result.Memory.PhysicalMB = (double)physicalMem / (1024 * 1024);
		_result.Memory.VirtualMB = (double)virtualMem / (1024 * 1024);
	}
	*/
}

void BenchMarkManager::WriteCSV()
{
	ofstream file(fileName, ios::app);

	if (!file.is_open())
		return;

	file << "========== Direct LAN MultiRoom Bench ==========\n\n";

	file << "==============================\n";
	file << "환경\n";
	file << "CPU : Intel i5-12400F (6C/12T)\n";
	file << "OS : Windows\n";
	file << "Build : Release x64\n";
	file << "Direct LAN TEST\n\n";

	file << "Server\n";
	file << "IO Worker : 2\n";
	file << "Logic Worker : 2\n";
	file << "Send Worker : 4\n";
	file << "Round = 100Ticks / Room\n\n";

	file << "Client\n";
	file << "ARM Mac M1 Dummy Client (.NET)\n";
	file << "5000 Dummy Session\n\n";

	file << "250 Room\n";
	file << "20 Sessions / Room\n";
	file << "50 Monsters AI / Room\n";
	file << "==============================\n\n";

	file << "==============================\n";
	file << "Multi Room Benchmark Round : ";
	file << _roundCount << "\n";
	file << "==============================\n\n";

	// Total
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

	file << "\n=== Global IO Pending Counts ===\n";
	file << _result.globalIoPendingCounts << "\n";
	file << "============================\n";

	// Worker
	file << "\n========== LOGIC WORKER ==========\n";
	file << "Worker#,JobCounts,JobQueueCount,TimeSliceExceeded,ActiveTime,Utilization\n";

	int32 workerNum = 0;
	for (auto& w : _result.Workers)
	{
		if (w.type != WorkerType::LOGIC)
			continue;

		file
			<< workerNum << ","
			<< w.JobCounts << ","
			<< w.ExecuteQueueCounts << ","
			<< w.TimeSliceExceeded << ","
			//<< w.ImmediateEmpty << ","
			//<< w.DeferEmpty << ","
			<< w.ActiveTimeMs << ","
			<< std::fixed << std::setprecision(2)
			<< w.ActiveRatio << "%"
			<< "\n";

		workerNum++;
	}
	file << std::defaultfloat;

	file << "\n========== SEND WORKER ==========\n";
	file << "Worker#,JobCounts,JobQueueCount,TimeSliceExceeded,ActiveTime,Utilization\n";

	workerNum = 0;
	for (auto& w : _result.Workers)
	{
		if (w.type != WorkerType::SEND)
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

	// Memory
	//{
	//	file << "\n========================================\n";
	//	file << "[Memory Monitor]\n";
	//	file << "Physical Memory (RAM): " << std::fixed << std::setprecision(2)
	//		<< _result.Memory.PhysicalMB << " MB\n";
	//	file << "Virtual Memory (Commit): " << _result.Memory.VirtualMB << " MB\n";

	//	file << "========================================\n\n";
	//}
	//file << std::defaultfloat;

	// All Rooms
	for (auto& room : _result.rooms)
	{
		file << "========== RoomId : "
			<< room.roomId
			<< " Session : "
			<< RoomManager::Instance().GetSessionCount(room.roomId)
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

	file.close();

	cout << "Round " << _roundCount << " Clear\n";
}
