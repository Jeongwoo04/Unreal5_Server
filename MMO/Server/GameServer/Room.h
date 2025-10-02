#pragma once
#include "pch.h"
#include "SpatialHashGrid.h"

using GameMapRef = shared_ptr<class GameMap>;
using ObjectManagerRef = shared_ptr<class ObjectManager>;
using SkillSystemRef = shared_ptr<class SkillSystem>;
using RoomRef = shared_ptr<class Room>;
using MonsterRef = shared_ptr<class Monster>;
using ProjectileRef = shared_ptr<class Projectile>;
using FieldRef = shared_ptr<class Field>;

class Room : public JobQueue
{
public:
	Room();
	virtual ~Room();

	void SetRoomId(int32 roomId) { _roomId = roomId; }
	int32 GetRoomId() { return _roomId; }

	GameMapRef GetGameMap() { return _gameMap; }

	void Init(int32 mapId);
	void UpdateTick();
	void StartHeartbeat();
	void CheckHeartbeat();

	void SpawnInit();
	void SpawnMonster(int32 spTableId);
	void SpawnProjectile(ObjectRef owner, int32 dataId, const Vector3& pos, const Vector3& dir);
	void SpawnField(ObjectRef owner, int32 dataId, const Vector3& pos);

public:
	bool EnterRoom(ObjectRef object);
	bool LeaveRoom(ObjectRef object);
	bool LeaveGame(ObjectRef object, uint64 objectId);

	bool HandleEnterPlayer(GameSessionRef gameSession);
	bool HandleLeavePlayer(PlayerRef player);
	void HandleMovePlayer(Protocol::C_MOVE pkt);
	void HandleSkill(PlayerRef player, Protocol::C_SKILL pkt);

public:
	const SpawnTable* GetSpawnTable(int32 spawnId) const;

private:
	bool AddObject(ObjectRef object);
	bool RemoveObject(ObjectRef object, uint64 objectId);

public:
	void Broadcast(SendBufferRef sendBuffer, uint64 exceptId = 0);
	void BroadcastNearby(SendBufferRef sendBuffer, const Vector2Int& center, uint64 exceptId = 0);
	void BroadcastMove(const Protocol::PosInfo& posInfo, uint64 exceptId = 0);

	void NotifySpawn(ObjectRef object, bool success);

public:
	void AddRemoveList(ObjectRef object);
	void ClearRemoveList();

public:
	GameMapRef _gameMap;
	SpatialHashGrid<PlayerRef> _playerGrid;
	SpatialHashGrid<MonsterRef> _monsterGrid;

public:
	ObjectManagerRef _objectManager;
	SkillSystemRef _skillSystem;

private:
	unordered_map<uint64, PlayerRef> _players;
	unordered_map<uint64, MonsterRef> _monsters;
	unordered_map<uint64, ProjectileRef> _projectiles;
	unordered_map<uint64, FieldRef> _fields;

	vector<ObjectRef> _removePending;

	MapInfo _mapInfo;
	int32 _roomId = 0;

	const float BROADCAST_RANGE = 20.f; // 카메라뷰 + 여유분
};