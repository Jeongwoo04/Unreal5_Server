#include "pch.h"
#include <iostream>
#include "Service.h"
#include "Session.h"
#include "ServerSession.h"
#include "ServerSessionManager.h"

char sendData[] = "Hello World";

int main()
{
	ClientPacketHandler::Init();

	this_thread::sleep_for(1s);

	ClientServiceRef service = make_shared<ClientService>(
#ifdef _DEBUG
		NetAddress(L"127.0.0.1", 7777),
#else
		NetAddress(L"192.168.0.10", 7777),
#endif
		make_shared<IocpCore>(),
		[=]() { return make_shared<ServerSession>(); }, // TODO : SessionManager 등
		100);

	ASSERT_CRASH(service->Start());

	for (int32 i = 0; i < 2; i++)
	{
		GThreadManager->Launch("ClientIOWorker", [=]()
			{
				while (true)
				{
					service->GetIocpCore()->Dispatch();
				}
			});
	}

	while (true)
	{
		GSessionManager.SendRandomPos();
		this_thread::sleep_for(chrono::milliseconds(300));
	}


	GThreadManager->Join();
}