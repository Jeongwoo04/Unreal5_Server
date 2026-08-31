#pragma once

class RoomDiagnostics
{
public:
	void BeginTick();
	void EndTick();

	// Object counts
	void SetObjectCounts(pair<int32,int32>& players, pair<int32,int32>& monsters, pair<int32,int32>& projectiles, pair<int32,int32> fields);

	// Network status
	void SetImmediateFlushInfo(int32 flushPktBytes);
	void SetDeferFlushInfo(int32 flushPktBytes);
	
	// Send room data
	void SendRoomData(const vector<pair<int32,int32>>&& temp, int32 roomId);	

private:
	// Tick measurement
	std::chrono::steady_clock::time_point _tickStart;
	deque<std::pair<std::chrono::steady_clock::time_point, double>> _sendDelays;

	deque<std::pair<std::chrono::steady_clock::time_point, int32>> _moveCounts;

	double _tickMs = 0.0;
	double _tickIntervalMs = 0.0; // ¿¹»ó°ª

	// Object
	pair<int32, int32> _players = {};
	pair<int32, int32> _monsters = {};
	pair<int32, int32> _projectiles = {};
	pair<int32, int32> _fields = {};

	// Network
	int32 _immediateFlushPktBytes = 0;
	int32 _deferFlushPktBytes = 0;
};