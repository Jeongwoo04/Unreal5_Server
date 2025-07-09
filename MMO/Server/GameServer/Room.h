#pragma once

class Room : public JobQueue
{
public:
	Room();
	virtual ~Room();

	void Init();
	void UpdateTick();

	void SpawnMonster();
	void AssignRandomPos(ObjectRef object);

public:
	bool EnterRoom(ObjectRef object, bool randPos = true);
	bool LeaveRoom(ObjectRef object);

	bool HandleEnterPlayer(PlayerRef player);
	bool HandleLeavePlayer(PlayerRef player);
	void HandleMove(Protocol::C_MOVE pkt);

public:

	RoomRef GetRoomRef();

	void RegisterPlayer(PlayerRef player);
	void RegisterMonster(MonsterRef monster);
	void RegisterProjectile(ProjectileRef projectile);

private:
	bool AddObject(ObjectRef object);
	bool RemoveObject(ObjectRef object, uint64 objectId);

private:
	void Broadcast(SendBufferRef sendBuffer, uint64 exceptId = 0);
	void NotifySpawn(ObjectRef object, bool success);
	void NotifyDespawn(ObjectRef object, uint64 objectId);

private:
	unordered_map<uint64, PlayerRef> _players;
	unordered_map<uint64, MonsterRef> _monsters;
	unordered_map<uint64, ProjectileRef> _projectiles;
};

extern RoomRef GRoom;