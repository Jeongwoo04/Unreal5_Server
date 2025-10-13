#include "pch.h"
#include "LoginSessionManager.h"
#include "LoginSession.h"

LoginSessionManager GLoginSessionManager;

void LoginSessionManager::Add(LoginSessionRef session)
{
	WRITE_LOCK;
	_sessions.insert(session);
}

void LoginSessionManager::Remove(LoginSessionRef session)
{
	WRITE_LOCK;
	_sessions.erase(session);
}

//void LoginSessionManager::Broadcast(SendBufferRef sendBuffer)
//{
//	WRITE_LOCK;
//	for (LoginSessionRef session : _sessions)
//	{
//		session->Send(sendBuffer);
//	}
//}