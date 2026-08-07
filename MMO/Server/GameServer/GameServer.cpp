#include "pch.h"
#include <iostream>
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include <tchar.h>
#include "Job.h"
#include "Room.h"
#include "RoomManager.h"
#include "DataManager.h"
#include "ConfigManager.h"
#include "CommandManager.h"

uint64 WORKER_TICK;

void DoIOWorker(ServerServiceRef& service)
{
	while (true)
	{
#ifdef BENCHMARK
		if (GBenchMarkManager->GetProcessState() == ProcessState::WAIT_WORKER_FLUSH && GBenchMarkManager->GetBenchRound() == LBenchRound)
		{
			GBenchMarkManager->AddIOData("kernelDelivery", std::move(LKernelDelivery));
			GBenchMarkManager->AddIOData("totalDelay", std::move(LTotalDelay));

			LKernelDelivery.clear();
			LKernelDelivery.reserve(10000);
			LTotalDelay.clear();
			LTotalDelay.reserve(10000);

			LBenchRound++;
		}
#endif

		// 네트워크 입출력 처리 -> 인게임 로직까지 (패킷 핸들러에 의해)
		service->GetIocpCore()->Dispatch(10);
	}
}

// 인게임 로직
void DoGameWorker()
{
	while (true)
	{
#ifdef BENCHMARK
		Time::LoopFrameTick();

		if (GBenchMarkManager->GetProcessState() == ProcessState::WAIT_WORKER_FLUSH && GBenchMarkManager->GetBenchRound() == LBenchRound)
		{
			GBenchMarkManager->AddWorkerData(LBenchRound, LThreadId, { WorkerType::LOGIC , LExecuteJobCount , LExecuteJobQueues , LWorkerActiveTime , LTimeSliceExceeded });

			LBenchRound++;
			LExecuteJobCount = 0;
			LExecuteJobQueues = 0;
			LWorkerActiveTime = 0;
			LTimeSliceExceeded = 0;
			//LImmediateEmpty = 0;
			//LDeferEmpty = 0;
		}
#endif

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
#ifdef BENCHMARK
		if (GBenchMarkManager->GetProcessState() == ProcessState::WAIT_WORKER_FLUSH && GBenchMarkManager->GetBenchRound() == LBenchRound)
		{
			GBenchMarkManager->AddWorkerData(LBenchRound, LThreadId, { WorkerType::SEND , LExecuteJobCount , LExecuteJobQueues , LWorkerActiveTime , LTimeSliceExceeded });
			GBenchMarkManager->AddIOData("QueueingDelay", std::move(LQueueingDelay));

			LQueueingDelay.clear();
			LQueueingDelay.reserve(10000);

			LBenchRound++;
			LExecuteJobCount = 0;
			LExecuteJobQueues = 0;
			LWorkerActiveTime = 0;
			LTimeSliceExceeded = 0;
		}
#endif

		LEndTickCount = ::GetTickCount64() + WORKER_TICK;

		// 글로벌 큐
		ThreadManager::DoGlobalSendQueueWork();
	}
}

void BenchMarksWriter()
{
	GBenchMarkManager->_roundTime.startTick = ::GetTickCount64();

	while (true)
	{
		switch (GBenchMarkManager->GetProcessState())
		{
		case ProcessState::COLLECT_ROOM:
		{
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
				GBenchMarkManager->SetProcessState(ProcessState::ABSTRACT_DATA);
			}
			break;
		}

		case ProcessState::ABSTRACT_DATA:
		{
			GBenchMarkManager->AbstractData();

			GBenchMarkManager->_roundTime.startTick = ::GetTickCount64();
			GBenchMarkManager->_roundTime.endTick = 0;
			break;
		}

		case ProcessState::CALCULATE:
		{
			cout << "CALCULATE\n";
			GBenchMarkManager->CalculateData();
			GBenchMarkManager->WriteCSV();
			GBenchMarkManager->SetProcessState(ProcessState::COLLECT_ROOM);
			break;
		}
		}

		this_thread::sleep_for(10ms);
	}
}

int main()
{
	GServerStartTick = ::GetTickCount64();
	ServerPacketHandler::Init();

	//ConfigManager::Instance().LoadConfig("../Data/config.json");
	//DataManager::Instance().LoadData("../Data");

	// 데이터 및 설정 파일 읽어오기
	ConfigManager::Instance().LoadConfig("../../Config");
	DataManager::Instance().LoadData("../../Data");

	const Config& config = ConfigManager::Instance().GetConfig();
	
	// Room 생성 및 시작
	RoomManager::Instance().Start(config._roomConfig);
	
	// 서버 서비스 시작
	ServerServiceRef service = make_shared<ServerService>(
		NetAddress(config._serverConfig.IP, config._serverConfig.Port),
		make_shared<IocpCore>(),
		[=]() { return make_shared<GameSession>(); },
		config._serverConfig.MaxSession);

	ASSERT_CRASH(service->Start());

	WORKER_TICK = config._serverConfig.WorkerTick;

	// 스레드 풀에 워커 생성 및 시작
	for (int32 i = 0; i < config._threadConfig.IO; i++)
	{
		GThreadManager->Launch("IOWorker#" + to_string(i), [&service]()
			{
				DoIOWorker(service);
			});
	}

	for (int32 i = 0; i < config._threadConfig.LOGIC; i++)
	{
		GThreadManager->Launch("GameWorker#" + to_string(i), []()
			{
				DoGameWorker(); 
			});
	}

	for (int32 i = 0; i < config._threadConfig.SEND; i++)
	{
		GThreadManager->Launch("SendWorker#" + to_string(i), []()
			{
				DoSendWorker();
			});
	}

#ifdef BENCHMARK
	GThreadManager->Launch("BenchMarkWorker#", []()
		{
			BenchMarksWriter();
		});
#endif

	int32 _runTime = 0;
	while (true)
	{
		cout << "Server Runtime  : " << _runTime++ << endl;

		this_thread::sleep_for(1s);
	}

	// Main Thread
	//DoGameWorker();

	GThreadManager->Join();
}