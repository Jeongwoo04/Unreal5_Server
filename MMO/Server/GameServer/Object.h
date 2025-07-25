#pragma once
#include "pch.h"
#include "Room.h"
#include "Protocol.pb.h"

using namespace Protocol;

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

	void SetState(Protocol::StateMachine& state) { _posInfo.set_state(state); }
	Protocol::StateMachine GetState() const { return _posInfo.state(); }

	void SetMoveType(Protocol::MoveState& type) { _posInfo.set_move_type(type); }
	Protocol::MoveState GetMoveType() const { return _posInfo.move_type(); }

	void SetId(uint64 id);
	uint64 GetId() { return _id; }

	void SetRoom(RoomRef room) { _room = room; }
	RoomRef GetRoom() const { return _room.lock(); }

	void ApplyPos();

public: 
	Protocol::ObjectInfo _objectInfo;
	Protocol::PosInfo _posInfo;

	Vector2Int _gridPos;
	Vector3 _worldPos;

	Vector3 _dir;

protected:
	Protocol::StateMachine _state = Protocol::STATE_MACHINE_IDLE;
	Protocol::MoveState _moveState = Protocol::MOVE_STATE_NONE;

	uint64 _id = 0;
	weak_ptr<Room> _room; // 스마트포인터는 set 할때 멀티스레드에서 위험
};

