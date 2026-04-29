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

#include "Monster.h"

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

#include <psapi.h>
#pragma comment(lib, "psapi.lib")

void DoMonitoringWorker()
{
	while (true)
	{


		PROCESS_MEMORY_COUNTERS_EX pmc;
		// 현재 프로세스의 메모리 정보 가져오기
		if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
		{
			// WorkingSetSize: 현재 실제 물리 메모리 점유량 (RAM)
			// PrivateUsage: 이 프로세스에 할당된 가상 메모리 점유량 (Commit Charge)
			size_t physicalMem = pmc.WorkingSetSize;
			size_t virtualMem = pmc.PrivateUsage;

			std::cout << "========================================" << std::endl;
			std::cout << "[Memory Monitor]" << std::endl;
			std::cout << "Physical Memory (RAM): " << std::fixed << std::setprecision(2)
				<< (double)physicalMem / (1024 * 1024) << " MB" << std::endl;
			std::cout << "Virtual Memory (Commit): " << (double)virtualMem / (1024 * 1024) << " MB" << std::endl;

			// std::cout << "Active Objects: " << MemoryManager::Instance().GetActiveCount() << std::endl;
			// std::cout << "Chunk Count: " << MemoryManager::Instance().GetChunkCount() << std::endl;
			std::cout << "========================================" << std::endl;
		}
	}
}

int main()
{
	ServerPacketHandler::Init();

	ConfigManager::Instance().LoadConfig("../Data/config.json");
	DataManager::Instance().LoadData("../Data");

	RoomManager::Instance().Init(1, 1);
	
	ServerServiceRef service = make_shared<ServerService>(
//#ifdef _DEBUG
		NetAddress(L"127.0.0.1", 7777),
//#else
//		NetAddress(L"192.168.0.10", 7777),
//#endif
		make_shared<IocpCore>(),
		[=]() { return make_shared<GameSession>(); },
		10);

	ASSERT_CRASH(service->Start());
		
	for (int32 i = 0; i < 2; i++)
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

	for (int32 i = 0; i < 2; i++)
	{
		GThreadManager->Launch("SendWorker#" + to_string(i), []()
			{
				DoSendWorker();
			});
	}

	//GThreadManager->Launch("MemoryMonitoring#", []()
	//	{
	//		DoMonitoringWorker();
	//	});

	// Main Thread
	//DoGameWorker();

	GThreadManager->Join();
}