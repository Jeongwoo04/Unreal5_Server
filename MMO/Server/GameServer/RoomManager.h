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

	void Init(int32 count, int32 mapId);
	//RoomRef Add(int32 count, int32 mapId);
	//bool Remove(int32 roomId);
	//RoomRef Find(int32 roomId);

	RoomRef FindUsableRoom();

	//void UpdateReserveAllRooms();

private:
	USE_LOCK;
	atomic<int32> _roomCounter = 0;
	vector<RoomRef> _rooms;

	//int32 _roomId = 1;	
	//unordered_map<int32, RoomRef> _rooms;
};