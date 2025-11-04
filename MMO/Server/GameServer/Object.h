#pragma once
#include "pch.h"
#include "Room.h"
#include "Data.Contents.h"
#include "Protocol.pb.h"

using namespace Protocol;

using RoomRef = shared_ptr<class Room>;
struct SkillInstance;

struct BuffInstance
{
	ObjectRef target = nullptr;
	const BuffInfo* buff = nullptr;
	float duration = 0.f;
	float effectPerTick = 0.f;
};

class Object : public enable_shared_from_this<Object>
{
public:
	Object();
	virtual ~Object();

	virtual void Update(float deltaTime);
	virtual void OnDamaged(ObjectRef attacker, int32 damage);
	virtual void OnDead(ObjectRef attacker);
	virtual void MoveToNextPos(const Vector3& destPos, Vector3* dir = nullptr, Vector2Int* blocked = nullptr);

	virtual Protocol::ObjectType GetObjectType() const { return _objectInfo.object_type(); }
	virtual Protocol::CreatureType GetCreatureType() const { return _objectInfo.creature_type(); }
	virtual Protocol::PlayerType GetPlayerType() const { return _objectInfo.player_type(); }

	void ChangeState(const Protocol::StateMachine& state);
	Protocol::StateMachine GetState() const { return _posInfo.state(); }

	void SetId(uint64 id);
	uint64 GetId() { return _objectInfo.object_id(); }

	void SetRoom(RoomRef room) { _room = room; }
	RoomRef GetRoom() const { return _room.lock(); }

	bool IsDead() { return GetState() == Protocol::STATE_MACHINE_DEAD; }

	void AddBuff(const BuffInfo& buff);
	void UpdateBuffs();
	void SetPosInfo(const PosInfo& posInfo);
	void SetSpawnPos(const Vector3& pos, float yaw = 0.f);
	void SetSpawnRandomPos(Vector3 pos, float yaw = 0.f, int32 range = 0);

	bool IsMoveBatch();

public: 
	Protocol::ObjectInfo _objectInfo;
	Protocol::PosInfo _posInfo;
	Protocol::StatInfo _statInfo;

	Vector2Int _gridPos;
	Vector3 _worldPos;
	Vector3 _lastFlushPos;

	vector<Vector2Int> _interestCell;

	int32 _spTableId;

	float _collisionRadius;

protected:
	weak_ptr<Room> _room;

	// TEMP : BuffSystem
private:
	vector<BuffInstance> _activeBuffs;
	void ApplyBuffEffect(BuffInstance& buff, float deltaTime);
};

