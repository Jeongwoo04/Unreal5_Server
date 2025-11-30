#include "pch.h"
#include "ThreadManager.h"
#include "CoreTLS.h"
#include "CoreGlobal.h"
#include "GlobalQueue.h"
#include <string>

/*------------------
	ThreadManager
-------------------*/

ThreadManager::ThreadManager()
{
	// Main Thread
	InitTLS("MainThread");
}

ThreadManager::~ThreadManager()
{
	Join();
}

void ThreadManager::Launch(const string& name, function<void(void)> callback)
{
	lock_guard<mutex> guard(_lock);

	_threads.push_back(thread([=]()
		{
			InitTLS(name);

#ifdef _WIN32
			DWORD sysId = GetCurrentThreadId();
#else
			pid_t sysId = syscall(SYS_gettid);
#endif

			SafeLog("[ThreadManager] Launch: " + LThreadName +
				" (LThreadId: " + to_string(LThreadId) +
				", SysThreadId: " + to_string(sysId) + ")");

			callback();
			DestroyTLS();
		}));
}

void ThreadManager::Join()
{
	for (thread& t : _threads)
	{
		if (t.joinable())
			t.join();
	}
	_threads.clear();
}

void ThreadManager::InitTLS(const string& name)
{
	static atomic<uint32> SThreadId = 1;
	LThreadId = SThreadId.fetch_add(1);
	LThreadName = name;

#ifdef _WIN32
	const wstring wname(name.begin(), name.end());
	SetThreadDescription(GetCurrentThread(), wname.c_str());
#endif
}

void ThreadManager::DestroyTLS()
{

}

void ThreadManager::DoGlobalQueueWork()
{
	while (true)
	{
		uint64 now = ::GetTickCount64();
		if (now > LEndTickCount)
			break;

		JobQueueRef jobQueue = GGlobalQueue->Pop();
		if (jobQueue == nullptr)
			break;

		jobQueue->Execute();
	}
}

void ThreadManager::DoGlobalSendQueueWork()
{
	while (true)
	{
		uint64 now = ::GetTickCount64();
		if (now > LEndTickCount)
			break;

		JobQueueRef jobQueue = GGlobalSendQueue->Pop();
		if (jobQueue == nullptr)
			break;

		jobQueue->ExecuteSendJob();
	}
}

void ThreadManager::DistributeReservedJobs()
{
	const uint64 now = ::GetTickCount64();

	GJobTimer->Distribute(now);
}
