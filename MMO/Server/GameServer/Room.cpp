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

bool Room::HandleEnterPlayer(PlayerRef player)
{
	bool success = EnterPlayer(player);

	// 랜덤 spawn
	player->info.set_x(Utils::GetRandom(0.f, 500.f));
	player->info.set_y(Utils::GetRandom(0.f, 500.f));
	player->info.set_z(100.f);
	player->info.set_yaw(Utils::GetRandom(0.f, 100.f));

	// 나에게 입장 알림
	{
		Protocol::S_ENTER_GAME enterGamePkt;
		enterGamePkt.set_success(success);
		enterGamePkt.mutable_player()->CopyFrom(player->info);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(enterGamePkt);
		if (auto session = player->_session.lock())
			session->Send(sendBuffer);
	}

	// 다른 플레이어에게 나의 입장 알림
	{
		Protocol::S_SPAWN spawnPkt;

		auto info = spawnPkt.add_players();
		info->CopyFrom(player->info);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
		Broadcast(sendBuffer, player->info.object_id());
	}

	// 나에게 이미 존재하는 플레이어들을 알림
	{
		Protocol::S_SPAWN spawnPkt;

		for (auto& it : _players)
		{
			if (it.first == player->info.object_id())
				continue;

			auto info = spawnPkt.add_players();
			info->CopyFrom(it.second->info);
		}

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
		if (auto session = player->_session.lock())
			session->Send(sendBuffer);
	}

	return success;
}

bool Room::HandleLeavePlayer(PlayerRef player)
{
	if (player == nullptr)
		return false;

	const uint64 objectId = player->info.object_id();
	bool success = LeavePlayer(objectId);

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

		if (auto session = player->_session.lock())
			session->Send(sendBuffer);
	}

	return success;
}

void Room::HandleMove(Protocol::C_MOVE pkt)
{
	const uint64 objectId = pkt.info().object_id();
	if (_players.find(objectId) == _players.end())
		return;
	
	PlayerRef& player = _players[objectId];

	// TODO : validation
	player->info.CopyFrom(pkt.info());

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

bool Room::EnterPlayer(PlayerRef player)
{
	if (_players.find(player->info.object_id()) != _players.end())
		return false;

	_players.insert(make_pair(player->info.object_id(), player));

	player->_room = GetRoomRef();

	return true;
}

bool Room::LeavePlayer(uint64 objectId)
{
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
		if (player->info.object_id() == exceptId)
			continue;
		
		if (GameSessionRef gameSession = player->_session.lock())
			gameSession->Send(sendBuffer);
	}
}
