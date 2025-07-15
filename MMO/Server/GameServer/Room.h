#pragma once

class Room : public JobQueue
{
public:
	Room();
	virtual ~Room();

	void SetRoomId(int32 roomId) { _roomId = roomId; }
	int32 GetRoomId() { return _roomId; }

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
	void BroadcastMove(SendBufferRef sendBuffer, uint64 exceptId = 0);

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

	int32 _roomId = 0;
};

extern RoomRef GRoom;