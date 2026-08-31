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

void RoomDiagnostics::SetObjectCounts(pair<int32, int32>& players, pair<int32, int32>& monsters, pair<int32, int32>& projectiles, pair<int32, int32> fields)
{
	_players = players;
	_monsters = monsters;
	_projectiles = projectiles;
	_fields = fields;
}

void RoomDiagnostics::SetImmediateFlushInfo(int32 flushPktBytes)
{
	_immediateFlushPktBytes = flushPktBytes;
}

void RoomDiagnostics::SetDeferFlushInfo(int32 flushPktBytes)
{
	_deferFlushPktBytes = flushPktBytes;
}

void RoomDiagnostics::SendRoomData(const vector<pair<int32,int32>>&& temp, int32 roomId)
{
	RoomLog snap;

	snap.roomID = roomId;
	snap.tickCost = _tickMs;
	snap.tickInterval = _tickIntervalMs;
	snap.players = temp[0];
	snap.monsters = temp[1];
	snap.projectiles = temp[2];
	snap.fields = temp[3];
	snap.ImmediatePktBiteSize = _immediateFlushPktBytes;
	snap.DeferPktBiteSize = _deferFlushPktBytes;

	GRoomLog = std::move(snap);
}