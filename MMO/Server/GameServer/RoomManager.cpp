#include "pch.h"
#include "RoomManager.h"
#include "ObjectManager.h"
#include "ConfigManager.h"

void RoomManager::Start(RoomConfig roomConfig)
{
	Init(roomConfig.MaxRoom, roomConfig.MapID, roomConfig.RoomTick);
}

void RoomManager::Init(int32 count, int32 mapId, uint64 roomTick)
{
	{
		WRITE_LOCK;
		int32 id = 0;
		while (id < count)
		{
			RoomRef room = make_shared<Room>(roomTick, "Room#" + to_string(id));

			room->SetRoomId(id++);
			_rooms.push_back(room);

			room->DoAsyncPushOnly(&Room::Init, mapId);
		}
	}
}

int32 RoomManager::GetSessionCount(int32 roomId)
{
	return _rooms[roomId]->_players.size();
}

/*
bool RoomManager::Remove(int32 roomId)
{
	WRITE_LOCK;
	return _rooms.erase(roomId) > 0;
}

RoomRef RoomManager::Find(int32 roomId)
{
	WRITE_LOCK;
	auto it = _rooms.find(roomId);
	if (it != _rooms.end())
		return it->second;

	return nullptr;
}
*/

RoomRef RoomManager::FindUsableRoom()
{
	if (_rooms.empty())
		return nullptr;

	int32 index = _roomCounter.fetch_add(1);

	int32 targetIndex = index % _rooms.size();

	return _rooms[targetIndex];

	//return true;
}

// 현재 Add -> Init -> Timer로 진행중.
//void RoomManager::UpdateReserveAllRooms()
//{
	//for (auto& it : _rooms)
	//{
	//	RoomRef room = it.second;
	//	if (room == nullptr)
	//		return;
	//	room->DoAsync(&Room::UpdateTick);
	//}
//}
