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

void CommandThread(CommandManager* commandManager)
{
	cout << "[Server] CommandThread is running\n";
	string line;
	while (true)
	{
		cout << "> ";
		if (!getline(cin, line))
			break;

		if (line == "exit" || line == "quit")
		{
			cout << "Shut down" << endl;
			break;
		}

		commandManager->Execute(line);
	}

	cout << "[Command Thread] terminated" << endl;
}

//void LogThreadStart(int32 index)
//{
//#ifdef _WIN32
//	DWORD tid = GetCurrentThreadId();
//#else
//	pid_t tid = syscall(SYS_gettid);
//#endif
//	cout << "[Thread#" << index << " | ID " << tid << "] start.\n";
//}

int main()
{
	cout << "[Server] Start\n";
	ServerPacketHandler::Init();
	cout << "[Server] ServerPacketHandler Init\n";

	ConfigManager::Instance().LoadConfig("../Data/config.json");
	DataManager::Instance().LoadData("../Data");
	cout << "[Server] DataManager LoadData\n";
	
	
	cout << "[Server] ServerService Start '127.0.0.1', '7777'\n";
	ServerServiceRef service = make_shared<ServerService>(
		NetAddress(L"127.0.0.1", 7777),
		make_shared<IocpCore>(),
		[=]() { return make_shared<GameSession>(); },
		100);

	ASSERT_CRASH(service->Start());

	cout << "[Server] IO Worker Start : 8 Thread\n";
	
	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch("IOWorker#" + to_string(i), [&service]()
			{
				DoIOWorker(service);
			});
	}

	cout << "[Server] Game Worker Start : 8 Thread\n";
	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch("GameWorker#" + to_string(i), []()
			{
				DoGameWorker(); 
			});
	}

	//for (int32 i = 0; i < 2; i++)
		RoomRef room = RoomManager::Instance().Add(1);

	CommandManager commandManager;
	commandManager.Init();
	thread commandThread(CommandThread, &commandManager);

	// Main Thread
	//DoWorkerJob(service);
	if (commandThread.joinable())
		commandThread.join();

	GThreadManager->Join();
}