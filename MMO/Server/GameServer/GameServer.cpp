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

int main()
{
	ServerPacketHandler::Init();

	ConfigManager::Instance().LoadConfig("../Data/config.json");
	DataManager::Instance().LoadData("../Data");
	//for (int32 i = 0; i < 2; i++)
	//{
	for (int32 i = 0; i < 2; i++)
		RoomRef room = RoomManager::Instance().Add(1);
	//}
	
	ServerServiceRef service = make_shared<ServerService>(
		NetAddress(L"127.0.0.1", 7777),
		make_shared<IocpCore>(),
		[=]() { return make_shared<GameSession>(); },
		1000);

	ASSERT_CRASH(service->Start());

	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch([&service]()
			{
				DoIOWorker(service);
			});
	}

	for (int32 i = 0; i < 3; i++)
	{
		GThreadManager->Launch([]()
			{
				DoGameWorker(); 
			});
	}

	// Main Thread
	//DoWorkerJob(service);

	GThreadManager->Join();
}