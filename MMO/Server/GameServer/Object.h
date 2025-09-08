#pragma once
#include "pch.h"
#include "Room.h"
#include "Data.Contents.h"
#include "Protocol.pb.h"

using namespace Protocol;

using RoomRef = shared_ptr<class Room>;

class Object : public enable_shared_from_this<Object>
{
public:
	Object();
	virtual ~Object();

	virtual void Update();
	virtual void OnDamaged(ObjectRef attacker, int32 damage);
	virtual void OnDead(ObjectRef attacker);

	virtual Protocol::ObjectType GetObjectType() const { return _objectInfo.object_type(); }
	virtual Protocol::CreatureType GetCreatureType() const { return _objectInfo.creature_type(); }
	virtual Protocol::PlayerType GetPlayerType() const { return _objectInfo.player_type(); }

	void SetState(const Protocol::StateMachine& state) { _posInfo.set_state(state); }
	Protocol::StateMachine GetState() const { return _posInfo.state(); }

	void SetId(uint64 id);
	uint64 GetId() { return _objectInfo.object_id(); }

	void SetRoom(RoomRef room) { _room = room; }
	RoomRef GetRoom() const { return _room.lock(); }

	bool IsDead() { return GetState() == Protocol::STATE_MACHINE_DEAD; }

	void SetPos(const Vector3& pos);
	void SetPosInfo(const PosInfo& posInfo);
	void SetSpawnPos(const Vector3& pos, float yaw = 0.f);
	void SetSpawnRandomPos(Vector3 pos, float yaw = 0.f);

public: 
	Protocol::ObjectInfo _objectInfo;
	Protocol::PosInfo _posInfo;
	Protocol::StatInfo _statInfo;

	Vector2Int _gridPos;
	Vector3 _worldPos;

	Vector3 _dir;

	int32 _spTableId;

protected:
	Protocol::StateMachine _state = Protocol::STATE_MACHINE_IDLE;

	weak_ptr<Room> _room; // 스마트포인터는 set 할때 멀티스레드에서 위험
};

