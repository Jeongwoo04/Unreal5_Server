#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "Projectile.h"
#include "Field.h"
#include "ObjectManager.h"
#include "SkillSystem.h"

using namespace std::chrono;

Room::Room(string name) : JobQueue(name)
{
	_broadcastQueue = make_shared<JobQueue>("BC_" + name);

	_gameMap = make_shared<GameMap>();
	_objectManager = make_shared<ObjectManager>();
	_skillSystem = make_shared<SkillSystem>();
	_objectManager->Init();
	_skillSystem->Init();
}

Room::~Room()
{

}

void Room::Init(int32 mapId)
{
	auto mapIt = DataManager::Instance().MapDataDict.find(mapId);
	if (mapIt == DataManager::Instance().MapDataDict.end())
		return;

	_mapInfo = mapIt->second;
	_gameMap->LoadGameMap(_mapInfo.filePath);
	_skillSystem->SetRoom(static_pointer_cast<Room>(shared_from_this()));

	SpawnInit();

	UpdateTick();
	//StartHeartbeat();
}

void Room::UpdateTick()
{
	using namespace std::chrono;

	auto now = high_resolution_clock::now();
	auto diff = duration_cast<milliseconds>(now - _prevTime).count();
	_prevTime = now;

	if (diff > 0)
		std::cout << "[RoomTick] Interval: " << diff << " ms\n";

	constexpr float deltaTime = 0.1f;
	uint64 tick = static_cast<uint64>(deltaTime * 1000);
	DoTimer(tick, &Room::UpdateTick);

	_bench.Begin("Room");
	
	for (auto& m : _monsters)
	{
		m.second->Update(deltaTime);
	}
	for (auto& p : _projectiles)
	{
		p.second->Update(deltaTime);
	}
	for (auto& f : _fields)
	{
		f.second->Update(deltaTime);
	}
	_skillSystem->Update(deltaTime);

	ClearRemoveList();
	_bench.End("Room");

	_bench.PrintAndSaveSummary(GetRoomId(), "100 dummy BCQueue execute delay, Push BCJob");
}

void Room::StartHeartbeat()
{
	constexpr uint64 Tick = 10000;
	DoTimer(Tick, &Room::CheckHeartbeat);
}

void Room::CheckHeartbeat()
{
	//S_HEARTBEAT pkt;

	//pkt.set_servertime(GetTickCount64());
	//auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
	//Broadcast(sendBuffer);

	//constexpr uint64 Tick = 10000;
	//DoTimer(Tick, &Room::CheckHeartbeat);
}

// TEMP: Command Spawn
void Room::Spawn(int32 dataId, bool randPos, Vector3 pos, int32 count)
{
	for (int32 i = 0; i < count; i++)
	{
		auto obj = _objectManager->Spawn(dataId, true, pos);
		if (!obj)
			return;

		EnterRoom(obj);
	}
}

void Room::Kill()
{	
	Vector2Int centerInt = { 0,0 };
	Vector3 centerFloat = { 0,0,100 };
	auto monster = _monsterGrid.FindNearest(centerInt, 40.f, centerFloat);
	AddRemoveList(monster);
}

void Room::KillAll()
{
	for (auto& [id, monster] : _monsters)
	{
		AddRemoveList(monster);
	}
}

void Room::GetList()
{
	for (auto& [id, monster] : _monsters)
	{
		cout << "Monster ID: " << id << endl;
	}	
}

void Room::SpawnInit()
{
	for (auto& spawnIt : _mapInfo.spawnTables)
	{
		for (int32 i = 0; i < spawnIt.second.count; i++)
		{
			SpawnMonster(spawnIt.second.spawnId);
		}
	}
}

void Room::SpawnMonster(int32 spTableId)
{
	auto spTableData = _mapInfo.spawnTables.find(spTableId)->second;

	auto obj = _objectManager->Spawn(spTableData.dataId, true, spTableData.spawnPos);
	if (!obj)
		return;

	obj->_spTableId = spTableId;

	EnterRoom(obj);
}

void Room::SpawnProjectile(ObjectRef owner, int32 dataId, const Vector3& pos, const Vector3& dir)
{
	auto obj = _objectManager->Spawn(dataId, false, pos);
	if (!obj)
		return;
	else
	{
		auto pro = static_pointer_cast<Projectile>(obj);
		pro->SetOwner(owner);
		pro->SetDir(dir);
	}

	EnterRoom(obj);
}

void Room::SpawnField(ObjectRef owner, int32 dataId, const Vector3& pos)
{
	// TODO : EnterRoom switch문 추가
	auto obj = _objectManager->Spawn(dataId, false, pos);
	if (!obj)
		return;
	else
	{
		auto field = static_pointer_cast<Field>(obj);
		field->SetOwner(owner);
	}

	EnterRoom(obj);
}

bool Room::EnterRoom(ObjectRef object)
{
	if (object == nullptr)
		return false;

	bool success = AddObject(object);

	NotifySpawn(object, success);

	return success;
}

bool Room::LeaveRoom(ObjectRef object)
{
	if (object == nullptr)
		return false;

	const uint64 objectId = object->_objectInfo.object_id();

	bool success = RemoveObject(object, objectId);

	return success;
}

bool Room::LeaveGame(ObjectRef object, uint64 objectId)
{
	if (object == nullptr)
		return false;

	bool removed = LeaveRoom(object);

	if (removed)
	{
		// 성공적으로 Room에서 빠졌으니 클라에 알림
		Protocol::S_DESPAWN pkt;
		pkt.add_object_ids(object->GetId());
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
		Broadcast(sendBuffer);
	}

	// 나에게 퇴장 패킷 보내기
	if (object->_objectInfo.creature_type() == Protocol::CREATURE_TYPE_PLAYER)
	{
		PlayerRef player = static_pointer_cast<Player>(object);

		Protocol::S_LEAVE_GAME leaveGamePkt;

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(leaveGamePkt);
		if (auto session = player->GetSession())
			session->Send(sendBuffer);
	}

	return true;
}

bool Room::HandleEnterPlayer(GameSessionRef gameSession)
{
	//printf("[Server] Handle Enter Player\n");
	PlayerRef player = static_pointer_cast<Player>(_objectManager->Spawn(10101, true, {0, 0, 0}));
	if (player == nullptr)
		return false;

	gameSession->_player = player;
	player->SetSession(gameSession);

	return EnterRoom(player);
}

bool Room::HandleLeavePlayer(PlayerRef player)
{	
	return LeaveGame(static_pointer_cast<Object>(player), player->GetId());
}

void Room::HandleMovePlayer(Protocol::C_MOVE pkt)
{
	_bench.Begin("MovePrevSection");
	const uint64 objectId = pkt.info().object_id();
	if (_players.find(objectId) == _players.end())
		return;

	PlayerRef player = _players[objectId];
	if (player == nullptr || player->GetRoom() == nullptr || player->GetRoom()->GetGameMap() == nullptr)
		return;

	if (player->GetState() == Protocol::STATE_MACHINE_CASTING)
	{
		auto activeSkill = player->GetActiveSkill();
		if (activeSkill)
		{
			_skillSystem->CancelCasting(player, activeSkill->castId);
			cout << "[Server] CancelCasting\n";
		}
	}
	else if (player->GetState() == Protocol::STATE_MACHINE_SKILL)
	{
		cout << "[Server] Move Fail: Player state is Skill\n";
		return;
	}

	Vector3 destPos = Vector3(pkt.info());

	player->_posInfo.set_state(pkt.info().state());
	_bench.End("MovePrevSection");
	_bench.Begin("MoveToNextPos");
	player->MoveToNextPos(destPos);
	_bench.End("MoveToNextPos");
	_bench.Begin("BCJobPush");
	BroadcastMove(player->_posInfo);
	_bench.End("BCJobPush");
}

void Room::HandleSkill(PlayerRef player, Protocol::C_SKILL pkt)
{
	if (player == nullptr)
		return;

	int32 skillId = pkt.skill_info().skillid();

	auto it = DataManager::Instance().SkillDict.find(skillId);
	if (it == DataManager::Instance().SkillDict.end())
		return;

	const Skill& skillData = it->second;
	uint64 now = ::GetTickCount64();

	if (player->GetState() == Protocol::STATE_MACHINE_CASTING
		|| player->GetState() == Protocol::STATE_MACHINE_SKILL)
	{
		auto activeSkill = player->GetActiveSkill();
		if (activeSkill && pkt.castid() != activeSkill->castId)
		{
			_skillSystem->CancelCasting(player, activeSkill->castId);
		}
	}

	// 1. 사용 가능 여부 체크 (쿨타임, 캐스팅, 자원)
	if (!player->CanUseSkill(skillId, now))
	{
		return;
	}

	player->_posInfo.set_yaw(Vector3::DirToYaw2D(Vector3(pkt.mutable_skill_info()->targetpos()) - player->_worldPos));
	_skillSystem->ExecuteSkill(player, skillId, Vector3(pkt.skill_info().targetpos()), pkt.castid(), pkt.clientsend());
}

const SpawnTable* Room::GetSpawnTable(int32 spawnId) const
{
	auto it = _mapInfo.spawnTables.find(spawnId);
	if (it != _mapInfo.spawnTables.end())
		return &it->second;
	return nullptr;
}

bool Room::AddObject(ObjectRef object)
{
	if (object == nullptr)
		return false;

	object->SetRoom(static_pointer_cast<Room>(shared_from_this()));

	switch (object->GetObjectType())
	{
	case OBJECT_TYPE_CREATURE:
		switch (object->GetCreatureType())
		{
		case CREATURE_TYPE_PLAYER:
		{
			auto player = static_pointer_cast<Player>(object);
			_players[player->GetId()] = player;
			_playerGrid.ApplyAdd(player, player->_gridPos);
		}	break;
		case CREATURE_TYPE_MONSTER:
		{
			auto monster = static_pointer_cast<Monster>(object);
			_monsters[monster->GetId()] = monster;
			_monsterGrid.ApplyAdd(monster, monster->_gridPos);
		}	break;
		default:
			break;
		}
		break;
	case OBJECT_TYPE_PROJECTILE:
	{
		auto proj = static_pointer_cast<Projectile>(object);
		_projectiles[proj->GetId()] = proj;
	}	break;
	case OBJECT_TYPE_ENV:
	{
		auto field = static_pointer_cast<Field>(object);
		_fields[field->GetId()] = field;
	} break;
	default:
		break;
	}

	return true;
}

bool Room::RemoveObject(ObjectRef object, uint64 objectId)
{
	if (object == nullptr)
		return false;

	int32 eraseCount = 0;

	switch (object->GetObjectType())
	{
	case OBJECT_TYPE_CREATURE:
		switch (object->GetCreatureType())
		{
		case CREATURE_TYPE_PLAYER:
			{
				eraseCount = static_cast<int32>(_players.erase(objectId));
				auto player = static_pointer_cast<Player>(object);
				_playerGrid.ApplyRemove(player, player->_gridPos);
				break;
			}
		case CREATURE_TYPE_MONSTER:
			{
				eraseCount = static_cast<int32>(_monsters.erase(objectId));
				auto monster = static_pointer_cast<Monster>(object);
				_monsterGrid.ApplyRemove(monster, monster->_gridPos);
				break;
			}
		default:
			break;
		}
	case OBJECT_TYPE_PROJECTILE:
		eraseCount = static_cast<int32>(_projectiles.erase(objectId));
		break;
	case OBJECT_TYPE_ENV:
		eraseCount = static_cast<int32>(_fields.erase(objectId));
		break;
	}

	_objectManager->Despawn(object);
	object->SetRoom(nullptr);

	return eraseCount > 0 ? true : false;
}

void Room::Broadcast(SendBufferRef sendBuffer, uint64 exceptId)
{
	_bench.Begin("BC_AllSnapshot");
	vector<PlayerRef> snapshot;
	snapshot.reserve(_players.size());

	for (auto& [id, player] : _players)
	{
		if (player->_objectInfo.object_id() == exceptId)
			continue;

		snapshot.push_back(player);
	}
	_bench.End("BC_AllSnapshot");

	auto enqueueTime = GetTimeMs();
	_bench.Begin("BCAllJobPush");
	_broadcastQueue->Push(make_shared<Job>(
		[snapshot = std::move(snapshot), sendBuffer, enqueueTime, this]()
		{
			double executeTime = GetTimeMs();
			double delayMs = static_cast<double>(executeTime - enqueueTime);

			_bench.AddBCQDelay(delayMs);
			_bench.AddSendCount(snapshot.size());

			for (auto& player : snapshot)
			{
		if (auto session = player->GetSession())
			session->Send(sendBuffer);
	}
		}));
	_bench.End("BCAllJobPush");
}

void Room::BroadcastNearby(SendBufferRef sendBuffer, const Vector3& center, uint64 exceptId)
{
	_bench.Begin("BC_FindAround");
	vector<PlayerRef> nearbyPlayers =
		_playerGrid.FindAroundFloat(center, BROADCAST_RANGE);
	_bench.End("BC_FindAround");

	auto enqueueTime = GetTickCount64();
	_bench.Begin("BCNearJobPush");
	_broadcastQueue->Push(make_shared<Job>([nearbyPlayers = std::move(nearbyPlayers), sendBuffer, exceptId, enqueueTime, this]()
		{
			uint64 executeTime = GetTickCount64();
			double delayMs = static_cast<double>(executeTime - enqueueTime);

			_bench.AddBCQDelay(delayMs);
			_bench.AddSendCount(nearbyPlayers.size());

			for (auto& player : nearbyPlayers)
	{
				if (player->GetId() == exceptId)
			continue;
				if (auto session = player->GetSession())
			session->Send(sendBuffer);
	}
		}));
	_bench.End("BCNearJobPush");
}

void Room::BroadcastMove(const Protocol::PosInfo& posInfo, uint64 exceptId)
{
	Protocol::S_MOVE movePkt;

	movePkt.mutable_info()->CopyFrom(posInfo);

	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);

	BroadcastNearby(sendBuffer, Vector3(posInfo), exceptId);
}

void Room::NotifySpawn(ObjectRef object, bool success)
{
	// object가 player일 경우 본인에게 Enter 패킷 + 이미 존재하는 주변 object spawn
	if (object->GetCreatureType() == CREATURE_TYPE_PLAYER)
	{
		PlayerRef player = static_pointer_cast<Player>(object);

		{
			Protocol::S_ENTER_GAME enterGamePkt;
			enterGamePkt.set_success(success);
			enterGamePkt.mutable_object()->CopyFrom(object->_objectInfo);

			SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(enterGamePkt);
			if (auto session = player->GetSession())
				session->Send(sendBuffer);
		}
		{
			Protocol::S_SPAWN spawnPkt;

			for (auto& playerIt : _players)
			{
				if (playerIt.first == object->_objectInfo.object_id())
					continue;

				*spawnPkt.add_objects() = playerIt.second->_objectInfo;
			}
			for (auto& monsterIt : _monsters)
			{
				*spawnPkt.add_objects() = monsterIt.second->_objectInfo;
			}
			for (auto& projectileIt : _projectiles)
			{
				*spawnPkt.add_objects() = projectileIt.second->_objectInfo;
			}

			SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
			if (auto session = player->GetSession())
				session->Send(sendBuffer);
		}
	}

	// 다른 플레이어에게 object 입장 알림 (player, monster, projectile 입장 시)
	{
		Protocol::S_SPAWN spawnPkt;

		*spawnPkt.add_objects() = object->_objectInfo;

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
		Broadcast(sendBuffer, object->_objectInfo.object_id());
	}
}

void Room::AddRemoveList(ObjectRef object)
{
	_removePending.push_back(object);
}

void Room::ClearRemoveList()
{
	const int32 MAX_REMOVE_COUNT_PER_TICK = 100;
	int32 count = (int32)_removePending.size() > 100 ? 100 : (int32)_removePending.size();

	Protocol::S_DESPAWN pkt;

	for (int32 i = 0; i < count; i++)
	{
		LeaveRoom(_removePending[i]);
		pkt.add_object_ids(_removePending[i]->GetId());
	}

	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
	Broadcast(sendBuffer);

	_removePending.erase(_removePending.begin(), _removePending.begin() + count);
}