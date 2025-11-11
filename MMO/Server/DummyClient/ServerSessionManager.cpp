#include "pch.h"
#include "ServerSessionManager.h"
#include "ServerSession.h"

ServerSessionManager GSessionManager;

void ServerSessionManager::Add(ServerSessionRef session)
{
	WRITE_LOCK;
	_sessions.insert(session);
}

void ServerSessionManager::Remove(ServerSessionRef session)
{
	WRITE_LOCK;
	_sessions.erase(session);
}

void ServerSessionManager::Broadcast(SendBufferRef sendBuffer)
{
	WRITE_LOCK;
	for (ServerSessionRef session : _sessions)
	{
		session->Send(sendBuffer);
	}
}

void ServerSessionManager::SendRandomPos()
{
	for (auto& session : _sessions)
	{
		float dx = (rand() % 3 - 1) * 100;
		float dy = (rand() % 3 - 1) * 100;

		auto tmpPos = session->_posInfo;
		if (tmpPos.x() + dx > 4000.f || tmpPos.x() + dx < -4000.f)
			continue ;
		if (tmpPos.y() + dy > 4000.f || tmpPos.y() + dy < -4000.f)
			continue ;

		tmpPos.set_x(tmpPos.x() + dx);
		tmpPos.set_y(tmpPos.y() + dy);
		session->_posInfo.CopyFrom(tmpPos);

		Protocol::C_MOVE pkt;
		{
			pkt.mutable_info()->CopyFrom(tmpPos);
		}

		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
		session->Send(sendBuffer);
	}
}
