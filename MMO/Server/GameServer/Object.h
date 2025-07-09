#pragma once
#include "Room.h"

using namespace Protocol;

class Object : public enable_shared_from_this<Object>
{
public:
	Object();
	virtual ~Object();

	virtual void Update();

	virtual ObjectType GetObjectType() const { return _objectInfo.object_type(); }
	virtual CreatureType GetCreatureType() const { return _objectInfo.creature_type(); }
	virtual PlayerType GetPlayerType() const { return _objectInfo.player_type(); }

	void SetId(uint64 id);
	uint64 GetId() { return _id; }
	RoomRef GetRoom() { return _room.lock(); }
	void SetRoom(RoomRef room) { _room = room; }

public: 
	Protocol::ObjectInfo _objectInfo;
	Protocol::PosInfo _posInfo;

protected:
	uint64 _id = 0;
	weak_ptr<Room> _room; // 스마트포인터는 set 할때 멀티스레드에서 위험
};

