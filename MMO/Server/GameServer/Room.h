#pragma once
#include "pch.h"
#include "SpatialHashGrid.h"

using GameMapRef = shared_ptr<class GameMap>;
using ObjectManagerRef = shared_ptr<class ObjectManager>;
using RoomRef = shared_ptr<class Room>;
using MonsterRef = shared_ptr<class Monster>;
using ProjectileRef = shared_ptr<class Projectile>;
using ArrowRef = shared_ptr<class Arrow>;

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

	void SpawnInit();
	void SpawnMonster(int32 spTableId);
	void SpawnProjectile(int32 dataId, const Vector3& pos);
	void SpawnField(int32 dataId, const Vector3& pos);

public:
	bool EnterRoom(ObjectRef object);
	bool LeaveRoom(ObjectRef object);

	bool HandleEnterPlayer(GameSessionRef gameSession);
	bool HandleLeavePlayer(PlayerRef player);
	void HandleMovePlayer(Protocol::C_MOVE pkt);
	void HandleSkill(PlayerRef player, Protocol::C_SKILL pkt);

public:
	RoomRef GetRoomRef();
	const SpawnTable* GetSpawnTable(int32 spawnId) const;

private:
	bool AddObject(ObjectRef object);
	bool RemoveObject(ObjectRef object, uint64 objectId);

public:
	void Broadcast(SendBufferRef sendBuffer, uint64 exceptId = 0);
	void BroadcastMove(const Protocol::PosInfo& posInfo, uint64 objectId = 0);

	void NotifySpawn(ObjectRef object, bool success);
	void NotifyDespawn(ObjectRef object, uint64 objectId);

public:
	void AddRemoveList(ObjectRef object);
	void ClearRemoveList();

public:
	GameMapRef _gameMap;
	SpatialHashGrid<PlayerRef> _playerGrid;
	SpatialHashGrid<MonsterRef> _monsterGrid;

public:
	ObjectManagerRef _objectManager;

private:
	unordered_map<uint64, PlayerRef> _players;
	unordered_map<uint64, MonsterRef> _monsters;
	unordered_map<uint64, ProjectileRef> _projectiles;

	vector<ObjectRef> _removePending;

	//SpatialHashGrid<ProjectileRef> _projectileGrid;

	MapInfo _mapInfo;
	int32 _roomId = 0;
};