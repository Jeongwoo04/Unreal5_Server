#include "pch.h"
#include "RoomDiagnostics.h"

void RoomDiagnostics::BeginTick()
{
	auto now = std::chrono::steady_clock::now();

	if (_tickStart.time_since_epoch().count() != 0)
	{
		// 이전 Tick 시작 ~ 현재 Tick 시작까지 간격(ms)
		_tickIntervalMs = std::chrono::duration<double, std::milli>(now - _tickStart).count();
	}

	_tickStart = now;
}

void RoomDiagnostics::EndTick()
{
	auto end = std::chrono::steady_clock::now();
	_tickMs = std::chrono::duration<double, std::milli>(end - _tickStart).count();
}

void RoomDiagnostics::SetObjectCounts(int32 players, int32 monsters, int32 projectiles, int32 fields)
{
	_players = players;
	_monsters = monsters;
	_projectiles = projectiles;
	_fields = fields;
}

void RoomDiagnostics::SetImmediateFlushInfo(int32 flushQueueSize, int32 flushPktBytes)
{
	_immediateFlushQueue = flushQueueSize;
	_immediateFlushPktBytes = flushPktBytes;
}

void RoomDiagnostics::SetDeferFlushInfo(int32 flushQueueSize, int32 flushPktBytes)
{
	_deferFlushQueue = flushQueueSize;
	_deferFlushPktBytes = flushPktBytes;
}

// TEMP : Move Burst 확인
//void RoomDiagnostics::AddMoveCount(int32 moveCount)
//{
//	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
//
//	// Zero 시작
//	if (moveCount == 0)
//	{
//	    if (_zeroActive == false)
//	    {
//	        _zeroActive = true;
//	        _zeroStart = now;
//	    }
//	}
//	else
//	{
//	    // Zero 종료 → 기록
//	    if (_zeroActive)
//	    {
//	        std::chrono::duration<double> dur = now - _zeroStart;
//	        _zeroHistory.emplace_back(now, dur);
//	        _zeroActive = false;
//	    }
//	}
//
//	// 5초 이상 지난 기록 제거
//	while (!_zeroHistory.empty())
//	{
//	    auto age = now - _zeroHistory.front().first;
//	    if (age > std::chrono::seconds(5))
//	        _zeroHistory.pop_front();
//	    else
//	        break;
//	}
//
//	_moveHistory.emplace_back(now, moveCount);
//
//	// 최근 1초 유지
//	while (!_moveHistory.empty())
//	{
//	    auto age = now - _moveHistory.front().first;
//	    if (age > std::chrono::seconds(1))
//	        _moveHistory.pop_front();
//	    else
//	        break;
//	}
//}

// TEMP : SkillSystem
//void RoomDiagnostics::AddSkillInstance(int32 count)
//{
//	_skillInstanceCount = count;
//}
//
//void RoomDiagnostics::AddCastingStart(int32 count)
//{
//	_castingStartHistory.push_back(count);
//
//	if (_castingStartHistory.size() > WINDOW_SIZE)
//		_castingStartHistory.pop_front();
//}
//
//void RoomDiagnostics::AddCastingSuccess(int32 count)
//{
//	_castingSuccessHistory.push_back(count);
//
//	if (_castingSuccessHistory.size() > WINDOW_SIZE)
//		_castingSuccessHistory.pop_front();
//}
//
//void RoomDiagnostics::AddAction(int32 count)
//{
//	_actionHistory.push_back(count);
//
//	if (_actionHistory.size() > WINDOW_SIZE)
//		_actionHistory.pop_front();
//}
//
//void RoomDiagnostics::AddEnd(int32 count)
//{
//	_endCount = count;
//}
//
//void RoomDiagnostics::AddHit(int32 count)
//{
//	_hitCount = count;
//}

void RoomDiagnostics::AddSendDelay(double delayMs)
{
	auto now = std::chrono::steady_clock::now();

	_sendDelayLast = delayMs;

	// 최근 10초 기록 유지
	_sendDelays.push_back({ now, delayMs });

	// 10초 이상 지난 기록 제거
	while (!_sendDelays.empty() &&
		std::chrono::duration<double>(_sendDelays.front().first - now).count() < -10.0)
	{
		_sendDelays.pop_front();
	}

	// 최근 10초 peak 계산
	_sendDelayPeak = 0;
	double sum = 0.0;
	for (auto& p : _sendDelays)
	{
		sum += p.second;
		if (p.second > _sendDelayPeak)
			_sendDelayPeak = p.second;
	}

	// 최근 10초 평균 계산
	if (!_sendDelays.empty())
		_sendDelayAvg = sum / _sendDelays.size();
	else
		_sendDelayAvg = 0;
}

void RoomDiagnostics::SetRoomWorkerInfo(const string& roomWorker)
{
	_roomWorker = roomWorker;
}

void RoomDiagnostics::SetSendWorkerInfo(const string& sendWorker)
{
	_sendWorker = sendWorker;
}

void RoomDiagnostics::Render()
{
	// Clear console
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hConsole == INVALID_HANDLE_VALUE)
		return;

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD count;
	DWORD cellCount;
	COORD homeCoords = { 0, 0 };

	if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
		return;

	cellCount = csbi.dwSize.X * csbi.dwSize.Y;

	// 화면 지우기
	FillConsoleOutputCharacter(hConsole, ' ', cellCount, homeCoords, &count);
	FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, homeCoords, &count);

	// 커서 좌상단으로 이동
	SetConsoleCursorPosition(hConsole, homeCoords);

	printf("[ROOM]\n");
	printf("Tick: %.2f ms\n", _tickMs);
	printf("TickInterval: %.0f ms\n\n", _tickIntervalMs);

	printf("[OBJECT]\n");
	printf("Players: %d\n", _players);
	printf("Monsters: %d\n", _monsters);
	printf("Projectiles: %d\n", _projectiles);
	printf("Fields: %d\n\n", _fields);

	// TEMP : SkillSystem 
	//printf("[SKILL & HIT]\n");
	//printf("SkillInstance: %d\n", _skillInstanceCount);
	//printf("Remove: %d\n", _endCount);
	//printf("Hit: %d\n\n", _hitCount);
	//printf("Casting Start: %d / sec\n", GetCastingStartPerSec());
	//printf("Casting Success: %d / sec\n", GetCastingSuccessPerSec());
	//printf("Action: %d / sec\n\n", GetActionPerSec());


	printf("[NETWORK]\n");
	printf("ImmediateFlushQueue: %d\n", _immediateFlushQueue);
	printf("ImmediateFlushPktBytes: %d\n", _immediateFlushPktBytes);
	printf("DeferFlushQueue: %d\n", _deferFlushQueue);
	printf("DeferFlushPktBytes: %d\n", _deferFlushPktBytes);
	printf("SendDelay: %.2f ms\n", _sendDelayLast);
	printf("SendDelayAvg(10s): %.2f\n", _sendDelayAvg);
	printf("SendDelayPeak(10s): %.2f ms\n\n", _sendDelayPeak);

	// TEMP : Move Burst 재현
	//printf("[C_MOVE EXECUTE(1s)]\n");
	//printf("MoveCountAvg: %.0f \n", GetMoveAvg());
	//std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	//printf("MoveCountZeroDurationPeak(5s): %.2f ms\n\n", GetZeroPeak(now) * 1000.0);

	printf("[WORKER]\n");
	printf("RoomWorker: %s\n", _roomWorker.c_str());
	printf("SendWorker: %s\n", _sendWorker.c_str());
}