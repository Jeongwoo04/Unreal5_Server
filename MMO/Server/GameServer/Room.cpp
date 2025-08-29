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
	_gameMap->LoadGameMap(mapId);

	SpawnMonster();

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

	SpawnMonster();

	DoTimer(100, &Room::UpdateTick);
}

void Room::SpawnMonster()
{
	int32 maxMonsterGen = 1;
	int32 requireCount = 0;

	requireCount = maxMonsterGen - static_cast<int32>(_monsters.size());
	if (requireCount > 0)
	{
		for (int32 i = 0; i < requireCount; i++)
		{
			MonsterRef monster = static_pointer_cast<Monster>(_objectManager->Spawn("Goblin"));
			EnterRoom(monster, true);
		}
	}
}

void Room::AssignRandomPos(ObjectRef object)
{
	if (object->_objectInfo.creature_type() == Protocol::CREATURE_TYPE_PLAYER)
	{
		object->_posInfo.set_x(Utils::GetRandom(0.f, 100.f));
		object->_posInfo.set_y(Utils::GetRandom(0.f, 100.f));
		object->_posInfo.set_z(100.f);
		object->_posInfo.set_yaw(Utils::GetRandom(0.f, 100.f));
		object->_objectInfo.mutable_pos_info()->CopyFrom(object->_posInfo);
		object->_gridPos = Vector2Int(object->_posInfo);
		object->_worldPos = Vector3(object->_posInfo);

		PlayerRef player = static_pointer_cast<Player>(object);
		if (player->GetRoom() == nullptr)
			return;

		_playerGrid.ApplyAdd(player, player->_gridPos);
	}
	else if (object->_objectInfo.creature_type() == Protocol::CREATURE_TYPE_MONSTER)
	{
		object->_posInfo.set_x(Utils::GetRandom(2000.f, 3000.f));
		object->_posInfo.set_y(Utils::GetRandom(2000.f, 3000.f));
		object->_posInfo.set_z(100.f);
		object->_posInfo.set_yaw(Utils::GetRandom(0.f, 100.f));
		object->_objectInfo.mutable_pos_info()->CopyFrom(object->_posInfo);
		object->_gridPos = Vector2Int(object->_posInfo);
		object->_worldPos = Vector3(object->_posInfo);

		MonsterRef monster = static_pointer_cast<Monster>(object);
		if (monster->GetRoom() == nullptr)
			return;

		_monsterGrid.ApplyAdd(monster, monster->_gridPos);
	}
}

bool Room::EnterRoom(ObjectRef object, bool randPos /*= true*/)
{
	if (object == nullptr)
		return false;

	bool success = AddObject(object);

	// 랜덤 spawn
	if (randPos)
		AssignRandomPos(object);

	NotifySpawn(object, success);

	return success;
}

bool Room::LeaveRoom(ObjectRef object)
{
	if (object == nullptr)
		return false;

	const uint64 objectId = object->_objectInfo.object_id();

	//_gameMap->RemoveObject(object);
	if (object->_objectInfo.creature_type() == Protocol::CREATURE_TYPE_PLAYER)
	{
		PlayerRef player = static_pointer_cast<Player>(object);
		if (player->GetRoom() == nullptr)
			return false;

		_playerGrid.ApplyRemove(player, player->_gridPos);
	}
	else if (object->_objectInfo.creature_type() == Protocol::CREATURE_TYPE_MONSTER)
	{
		MonsterRef monster = static_pointer_cast<Monster>(object);
		if (monster->GetRoom() == nullptr)
			return false;

		_monsterGrid.ApplyRemove(monster, monster->_gridPos);
	}

	bool success = RemoveObject(object, objectId);

	NotifyDespawn(object, objectId);
	// TODO : GameMap에서 지워야하는 애들 처리

	return success;
}

bool Room::HandleEnterPlayer(GameSessionRef gameSession)
{
	PlayerRef player = static_pointer_cast<Player>(_objectManager->Spawn("Knight"));
	if (player == nullptr)
		return false;

	gameSession->_player = player;
	player->SetSession(gameSession);

	return EnterRoom(player, true);
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
		cout << "Can't find player" << endl;
		return;
	}
	
	PlayerRef& player = _players[objectId];
	if (player == nullptr || player->GetRoom() == nullptr || player->GetRoom()->GetGameMap() == nullptr)
	{
		cout << "Invalid player" << endl;
		return;
	}

	Vector2Int destPos = Vector2Int(pkt.info());

	if (!_gameMap->CanGo(destPos, false))
	{
		cout << "Can't move there" << endl;
		return;
	}

	_playerGrid.ApplyMove(player, player->_gridPos, destPos);

	player->_posInfo.CopyFrom(pkt.info());
	player->_gridPos = destPos;
	player->_worldPos = { pkt.info().x(), pkt.info().y() };

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

		const Vector3 forward(cosf(yaw), sinf(yaw)); // _x, _y 사용, _z는 무시

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
		ProjectileRef proj = static_pointer_cast<Projectile>(_objectManager->Spawn("Arrow"));
		if (proj == nullptr)
			return;

		proj->SetOwner(player);
		proj->SetData(skillData);

		proj->_posInfo.set_state(Protocol::STATE_MACHINE_MOVING);
		proj->_posInfo.set_yaw(player->_posInfo.yaw());
		proj->_posInfo.set_x(player->_posInfo.x());
		proj->_posInfo.set_y(player->_posInfo.y());
		proj->_posInfo.set_z(player->_posInfo.z());
		proj->_statInfo.set_speed(proj->_projectileInfo.speed());

		proj->_gridPos = Vector2Int(proj->_posInfo);
		proj->_worldPos = Vector3(proj->_posInfo);

		EnterRoom(static_pointer_cast<Object>(proj), false);
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
			_players[object->GetId()] = static_pointer_cast<Player>(object);
			break;
		case CREATURE_TYPE_MONSTER:
			_monsters[object->GetId()] = static_pointer_cast<Monster>(object);
			break;
		default:
			break;
		}
		break;
	case OBJECT_TYPE_PROJECTILE:
		_projectiles[object->GetId()] = static_pointer_cast<Projectile>(object);
		break;
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
			eraseCount = static_cast<int32>(_players.erase(objectId));
			break;
		case CREATURE_TYPE_MONSTER:
			eraseCount = static_cast<int32>(_monsters.erase(objectId));
			break;
		}
		break;
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
