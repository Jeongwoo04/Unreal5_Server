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
#include "ConsoleUtils.h"

uint64 WORKER_TICK;

//void DoRenderingWorker()
//{
//	HANDLE hScreens[2];
//	hScreens[0] = GetStdHandle(STD_OUTPUT_HANDLE);
//
//	hScreens[1] = CreateConsoleScreenBuffer(
//		GENERIC_READ | GENERIC_WRITE,
//		FILE_SHARE_READ | FILE_SHARE_WRITE,
//		NULL,
//		CONSOLE_TEXTMODE_BUFFER,
//		NULL
//	);
//
//	if (hScreens[1] == INVALID_HANDLE_VALUE)
//	{
//		std::cerr << "콘솔 버퍼 생성 실패!" << std::endl;
//		return;
//	}
//
//	// 커서 깜빡임 숨기기 (선택사항이지만 꺼두는 게 훨씬 깔끔합니다)
//	CONSOLE_CURSOR_INFO cursorInfo;
//	GetConsoleCursorInfo(hScreens[0], &cursorInfo);
//	cursorInfo.bVisible = FALSE;
//	SetConsoleCursorInfo(hScreens[0], &cursorInfo);
//	SetConsoleCursorInfo(hScreens[1], &cursorInfo);
//
//	int currentBufferIndex = 0;
//
//	while (true)
//	{
//		RoomLog roomLog = GRoomLog;
//		RoundLog roundLog = GRoundLog;
//
//		// 2. 이번에 그려야 할 '안 보이는 쪽 버퍼(Back Buffer)' 선택
//		int backBufferIndex = 1 - currentBufferIndex;
//		HANDLE hTargetBuffer = hScreens[backBufferIndex];
//
//		// 3. 백 버퍼의 커서를 맨 위(0, 0)로 이동
//		COORD homeCoords = { 0, 0 };
//		SetConsoleCursorPosition(hTargetBuffer, homeCoords);
//
//		// 4. 출력을 쏠 때 printf는 기본 stdout(0번)으로 가므로, 
//		//    백 버퍼에 정확히 쓰려면 SetStdHandle로 표준 출력을 잠시 바꿔치기합니다.
//		SetStdHandle(STD_OUTPUT_HANDLE, hTargetBuffer);
//
//		// --- 여기서부터 출력 로직 ---
//		printf("[ROOM]\n");
//		printf("Tick Cost: %.2f ms\n", roomLog.tickCost);
//		printf("Tick Interval: %.0f ms\n\n", roomLog.tickInterval);
//
//		printf("[OBJECT]\n");
//		printf("Player] Chunk Count : %d, Player Count : %d\n", roomLog.players.first, roomLog.players.second);
//		printf("Monster] Chunk Count : %d, Monster Count : %d\n", roomLog.monsters.first, roomLog.monsters.second);
//		printf("Projectile] Chunk Count : %d, Projectile Count : %d\n", roomLog.projectiles.first, roomLog.projectiles.second);
//		printf("Fields] Chunk Count : %d, Field Count %d\n\n", roomLog.fields.first, roomLog.fields.second);
//
//		printf("[NETWORK]\n");
//		printf("ImmediateFlushPktBytes: %d\n", roomLog.immediateBytes);
//		printf("DeferFlushPktBytes: %d\n\n", roomLog.deferBytes);
//
//		if (roundLog.round > 0)
//		{
//			printf("[ROUND BENCH] : %d Round\n\n", roundLog.round);
//
//			printf("[TOTAL ROOM]\n");
//			printf("Tick Cost: %.4f\n", roundLog.totalRoom.at("Room"));
//			printf("Tick Interval: %.4f\n", roundLog.totalRoom.at("TickInterval"));
//			printf("Update Cost: %.4f\n\n", roundLog.totalRoom.at("Update"));
//
//			printf("[ROOM SEND JOB]\n");
//			printf("Total Delay: %.4f\n", roundLog.totalRoom.at("totalDelay"));
//			printf("Queueing Delay: %.4f\n", roundLog.totalRoom.at("QueueingDelay"));
//			printf("Kernel Delivery: %.4f\n\n", roundLog.totalRoom.at("kernelDelivery"));
//
//			printf("[GLOBAL IO PENDING]\n");
//			printf("Pending Counts : %d\n\n", roundLog.IOPending);
//
//			printf("[LOGIC WORKER]\n");
//			int workerNum = 0;
//			for (const auto& it : roundLog.workers)
//			{
//				if (it.type != WorkerType::LOGIC)
//					continue;
//
//				printf("Worker # %d | Job Counts %lld | ActiveTime %.2f | Utilization %.2f %%\n", workerNum, it.JobCounts, it.ActiveTimeMs, it.ActiveRatio);
//				workerNum++;
//			}
//
//			printf("[SEND WORKER]\n");
//			workerNum = 0;
//			for (const auto& it : roundLog.workers)
//			{
//				if (it.type != WorkerType::SEND)
//					continue;
//
//				printf("Worker # %d | Job Counts %lld | ActiveTime %.2f | Utilization %.2f %%\n", workerNum, it.JobCounts, it.ActiveTimeMs, it.ActiveRatio);
//				workerNum++;
//			}
//		}
//		// --- 출력 끝 ---
//
//		// 5. 다 그렸으면 화면 버퍼를 싹 바꿔치기 (이 순간 화면이 통째로 교체됨 -> 깜빡임 0)
//		SetConsoleActiveScreenBuffer(hTargetBuffer);
//		currentBufferIndex = backBufferIndex;
//
//		// CPU 100% 점유 방지 및 갱신 주기 조절
//		Sleep(50);
//	}
//
//	// 종료 시 생성했던 두 번째 버퍼 핸들 정리
//	CloseHandle(hScreens[1]);
//
//	//while (true)
//	//{
//	//	RoomLog roomLog = GRoomLog;
//	//	RoundLog roundLog = GRoundLog;
//
//	//	// Clear console
//	//	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
//	//	if (hConsole == INVALID_HANDLE_VALUE)
//	//		return;
//
//	//	COORD homeCoords = { 0, 0 };
//	//	SetConsoleCursorPosition(hConsole, homeCoords);
//
//	//	//CONSOLE_SCREEN_BUFFER_INFO csbi;
//	//	//DWORD count;
//	//	//DWORD cellCount;
//	//	//COORD homeCoords = { 0, 0 };
//
//	//	//if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
//	//	//	return;
//
//	//	//cellCount = csbi.dwSize.X * csbi.dwSize.Y;
//
//	//	//// 화면 지우기
//	//	//FillConsoleOutputCharacter(hConsole, ' ', cellCount, homeCoords, &count);
//	//	//FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, homeCoords, &count);
//
//	//	//// 커서 좌상단으로 이동
//	//	//SetConsoleCursorPosition(hConsole, homeCoords);
//
//	//	printf("[ROOM]\n");
//	//	printf("Tick Cost: %.2f ms\n", roomLog.tickCost);
//	//	printf("Tick Interval: %.0f ms\n\n", roomLog.tickInterval);
//
//	//	printf("[OBJECT]\n");
//	//	printf("Player] Chunk Count : %d, Player Count : %d\n", roomLog.players.first, roomLog.players.second);
//	//	printf("Monster] Chunk Count : %d, Monster Count : %d\n", roomLog.monsters.first, roomLog.monsters.second);
//	//	printf("Projectile] Chunk Count : %d, Projectile Count : %d\n", roomLog.projectiles.first, roomLog.projectiles.second);
//	//	printf("Fields] Chunk Count : %d, Field Count %d\n\n", roomLog.fields.first, roomLog.fields.second);
//
//	//	printf("[NETWORK]\n");
//	//	printf("ImmediateFlushPktBytes: %d\n", roomLog.immediateBytes);
//	//	printf("DeferFlushPktBytes: %d\n\n", roomLog.deferBytes);
//
//	//	if (roundLog.round > 0)
//	//	{
//	//		printf("[ROUND BENCH] : %d Round\n\n", roundLog.round);
//
//	//		printf("[TOTAL ROOM]\n");
//	//		printf("Tick Cost: %.4f\n", roundLog.totalRoom.find("Room")->second);
//	//		printf("Tick Interval: %.4f\n", roundLog.totalRoom.find("TickInterval")->second);
//	//		printf("Update Cost: %.4f\n\n", roundLog.totalRoom.find("Update")->second);
//
//	//		printf("[ROOM SEND JOB]\n");
//	//		printf("Total Delay: %.4f\n", roundLog.totalRoom.find("totalDelay")->second);
//	//		printf("Queueing Delay: %.4f\n", roundLog.totalRoom.find("QueueingDelay")->second);
//	//		printf("Kernel Delivery: %.4f\n\n", roundLog.totalRoom.find("kernelDelivery")->second);
//
//	//		printf("[GLOBAL IO PENDING]\n");
//	//		printf("Pending Counts : %d\n\n", roundLog.IOPending);
//
//	//		printf("[LOGIC WORKER]\n");
//	//		int workerNum = 0;
//	//		for (const auto& it : roundLog.workers)
//	//		{
//	//			if (it.type != WorkerType::LOGIC)
//	//				continue;
//
//	//			printf("Worker # %d | Job Counts %lld | ActiveTime %.2f | Utilization %.2f %%\n", workerNum, it.JobCounts, it.ActiveTimeMs, it.ActiveRatio);
//	//			workerNum++;
//	//		}
//	//		
//
//	//		printf("[SEND WORKER]\n");
//	//		workerNum = 0;
//	//		for (const auto& it : roundLog.workers)
//	//		{
//	//			if (it.type != WorkerType::SEND)
//	//				continue;
//
//	//			printf("Worker # %d | Job Counts %lld | ActiveTime %.2f | Utilization %.2f %%\n", workerNum, it.JobCounts, it.ActiveTimeMs, it.ActiveRatio);
//	//			workerNum++;
//	//		}
//	//	}
//	//}
//}

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
	
	{
		std::unique_lock<std::mutex> lock(GMonitoringMutex);

		GMonitoringCV.wait(lock, []()
			{
				return GMonitoringRoomID.load(std::memory_order_acquire) >= 0;
			});
	}

	this_thread::sleep_for(1s);
	DoRenderingWorker();

	GThreadManager->Join();
}