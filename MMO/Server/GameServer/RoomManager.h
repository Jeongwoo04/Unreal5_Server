#pragma once
#include "Room.h"
#include "ConfigManager.h"

class RoomManager
{
public:
	static RoomManager& Instance()
	{
		static RoomManager instance;
		return instance;
	}

	void Start(RoomConfig roomConfig);

	void Init(int32 count, int32 mapId, uint64 roomTick);
	//RoomRef Add(int32 count, int32 mapId);
	//bool Remove(int32 roomId);
	//RoomRef Find(int32 roomId);

	RoomRef FindUsableRoom();
	int32 GetSessionCount(int32 roomId);

	//void UpdateReserveAllRooms();

private:
	USE_LOCK;
	atomic<int32> _roomCounter = 0;
	vector<RoomRef> _rooms;

	//int32 _roomId = 1;	
	//unordered_map<int32, RoomRef> _rooms;
};