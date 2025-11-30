#pragma once

class RoomDiagnostics
{
public:
	void BeginTick();
	void EndTick();

	// Object counts
	void SetObjectCounts(int32 players, int32 monsters, int32 projectiles, int32 fields);

	// Network status
	void SetImmediateFlushInfo(int32 flushQueueSize, int32 flushPktBytes);
	void SetDeferFlushInfo(int32 flushQueueSize, int32 flushPktBytes);
	void AddSendDelay(double delayMs); // for avg / peak
	//void AddMoveCount(int32 count);
	//void AddSkillInstance(int32 count);
	//void AddCastingStart(int32 count);
	//void AddCastingSuccess(int32 count);
	//void AddAction(int32 count);
	//void AddEnd(int32 count);
	//void AddHit(int32 count);

	// Worker
	void SetRoomWorkerInfo(const string& roomWorker);
	void SetSendWorkerInfo(const string& sendWorker);

	// Render console output
	void Render();

private:
	// Tick measurement
	std::chrono::steady_clock::time_point _tickStart;
	deque<std::pair<std::chrono::steady_clock::time_point, double>> _sendDelays;

	deque<std::pair<std::chrono::steady_clock::time_point, int32>> _moveCounts;

	double _tickMs = 0.0;
	double _tickIntervalMs = 0.0; // 예상값

	// Object
	int32 _players = 0;
	int32 _monsters = 0;
	int32 _projectiles = 0;
	int32 _fields = 0;

	// Network
	int32 _immediateFlushQueue = 0;
	int32 _immediateFlushPktBytes = 0;

	int32 _deferFlushQueue = 0;
	int32 _deferFlushPktBytes = 0;

	double _sendDelayLast = 0.0;
	double _sendDelayAvg = 0.0;
	double _sendDelayPeak = 0.0;

	int32 _sendDelayCount = 0;

	// TEMP : Move Burst 재현
	//bool _zeroActive;
	//std::chrono::steady_clock::time_point _zeroStart;

	//std::deque<std::pair<
	//	std::chrono::steady_clock::time_point,
	//	std::chrono::duration<double>>> _zeroHistory; // (기록시각, 지속시간)
	//std::deque<std::pair<
	//	std::chrono::steady_clock::time_point,
	//	int>> _moveHistory;

	//double GetZeroPeak(std::chrono::steady_clock::time_point now) const
	//{
	//	double peak = 0.0;

	//	for (const auto& pair : _zeroHistory) // 기록된 streak
	//	{
	//		double d = pair.second.count();
	//		if (d > peak)
	//			peak = d;
	//	}

	//	if (_zeroActive) // 진행 중인 streak도 반영
	//	{
	//		double live = std::chrono::duration<double>(now - _zeroStart).count();
	//		if (live > peak)
	//			peak = live;
	//	}

	//	return peak;
	//}

	//double GetMoveAvg() const
	//{
	//	int sum = 0;
	//	for (const auto& item : _moveHistory)
	//		sum += item.second;

	//	return static_cast<double>(sum); // 최근 1초 → 초당 합계 = 평균
	//}

	// Worker thread
	string _roomWorker;
	string _sendWorker;

	int32 _skillInstanceCount = 0;
	int32 _endCount = 0;
	int32 _hitCount = 0;

	// TEMP : SkillSystem
	//static const int WINDOW_SIZE = 10; // 10번 -> 1초
	//std::deque<int32> _castingStartHistory;
	//std::deque<int32> _castingSuccessHistory;
	//std::deque<int32> _actionHistory;

	//int32 GetCastingStartPerSec() const
	//{
	//	int32 sum = 0;
	//	for (auto c : _castingStartHistory)
	//		sum += c;
	//	return sum; // 이미 1초 윈도우라 바로 초당 값
	//}

	//int32 GetCastingSuccessPerSec() const
	//{
	//	int32 sum = 0;
	//	for (auto c : _castingSuccessHistory)
	//		sum += c;
	//	return sum;
	//}

	//int32 GetActionPerSec() const
	//{
	//	int32 sum = 0;
	//	for (auto c : _actionHistory)
	//		sum += c;
	//	return sum;
	//}
};