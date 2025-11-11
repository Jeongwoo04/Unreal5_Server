#include "pch.h"
#include "ClientPacketHandler.h"
#include "ServerSession.h"
#include "BufferReader.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];
using ServerSessionRef = shared_ptr<class ServerSession>;

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

bool Handle_S_LOGIN(PacketSessionRef& session, Protocol::S_LOGIN& pkt)
{
	for (auto& Player : pkt.players())
	{
		const Protocol::ObjectInfo& info = Player;
	}

	Protocol::C_ENTER_GAME EnterGamePkt;
	EnterGamePkt.set_playerindex(0);
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(EnterGamePkt);
	session->Send(sendBuffer);

	return true;
}

bool Handle_S_ENTER_GAME(PacketSessionRef& session, Protocol::S_ENTER_GAME& pkt)
{
	//cout << "S_ENTER_GAME" << endl;

	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);
	serverSession->_objectInfo = pkt.object();
	serverSession->_posInfo = pkt.object().pos_info();

	return true;
}

bool Handle_S_LEAVE_GAME(PacketSessionRef& session, Protocol::S_LEAVE_GAME& pkt)
{
	//cout << "S_LEAVE_GAME" << endl;

	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	return true;
}

bool Handle_S_SPAWN(PacketSessionRef& session, Protocol::S_SPAWN& pkt)
{
	//cout << "S_SPAWN" << endl;

	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	return true;
}

bool Handle_S_DESPAWN(PacketSessionRef& session, Protocol::S_DESPAWN& pkt)
{
	//cout << "S_DESPAWN" << endl;

	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	return true;
}

bool Handle_S_MOVE(PacketSessionRef& session, Protocol::S_MOVE& pkt)
{
	//cout << "S_MOVE" << endl;

	static DWORD64 lastLog = GetTickCount64();

	DWORD64 now = GetTickCount64();
	if (now - lastLog >= 1000) // 1초가 지났으면
	{
		SafeLog(to_string(pkt.info_size()));
		lastLog = now;        // 마지막 로그 시간 갱신
	}

	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);
	
	return true;
}

bool Handle_S_ACTION(PacketSessionRef& session, Protocol::S_ACTION& pkt)
{
	//cout << "S_ACTION" << endl;

	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	return true;
}

bool Handle_S_SKILL_EVENT(PacketSessionRef& session, Protocol::S_SKILL_EVENT& pkt)
{
	//cout << "S_SKILL_EVENT" << endl;

	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	return true;
}

bool Handle_S_SKILL(PacketSessionRef& session, Protocol::S_SKILL& pkt)
{
	//cout << "S_SKILL" << endl;

	static DWORD64 slastLog = GetTickCount64();

	DWORD64 now = GetTickCount64();
	if (now - slastLog >= 1000) // 1초가 지났으면
	{
		SafeLog(to_string(pkt.event_size()));
		slastLog = now;        // 마지막 로그 시간 갱신
	}

	SafeLog(to_string(pkt.event_size()));

	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	return true;
}

bool Handle_S_SKILL_CAST_START(PacketSessionRef& session, Protocol::S_SKILL_CAST_START& pkt)
{
	//cout << "S_SKILL_CAST_START" << endl;

	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	return true;
}

bool Handle_S_SKILL_CAST_SUCCESS(PacketSessionRef& session, Protocol::S_SKILL_CAST_SUCCESS& pkt)
{
	//cout << "S_SKILL_CAST_SUCCESS" << endl;

	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	return true;
}

bool Handle_S_SKILL_CAST_CANCEL(PacketSessionRef& session, Protocol::S_SKILL_CAST_CANCEL& pkt)
{
	//cout << "S_SKILL_CAST_CANCEL" << endl;

	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	return true;
}

bool Handle_S_HIT(PacketSessionRef& session, Protocol::S_HIT& pkt)
{
	//cout << "S_HIT" << endl;

	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	return true;
}

bool Handle_S_DIE(PacketSessionRef& session, Protocol::S_DIE& pkt)
{
	//cout << "S_DIE" << endl;

	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	return true;
}

bool Handle_S_HEARTBEAT(PacketSessionRef& session, Protocol::S_HEARTBEAT& pkt)
{
	//cout << "S_HEARTBEAT" << endl;

	ServerSessionRef serverSession = static_pointer_cast<ServerSession>(session);

	return true;
}

bool Handle_S_IMMEDIATE_FLUSH(PacketSessionRef& session, Protocol::S_IMMEDIATE_FLUSH& pkt)
{
	Protocol::S_MOVE move = pkt.move_pkt();
	Protocol::S_SKILL skill = pkt.skill_pkt();

	auto movecount = move.info_size();
	auto skillcount = skill.event_size();

	Handle_S_MOVE(session, move);
	Handle_S_SKILL(session, skill);

	return true;
}

bool Handle_S_DEFER_FLUSH(PacketSessionRef& session, Protocol::S_DEFER_FLUSH& pkt)
{
	Protocol::S_MOVE move = pkt.move_pkt();
	Protocol::S_SKILL skill = pkt.skill_pkt();
	Protocol::S_SPAWN spawn = pkt.spawn_pkt();
	Protocol::S_DESPAWN despawn = pkt.despawn_pkt();
	Protocol::S_HIT hit = pkt.hit_pkt();
	Protocol::S_DIE die = pkt.die_pkt();

	auto movecount = move.info_size();
	auto skillcount = skill.event_size();

	Handle_S_MOVE(session, move);
	Handle_S_SKILL(session, skill);
	//Handle_S_SKILL(session, skill);
	//Handle_S_SKILL(session, skill);
	//Handle_S_SKILL(session, skill);

	return true;
}