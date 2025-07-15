#include "pch.h"
#include "RoomManager.h"
#include "ObjectManager.h"

RoomRef RoomManager::Add()
{
	RoomRef room = make_shared<Room>();

	WRITE_LOCK;
	{
		room->SetRoomId(_roomId);
		_rooms[_roomId] = room;
		_roomId++;
	}

	return room;
}

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

bool RoomManager::FindUsableRoom()
{
	if (_rooms.empty())
		return false;

	// TODO : Room 입장 허용치에 관해 처리.
	return true;
}

void RoomManager::Init()
{
	UpdateReserveAllRooms();
}

void RoomManager::UpdateReserveAllRooms()
{
	for (auto& it : _rooms)
	{
		RoomRef room = it.second;
		if (room == nullptr)
			return;
		room->DoAsync(&Room::UpdateTick);
	}
}
