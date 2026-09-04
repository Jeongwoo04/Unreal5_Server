#pragma once
#include "pch.h"

#include <bitset>

struct RoomSnap;

enum ProcessState
{
    COLLECT_ROOM,
    WAIT_WORKER_FLUSH,
    ABSTRACT_DATA,
    CALCULATE
};

enum WorkerType
{
    IO,
    LOGIC,
    SEND
};

struct WorkerStats
{
    WorkerType type;

    uint64 JobCounts;
    uint64 ExecuteQueueCounts;
    uint64 ActiveTime;
    uint64 TimeSliceExceeded;

    //uint64 ImmediateEmpty;
    //uint64 DeferEmpty;

    double ActiveTimeMs;
    double ActiveRatio;
};

struct MemoryResult
{
    double PhysicalMB = 0.0;
    double VirtualMB = 0.0;
};

static constexpr int32 LOGIC_WORKER_COUNT = 2;
static constexpr int32 SEND_WORKER_COUNT = 2;

struct RoundData
{
    array<unordered_map<string, vector<double>>, 250> _roomData;
    bitset<250> received;
};

struct WorkerRoundData
{
    vector<WorkerStats> workers;
};

struct TotalData
{
    unordered_map<string, vector<double>> samples;
};

struct IOData
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

struct RoundTime
{
    uint64 startTick = 0;
    uint64 endTick = 0;
};

struct RoundSnapShot
{
    RoundData roundData;
    WorkerRoundData workerRoundData;
    TotalData totalData;
    IOData ioData;

    uint64 GlobalIOPendingCounts;
    RoundTime roundTime;
};

struct RoundBench
{
    int32 Round = 0;
    uint32 globalIoPendingCounts = 0;

    vector<RoomBenchResult> rooms;
    vector<BenchState> totalStates;

    vector<WorkerStats> Workers;

    MemoryResult Memory = {};
};

struct RoomLog
{
    int32 roomID;
    double tickCost;
    double tickInterval;
    pair<int32, int32> players;
    pair<int32, int32> monsters;
    pair<int32, int32> projectiles;
    pair<int32, int32> fields;
    int32 ImmediatePktBiteSize;
    int32 DeferPktBiteSize;
};

struct RoundLog
{
    int32 round = 0;
    unordered_map<string, double> totalRoom;
    int32 IOPending;
    vector<WorkerStats> workers;
};

class BenchMarkManager
{
public:
    void AddData(int32 roundCount, int32 roomId, unordered_map<string, vector<double>>&& data);
    void AddWorkerData(int32 roundCount, int32 workerId, WorkerStats stats);
    void AddIOData(const string& name, vector<double>&& data);
    bool CheckRoundRoom();
    bool CheckRoundWorker();
    vector<BenchState> CalculateStates(unordered_map<string, vector<double>>& records, bool isTotal);
    void AbstractData();
    void CalculateData();
    void WriteCSV();

    void Rendering();

    ProcessState GetProcessState() { return _processState; }
    void SetProcessState(ProcessState state) { _processState = state; }

    int32 GetBenchRound() { return _roundCount; }

public:
    USE_LOCK;
    static constexpr int32 ROOM_COUNT = 250;
    ProcessState _processState = ProcessState::COLLECT_ROOM;

    unordered_map<int32, RoundData> _roundData;
    unordered_map<int32, WorkerRoundData> _workerRoundData;

    IOData _ioData = {};
    RoundSnapShot _snapshot;
    RoundBench _result;

    RoundTime _roundTime;
    int32 _roundCount = 1;
    const string& fileName = "Temp.csv";
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