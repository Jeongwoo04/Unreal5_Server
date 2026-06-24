#pragma once
#include "pch.h"

#include <bitset>

struct WorkerStats
{
    uint64 JobCounts;
    uint64 ExecuteQueueCounts;
    uint64 ActiveTime;
    uint64 TimeSliceExceeded;
    
    double ActiveTimeMs;
    double ActiveRatio;
};

struct RoundTime
{
    uint64 startTick = 0;
    uint64 endTick = 0;
};

enum ProcessState
{
    COLLECT_ROOM,
    WAIT_WORKER_FLUSH,
    CALCULATE
};

static constexpr int32 LOGIC_WORKER_COUNT = 2;

struct RoundData
{
    array<unordered_map<string, vector<double>>, 250> _roomData;
    bitset<250> received;
};

struct WorkerRoundData
{
    array<WorkerStats, 12> workers;
    bitset<12> received;
};

struct TotalData
{
    unordered_map<string, vector<double>> samples;
};

//Name,Samples,Avg,Min,Max,p01,p99,StdDev
struct BenchState
{
    string Name;
    size_t Samples;
    double Avg;
    double Min;
    double Max;
    double p01;
    double p99;
    double StdDev;
};

struct RoomBenchResult
{
    int32 roomId;
    vector<BenchState> states;
};

struct RoundBenchResult
{
    vector<RoomBenchResult> rooms;
    vector<BenchState> totalStates;
};

class BenchMarkManager
{
public:
    void AddData(int32 roundCount, int32 roomId, unordered_map<string, vector<double>>&& data);
    void AddWorkerData(int32 roundCount, int32 workerId, WorkerStats stats);
    bool CheckRoundRoom();
    bool CheckRoundWorker();
    vector<BenchState> CalculateStates(unordered_map<string, vector<double>>& records);
    void CalculateData();
    void WriteCSV();

    ProcessState GetProcessState() { return _processState; }
    void SetProcessState(ProcessState state) { _processState = state; }

    int32 GetBenchRound() { return _roundCount; }

public:
    USE_LOCK;
    static constexpr int32 ROOM_COUNT = 250;
    ProcessState _processState = ProcessState::COLLECT_ROOM;

    unordered_map<int32, RoundData> _roundData;
    unordered_map<int32, TotalData> _roundTotalData;

    unordered_map<int32, WorkerRoundData> _workerRoundData;

    RoundBenchResult _result;
    RoundTime   _roundTime;
    int32 _roundCount = 1;
    const string& fileName = "Benchmark_MultiRoom.csv";
};

extern BenchMarkManager* GBenchMarkManager;

static inline double QpcToMilliseconds(int64 tick)
{
    static int64 frequency = []()
        {
            LARGE_INTEGER li;
            ::QueryPerformanceFrequency(&li);
            return li.QuadPart;
        }();

        return (tick * 1000.0) / frequency;
}