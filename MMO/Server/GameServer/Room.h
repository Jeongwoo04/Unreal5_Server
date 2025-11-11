#pragma once
#include "pch.h"
#include "SpatialGrid.h"
#include "BenchmarkLogger.h"
#include <optional>

using GameMapRef = shared_ptr<class GameMap>;
using ObjectManagerRef = shared_ptr<class ObjectManager>;
using SkillSystemRef = shared_ptr<class SkillSystem>;
using RoomRef = shared_ptr<class Room>;
using MonsterRef = shared_ptr<class Monster>;
using ProjectileRef = shared_ptr<class Projectile>;
using FieldRef = shared_ptr<class Field>;
using GameSessionRef = shared_ptr<class GameSession>;

struct MapInfo;
struct SpawnTable;

enum class Type
{
	SPAWN = 1,
	MOVE = 2,
	CAST_START = 3,
	CAST_CANCEL = 4,
	CAST_SUCCESS = 5,
	SKILL_ACTION = 6,
	HIT = 7,
	DIE = 8,	
	DESPAWN = 9
};

struct FlushQueue
{
	ObjectRef object;
	Type type;
	optional<Protocol::S_SKILL_EVENT> eventInfo;
};

/*
struct InterestDiff
{
	vector<Vector2Int> spawns;
	vector<Vector2Int> despawns;
	vector<Vector2Int> moves;
};

struct BroadcastGroup
{
	vector<Protocol::ObjectInfo> _spawns;
	vector<uint64> _despawns;
	vector<Protocol::PosInfo> _moves;
	vector<SendBufferRef> _batchBuffers;

	void Clear()
	{
		//_spawns.clear();
		//_despawns.clear();
		_moves.clear();
		_batchBuffers.clear();
	}

	bool IsEmpty()
	{
		return _spawns.empty() && _despawns.empty() && _moves.empty();
	}

	void AddSpawn(const Protocol::ObjectInfo& obj) { _spawns.push_back(obj); }
	void AddDespawn(uint64 id) { _despawns.push_back(id); }
	void AddMove(const Protocol::PosInfo& pos) { _moves.push_back(pos); }
};
*/

class Room : public JobQueue
{
public:
	Room(string name);
	virtual ~Room();

	void SetRoomId(int32 roomId) { _roomId = roomId; }
	int32 GetRoomId() { return _roomId; }

	GameMapRef GetGameMap() { return _gameMap; }

	void Init(int32 mapId);
	//void InitBaseOffsets();
	void UpdateTick();
	void UpdateMonster();
	void UpdateProjectile();
	void UpdateField();
	void UpdateSkillSystem();

	void StartHeartbeat();
	void CheckHeartbeat();

	// TEMP: Command
	void Spawn(int32 dataId, bool randPos, Vector3 pos, int32 count);
	void Kill();
	void KillAll();
	void GetList();
	//

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
	void BroadcastNearby(SendBufferRef sendBuffer, const Vector3& center, uint64 exceptId = 0);
	void BroadcastMove(const Protocol::PosInfo& posInfo, uint64 exceptId = 0);

	/*
	InterestDiff UpdateInterestCell(const Vector2Int& oldCenter, const Vector2Int& newCenter);
	vector<Vector2Int> InterestCells(const Vector2Int& center) const;
	InterestDiff DiffInterestCells(const vector<Vector2Int>& oldCell, const vector<Vector2Int>& newCell, const Vector2Int& delta) const;
	*/

	//void AddSpawnFlushQueue(ObjectRef obj);
	//void AddMoveFlushQueue(ObjectRef obj);
	//void AddSkillFlushQueue(ObjectRef obj, const Protocol::CastState& state, const SkillEvent& event = {});
	//void AddHitFlushQueue(ObjectRef obj);
	//void AddDieFlushQueue(ObjectRef obj);
	//void AddDespawnFlushQueue(ObjectRef obj);

	bool IsEmptyImmediatePkt(const Protocol::S_IMMEDIATE_FLUSH& pkt);
	bool IsEmptyDeferPkt(const Protocol::S_DEFER_FLUSH& pkt);

	void FlushImmediateBroadcast();
	void FlushDeferBroadcast();

	void NotifySpawn(ObjectRef object, bool success);

public:
	void AddRemoveList(ObjectRef object);
	void ClearRemoveList();

	int32 Index(const Vector2Int& pos) const;

public:
	GameMapRef _gameMap;
	SpatialGrid<PlayerRef> _playerGrid;
	SpatialGrid<MonsterRef> _monsterGrid;

	const int32 BROADCAST_RANGE = 20; // 카메라뷰 + 여유분
	//vector<Vector2Int> _baseOffsets;

public:
	ObjectManagerRef _objectManager;
	SkillSystemRef _skillSystem;

public:
	unordered_map<uint64, PlayerRef> _players;
	unordered_map<uint64, MonsterRef> _monsters;
	unordered_map<uint64, ProjectileRef> _projectiles;
	unordered_map<uint64, FieldRef> _fields;

public:
	vector<FlushQueue> _immediateFlushQueue; // C_Packet
	vector<FlushQueue> _deferFlushQueue; // Room Object Update
	//vector<BroadcastGroup> _flushBCQueue;

private:
	vector<ObjectRef> _removePending;

	MapInfo* _mapInfo;
	int32 _roomId = 0;

public:
	BenchmarkStat _bench;
	uint64 _tickCount = 0;

private:
	shared_ptr<JobQueue> _broadcastQueue;

	//TEMP
public:
	std::chrono::high_resolution_clock::time_point _prevTime;
};
