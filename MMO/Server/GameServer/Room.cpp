#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "Arrow.h"
#include "ObjectManager.h"
#include "GameSessionManager.h"

Room::Room()
{
	_gameMap = make_shared<GameMap>();
	_objectManager = make_shared<ObjectManager>();
	_objectManager->Init();
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

	SpawnInit();

	UpdateTick();
}

void Room::UpdateTick()
{
	for (auto& m : _monsters)
	{
		m.second->Update();
	}
	for (auto& p : _projectiles)
	{
		p.second->Update();
	}
	ClearRemoveList();

	DoTimer(100, &Room::UpdateTick);
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

	NotifyDespawn(object, objectId);

	return success;
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
	return LeaveRoom(player);
}

void Room::HandleMove(Protocol::C_MOVE pkt)
{
	const uint64 objectId = pkt.info().object_id();
	if (_players.find(objectId) == _players.end())
	{
		return;
	}
	
	PlayerRef& player = _players[objectId];
	if (player == nullptr || player->GetRoom() == nullptr || player->GetRoom()->GetGameMap() == nullptr)
	{
		return;
	}

	Vector2Int destPos = Vector2Int(pkt.info());

	if (!_gameMap->CanGo(destPos, false))
	{
		return;
	}

	_playerGrid.ApplyMove(player, player->_gridPos, destPos);

	player->SetPosInfo(pkt.info());

	{
		Protocol::S_MOVE movePkt;
		
		movePkt.mutable_info()->CopyFrom(pkt.info());

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);
		Broadcast(sendBuffer);
	}
}

void Room::HandleSkill(PlayerRef player, Protocol::C_SKILL pkt)
{
	if (player == nullptr)
		return;

	//if (player->_posInfo.state() != Protocol::STATE_MACHINE_IDLE)
	//	return;

	const auto& skillInfo = pkt.info();

	player->_posInfo.set_state(Protocol::STATE_MACHINE_SKILL);
	S_SKILL skillPkt;
	skillPkt.set_object_id(player->GetId());
	skillPkt.mutable_skill_info()->set_skillid(2);

	{
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(skillPkt);
		Broadcast(sendBuffer);
	}

	auto it = DataManager::Instance().SkillDict.find(pkt.info().skillid());
	if (it == DataManager::Instance().SkillDict.end())
		return;

	const Skill& skillData = it->second;

	switch (skillData.skillType)
	{
	case Protocol::SKILL_NONE:
		break;
	case Protocol::SKILL_AUTO:
	{
		const float attackRange = 1.f * CELL_SIZE;     // 평타 사거리
		const float monsterRadius = 42.f;              // 몬스터 반지름
		const float totalRadius = attackRange + monsterRadius;

		const float halfDegree = 45.f;
		const float cosThreshold = cosf(halfDegree * (PI / 180.f));

		const Vector3 attWorldPos = Vector3(player->_posInfo);
		const float yaw = player->_posInfo.yaw();

		const Vector3 forward(cosf(yaw), sinf(yaw), player->_posInfo.z()); // _x, _y 사용, _z는 무시

		const int32 gridRadius = static_cast<int32>(ceil(totalRadius / CELL_SIZE));
		const Vector2Int attGridPos(player->_posInfo);

		vector<MonsterRef> candidates = _monsterGrid.FindAround(attGridPos, gridRadius);

		for (MonsterRef monster : candidates)
		{
			if (monster == nullptr)
				continue;

			Vector3 targetWorldPos = Vector3(monster->_posInfo);
			Vector3 targetDir = targetWorldPos - attWorldPos;

			float distSq = targetDir.LengthSquared();  // _x, _y만 사용한 길이
			if (distSq > totalRadius * totalRadius)
				continue;

			Vector3 dir = targetDir.Normalized(); // _x, _y만 정규화
			float dot = Vector3::Dot(dir, forward);

			if (dot >= cosThreshold)
			{
				monster->OnDamaged(player, player->_statInfo.attack() + skillData.damage);
			}
		}
	}
		break;
	case Protocol::SKILL_PROJECTILE:
	{
		ProjectileRef proj = static_pointer_cast<Projectile>(_objectManager->Spawn(30101, false, player->_posInfo));
		if (proj == nullptr)
			return;

		proj->SetOwner(player);
		proj->SetData(skillData);

		proj->_posInfo.set_state(Protocol::STATE_MACHINE_MOVING);
		proj->_statInfo.set_speed(proj->_projectileInfo.speed());

		EnterRoom(static_pointer_cast<Object>(proj));
	}
		break;
	case Protocol::SKILL_AOE_DOT:
		break;
	}

	player->SetState(Protocol::STATE_MACHINE_IDLE);
	S_MOVE movePkt;
	movePkt.mutable_info()->CopyFrom(player->_posInfo);
	auto sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);
	BroadcastMove(sendBuffer);
}

RoomRef Room::GetRoomRef()
{
	return static_pointer_cast<Room>(shared_from_this());
}

void Room::BroadcastMove(SendBufferRef sendBuffer, uint64 exceptId)
{
	Broadcast(sendBuffer, exceptId);
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

	object->SetRoom(GetRoomRef());

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
	default:
		break;
	}

	return true;
}

bool Room::RemoveObject(ObjectRef object, uint64 objectId)
{
	if (object == nullptr)
		return false;

	object->SetRoom(nullptr);

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
	}

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

void Room::NotifyDespawn(ObjectRef object, uint64 objectId)
{
	// 나에게 퇴장 패킷 보내기
	if (object->_objectInfo.creature_type() == Protocol::CREATURE_TYPE_PLAYER)
	{
		PlayerRef player = static_pointer_cast<Player>(object);

		Protocol::S_LEAVE_GAME leaveGamePkt;

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(leaveGamePkt);
		if (auto session = player->GetSession())
			session->Send(sendBuffer);
	}

	// 다른 플레이어에게 object의 퇴장 알리기
	{
		Protocol::S_DESPAWN despawnPkt;
		despawnPkt.add_object_ids(objectId);
		
		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(despawnPkt);
		Broadcast(sendBuffer, objectId);
	}
}

void Room::AddRemoveList(ObjectRef object)
{
	_removePending.push_back(object);
}

void Room::ClearRemoveList()
{
	for (auto it : _removePending)
		LeaveRoom(it);

	_removePending.clear();
}
