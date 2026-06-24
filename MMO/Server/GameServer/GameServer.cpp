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
		if (GBenchMarkManager->GetProcessState() == ProcessState::WAIT_WORKER_FLUSH && GBenchMarkManager->GetBenchRound() == LBenchRound)
		{
			GBenchMarkManager->AddWorkerData(LBenchRound, LThreadId, { LExecuteJobCount , LExecuteJobQueues , LWorkerActiveTime , LTimeSliceExceeded });

			LBenchRound++;
			LExecuteJobCount = 0;
			LExecuteJobQueues = 0;
			LWorkerActiveTime = 0;
			LTimeSliceExceeded = 0;
		}

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

void BenchMarksWriter()
{
	while (true)
	{
		switch (GBenchMarkManager->GetProcessState())
		{
		case ProcessState::COLLECT_ROOM:
		{
			if (GBenchMarkManager->_roundTime.startTick == 0)
			{
				GBenchMarkManager->_roundTime.startTick = GetTickCount64();
			}

			if (GBenchMarkManager->CheckRoundRoom())
			{
				GBenchMarkManager->SetProcessState(ProcessState::WAIT_WORKER_FLUSH);
			}
			break;
		}

		case ProcessState::WAIT_WORKER_FLUSH:
		{
			if (GBenchMarkManager->CheckRoundWorker())
			{
				GBenchMarkManager->_roundTime.endTick = ::GetTickCount64();
				GBenchMarkManager->SetProcessState(ProcessState::CALCULATE);
			}
			break;
		}

		case ProcessState::CALCULATE:
		{
			GBenchMarkManager->CalculateData();
			GBenchMarkManager->_roundTime.startTick = 0;
			GBenchMarkManager->_roundTime.endTick = 0;
			GBenchMarkManager->WriteCSV();
			GBenchMarkManager->SetProcessState(ProcessState::COLLECT_ROOM);
			break;
		}
		}

		this_thread::sleep_for(10ms);
	}
}

#include <psapi.h>


int main()
{
	ServerPacketHandler::Init();

	ConfigManager::Instance().LoadConfig("../Data/config.json");
	DataManager::Instance().LoadData("../Data");

	//ConfigManager::Instance().LoadConfig("../../Data/config.json");
	//DataManager::Instance().LoadData("../../Data");

	RoomManager::Instance().Init(250, 1);
	
	ServerServiceRef service = make_shared<ServerService>(
//#ifdef _DEBUG
		//NetAddress(L"0.0.0.0", 7777),
		NetAddress(L"127.0.0.1", 7777),
//#else
//		NetAddress(L"192.168.0.10", 7777),
//#endif
		make_shared<IocpCore>(),
		[=]() { return make_shared<GameSession>(); },
		100);

	ASSERT_CRASH(service->Start());
		
	for (int32 i = 0; i < 2; i++)
	{
		GThreadManager->Launch("IOWorker#" + to_string(i), [&service]()
			{
				DoIOWorker(service);
			});
	}

	for (int32 i = 0; i < 2; i++)
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

	GThreadManager->Launch("BenchMarkWorker#", []()
		{
			BenchMarksWriter();
		});

	//GThreadManager->Launch("MemoryMonitoring#", []()
	//	{
	//		DoMonitoringWorker();
	//	});

	// Main Thread
	//DoGameWorker();

	GThreadManager->Join();
}