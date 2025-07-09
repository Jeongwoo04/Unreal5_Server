#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"
#include "ObjectManager.h"

RoomRef GRoom = make_shared<Room>();

Room::Room()
{

}

Room::~Room()
{

}

void Room::Init()
{
	
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

	DoTimer(100, &Room::UpdateTick);
}

void Room::SpawnMonster()
{
	int32 maxMonsterGen = 5;
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
	object->_posInfo.set_x(Utils::GetRandom(0.f, 500.f));
	object->_posInfo.set_y(Utils::GetRandom(0.f, 500.f));
	object->_posInfo.set_z(100.f);
	object->_posInfo.set_yaw(Utils::GetRandom(0.f, 100.f));

	object->_objectInfo.mutable_pos_info()->CopyFrom(object->_posInfo);
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

	// TODO : validation
	player->_posInfo.CopyFrom(pkt.info());

	{
		Protocol::S_MOVE movePkt;
		// 위치 정보
		{
			movePkt.mutable_info()->CopyFrom(pkt.info());
		}

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);
		Broadcast(sendBuffer);
	}	
}

RoomRef Room::GetRoomRef()
{
	return static_pointer_cast<Room>(shared_from_this());
}

void Room::RegisterPlayer(PlayerRef player)
{
	_players[player->_objectInfo.object_id()] = player;
}

void Room::RegisterMonster(MonsterRef monster)
{
	_monsters[monster->_objectInfo.object_id()] = monster;
}

void Room::RegisterProjectile(ProjectileRef projectile)
{
	_projectiles[projectile->_objectInfo.object_id()] = projectile;
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
	object->SetRoom(nullptr);

	switch (object->GetObjectType())
	{
	case OBJECT_TYPE_CREATURE:
		switch (object->GetCreatureType())
		{
		case CREATURE_TYPE_PLAYER:
			_players.erase(objectId);
			break;
		case CREATURE_TYPE_MONSTER:
			_monsters.erase(objectId);
			break;
		default:
			break;
		}
		break;
	case OBJECT_TYPE_PROJECTILE:
		_projectiles.erase(objectId);
		break;
	default:
		break;
	}

	return true;
}

void Room::Broadcast(SendBufferRef sendBuffer, uint64 exceptId)
{
	for (auto& it : _players)
	{
		PlayerRef player = it.second;
		if (player->_objectInfo.object_id() == exceptId)
			continue;
		
		if (GameSessionRef gameSession = player->_session.lock())
			gameSession->Send(sendBuffer);
	}
}

void Room::NotifySpawn(ObjectRef object, bool success)
{
	if (object->GetCreatureType() != CREATURE_TYPE_PLAYER)
		return;

	PlayerRef player = static_pointer_cast<Player>(object);

	// 나에게 입장 알림
	{
		Protocol::S_ENTER_GAME enterGamePkt;
		enterGamePkt.set_success(success);
		enterGamePkt.mutable_player()->CopyFrom(object->_objectInfo);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(enterGamePkt);
		if (auto session = player->_session.lock())
			session->Send(sendBuffer);
	}

	// 다른 플레이어에게 나의 입장 알림
	{
		Protocol::S_SPAWN spawnPkt;

		auto info = spawnPkt.add_players();
		info->CopyFrom(object->_objectInfo);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
		Broadcast(sendBuffer, object->_objectInfo.object_id());
	}

	// 나에게 이미 존재하는 플레이어들을 알림
	{
		Protocol::S_SPAWN spawnPkt;

		for (auto& it : _players)
		{
			if (it.first == object->_objectInfo.object_id())
				continue;

			auto info = spawnPkt.add_players();
			info->CopyFrom(it.second->_objectInfo);
		}

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
		if (auto session = player->_session.lock())
			session->Send(sendBuffer);
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
		if (auto session = player->_session.lock())
			session->Send(sendBuffer);
	}

	// 타인에게 퇴장 사실을 알리기
	{
		Protocol::S_DESPAWN despawnPkt;
		despawnPkt.add_object_ids(objectId);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(despawnPkt);
		Broadcast(sendBuffer, objectId);

		if (auto player = dynamic_pointer_cast<Player>(object))
			if (auto session = player->_session.lock())
				session->Send(sendBuffer);
	}
}
