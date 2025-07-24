#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "ObjectManager.h"

Room::Room()
{
	_gameMap = make_shared<GameMap>();
}

Room::~Room()
{

}

void Room::Init(int32 mapId)
{
	_gameMap->LoadGameMap(mapId);

	SpawnMonster();

	DoTimer(100, &Room::UpdateTick);
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

	SpawnMonster();

	DoTimer(100, &Room::UpdateTick);
}

PlayerRef Room::FindPlayer(const function<bool(ObjectRef)>& condition)
{
	for (auto& it : _players)
	{
		if (condition(it.second))
			return it.second;
	}

	return nullptr;
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
			MonsterRef monster = ObjectManager::Instance().Add<Monster>();
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
	}
	else if (object->_objectInfo.creature_type() == Protocol::CREATURE_TYPE_MONSTER)
	{
		object->_posInfo.set_x(Utils::GetRandom(2000.f, 3000.f));
		object->_posInfo.set_y(Utils::GetRandom(2000.f, 3000.f));
		object->_posInfo.set_z(100.f);
		object->_posInfo.set_yaw(Utils::GetRandom(0.f, 100.f));
	}

	object->_objectInfo.mutable_pos_info()->CopyFrom(object->_posInfo);
	object->SetCellPos(object->_posInfo);
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
	bool success = RemoveObject(object, objectId);

	NotifyDespawn(object, objectId);

	return success;
}

bool Room::HandleEnterPlayer(PlayerRef player)
{	
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
		return;
	
	PlayerRef& player = _players[objectId];

	Vector2Int gridPos = WorldToGrid(pkt.info());

	if (!_gameMap->CanGo(gridPos))
		return;

	player->_posInfo.CopyFrom(pkt.info());
	player->_gridPos = gridPos;
	//player->SetV3Pos(player-> _posInfo);

	{
		Protocol::S_MOVE movePkt;
		
		movePkt.mutable_info()->CopyFrom(pkt.info());

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);
		Broadcast(sendBuffer);
	}
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
	if (object->GetCreatureType() != CREATURE_TYPE_PLAYER)
		return;
	PlayerRef player = static_pointer_cast<Player>(object);

	// object가 player일 경우 본인에게 Enter 패킷 + 이미 존재하는 주변 object spawn
	{
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

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
		Broadcast(sendBuffer, object->_objectInfo.object_id());
	}
}

void Room::NotifyDespawn(ObjectRef object, uint64 objectId)
{
	if (object->GetCreatureType() != CREATURE_TYPE_PLAYER)
		return;

	PlayerRef player = static_pointer_cast<Player>(object);

	// 나에게 퇴장 패킷 보내기
	{
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
