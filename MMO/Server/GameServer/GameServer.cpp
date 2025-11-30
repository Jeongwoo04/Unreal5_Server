#include "pch.h"
#include <iostream>
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "GameSession.h"
#include "GameSessionManager.h"
//#include "ClientPacketHandler.h"
#include <tchar.h>
#include "Job.h"
#include "Room.h"
#include "RoomManager.h"
#include "DataManager.h"
#include "ConfigManager.h"
#include "CommandManager.h"

enum
{
	WORKER_TICK = 64
};

void DoIOWorker(ServerServiceRef& service)
{
	while (true)
	{
		// 네트워크 입출력 처리 -> 인게임 로직까지 (패킷 핸들러에 의해)
		service->GetIocpCore()->Dispatch(10);
	}
}

// 인게임 로직
void DoGameWorker()
{
	while (true)
	{
		LEndTickCount = ::GetTickCount64() + WORKER_TICK;

		// 예약된 일감 처리
		ThreadManager::DistributeReservedJobs();

		// 글로벌 큐
		ThreadManager::DoGlobalQueueWork();
	}
}

void DoSendWorker()
{
	while (true)
	{
		LEndTickCount = ::GetTickCount64() + WORKER_TICK;

		// 글로벌 큐
		ThreadManager::DoGlobalSendQueueWork();
	}
}

int main()
{
	ServerPacketHandler::Init();

	ConfigManager::Instance().LoadConfig("../Data/config.json");
	DataManager::Instance().LoadData("../Data");
	
	ServerServiceRef service = make_shared<ServerService>(
#ifdef _DEBUG
		NetAddress(L"127.0.0.1", 7777),
#else
		NetAddress(L"192.168.0.10", 7777),
#endif
		make_shared<IocpCore>(),
		[=]() { return make_shared<GameSession>(); },
		10);

	ASSERT_CRASH(service->Start());
		
	for (int32 i = 0; i < 4; i++)
	{
		GThreadManager->Launch("IOWorker#" + to_string(i), [&service]()
			{
				DoIOWorker(service);
			});
	}

	for (int32 i = 0; i < 3; i++)
	{
		GThreadManager->Launch("GameWorker#" + to_string(i), []()
			{
				DoGameWorker(); 
			});
	}

	for (int32 i = 0; i < 1; i++)
	{
		GThreadManager->Launch("SendWorker#" + to_string(i), []()
			{
				DoSendWorker();
			});
	}
	
	RoomRef room = RoomManager::Instance().Add(1);

	// Main Thread
	//DoGameWorker();

	GThreadManager->Join();
}