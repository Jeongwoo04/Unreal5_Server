#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "Projectile.h"
#include "Field.h"
#include "ObjectManager.h"
#include "SkillSystem.h"

Room::Room()
{
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
	StartHeartbeat();
}

void Room::UpdateTick()
{
	constexpr float deltaTime = 0.1f;

	_bench.Begin("Monster");
	for (auto& m : _monsters)
	{
		m.second->Update(deltaTime);
	} _bench.End("Monster");

	_bench.Begin("Projectile");
	for (auto& p : _projectiles)
	{
		p.second->Update(deltaTime);
	} _bench.End("Projectile");
	
	_bench.Begin("Field");
	for (auto& f : _fields)
	{
		f.second->Update(deltaTime);
	} _bench.End("Field");

	_bench.Begin("SkillSystem");
	_skillSystem->Update(deltaTime);
	_bench.End("SkillSystem");

	_bench.Begin("RemoveList");
	ClearRemoveList();
	_bench.End("RemoveList");

	if (++_tickCount % 100 == 0)
		_bench.PrintAndSaveSummary("RoomBenchmark.csv");

	DoTimer(static_cast<int32>(deltaTime * 1000), &Room::UpdateTick);
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

	// 나에게 퇴장 패킷 보내기
	if (object->_objectInfo.creature_type() == Protocol::CREATURE_TYPE_PLAYER)
	{
		PlayerRef player = static_pointer_cast<Player>(object);

		Protocol::S_LEAVE_GAME leaveGamePkt;

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(leaveGamePkt);
		if (auto session = player->GetSession())
			session->Send(sendBuffer);
	}
	if (removed)
	{
		// 성공적으로 Room에서 빠졌으니 클라에 알림
		Protocol::S_DESPAWN pkt;
		pkt.add_object_ids(object->GetId());
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
		Broadcast(sendBuffer);
	}

	return true;
}

bool Room::HandleEnterPlayer(GameSessionRef gameSession)
{
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
		}
	}
	else if (player->GetState() == Protocol::STATE_MACHINE_SKILL)
		return;
		
	Vector3 destPos = Vector3(pkt.info());

	player->_posInfo.set_state(pkt.info().state());
	player->MoveToNextPos(destPos);
	BroadcastMove(player->_posInfo);
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
		cout << "Can't Use Skill" << endl;
		return;
	}

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
	for (auto& it : _players)
	{
		PlayerRef player = it.second;
		if (player->_objectInfo.object_id() == exceptId)
			continue;
		
		if (auto session = player->GetSession())
			session->Send(sendBuffer);
	}
}

void Room::BroadcastNearby(SendBufferRef sendBuffer, const Vector2Int& center, uint64 exceptId)
{
	vector<PlayerRef> nearbyPlayers = _playerGrid.FindAroundFloat(center, BROADCAST_RANGE);

	for (auto& player : nearbyPlayers)
	{
		if (player->GetId() == exceptId)
			continue;
		if (auto session = player->GetSession())
			session->Send(sendBuffer);
	}
}

void Room::BroadcastMove(const Protocol::PosInfo& posInfo, uint64 exceptId)
{
	Protocol::S_MOVE movePkt;

	movePkt.mutable_info()->CopyFrom(posInfo);

	SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);

	BroadcastNearby(sendBuffer, Vector2Int(posInfo), exceptId);
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

				auto info = spawnPkt.add_objects();
				info->CopyFrom(playerIt.second->_objectInfo);
			}
			for (auto& monsterIt : _monsters)
			{
				auto info = spawnPkt.add_objects();
				info->CopyFrom(monsterIt.second->_objectInfo);
			}
			for (auto& projectileIt : _projectiles)
			{
				auto info = spawnPkt.add_objects();
				info->CopyFrom(projectileIt.second->_objectInfo);
			}

			SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
			if (auto session = player->GetSession())
				session->Send(sendBuffer);
		}
	}

	// 다른 플레이어에게 object 입장 알림 (player, monster, projectile 입장 시)
	{
		Protocol::S_SPAWN spawnPkt;

		auto info = spawnPkt.add_objects();
		info->CopyFrom(object->_objectInfo);
		info->mutable_pos_info()->CopyFrom(object->_posInfo);

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