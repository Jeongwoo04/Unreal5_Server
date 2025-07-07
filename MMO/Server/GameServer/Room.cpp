#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"

RoomRef GRoom = make_shared<Room>();

Room::Room()
{

}

Room::~Room()
{

}

bool Room::EnterRoom(ObjectRef object, bool randPos /*= true*/)
{
	bool success = AddObject(object);

	// 랜덤 spawn
	if (randPos)
	{
		object->_posInfo.set_x(Utils::GetRandom(0.f, 500.f));
		object->_posInfo.set_y(Utils::GetRandom(0.f, 500.f));
		object->_posInfo.set_z(100.f);
		object->_posInfo.set_yaw(Utils::GetRandom(0.f, 100.f));

		object->_objectInfo.mutable_pos_info()->CopyFrom(object->_posInfo);
	}

	// 나에게 입장 알림
	if (auto player = dynamic_pointer_cast<Player>(object))
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
	if (auto player = dynamic_pointer_cast<Player>(object))
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

	return success;
}

bool Room::LeaveRoom(ObjectRef object)
{
	if (object == nullptr)
		return false;

	const uint64 objectId = object->_objectInfo.object_id();
	bool success = RemoveObject(objectId);

	// 나에게 퇴장 패킷 보내기
	if (auto player = dynamic_pointer_cast<Player>(object))
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

void Room::UpdateTick()
{
	cout << "Update Room" << endl;

	DoTimer(100, &Room::UpdateTick);
}

RoomRef Room::GetRoomRef()
{
	return static_pointer_cast<Room>(shared_from_this());
}

bool Room::AddObject(ObjectRef object)
{
	// TODO : id generate -> find by id로 type확인
	PlayerRef player = static_pointer_cast<Player>(object);
	if (_players.find(player->_objectInfo.object_id()) != _players.end())
		return false;

	_players.insert(make_pair(player->_objectInfo.object_id(), player));

	player->_room = GetRoomRef();

	return true;
}

bool Room::RemoveObject(uint64 objectId)
{
	// TODO : id generate -> find by id로 type확인
	if (_players.find(objectId) == _players.end())
		return false;

	PlayerRef player = _players[objectId];
	player->_room.lock() = nullptr;

	_players.erase(objectId);

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
