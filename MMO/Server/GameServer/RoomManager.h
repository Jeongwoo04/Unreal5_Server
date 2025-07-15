#pragma once
#include "Room.h"

class RoomManager
{
public:
	static RoomManager& Instance()
	{
		static RoomManager instance;
		return instance;
	}

	RoomRef Add();
	bool Remove(int32 roomId);
	RoomRef Find(int32 roomId);

	bool FindUsableRoom();

	void Init();
	void UpdateReserveAllRooms();

private:
	USE_LOCK;

	int32 _roomId = 1;	
	unordered_map<int32, RoomRef> _rooms;
};