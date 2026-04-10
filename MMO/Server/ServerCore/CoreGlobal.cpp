#include "pch.h"
#include "CoreGlobal.h"
#include "pch.h"
#include "CoreGlobal.h"
#include "ThreadManager.h"
#include "MemoryManager.h"
#include "SocketUtils.h"
#include "SendBuffer.h"
#include "GlobalQueue.h"
#include "JobTimer.h"

ThreadManager* GThreadManager = nullptr;
MemoryManager* GMemoryManager = nullptr;
LocalMemoryManager* GLocalMemoryManager = nullptr;
SendBufferManager* GSendBufferManager = nullptr;
GlobalQueue* GGlobalQueue = nullptr;
GlobalQueue* GGlobalSendQueue = nullptr;
JobTimer* GJobTimer = nullptr;

class CoreGlobal
{
public:
	CoreGlobal()
	{
		GThreadManager = new ThreadManager();
		GMemoryManager = new MemoryManager();
		GLocalMemoryManager = new LocalMemoryManager();
#ifdef USE_OPTIMIZED_SENDBUFFER_CHUNK
		GSendBufferManager = new SendBufferManager();
#endif
		GGlobalQueue = new GlobalQueue();
		GGlobalSendQueue = new GlobalQueue();
		GJobTimer = new JobTimer();
		SocketUtils::Init();
	}

	~CoreGlobal()
	{
		delete GThreadManager;
		delete GMemoryManager;
		delete GLocalMemoryManager;
#ifdef USE_OPTIMIZED_SENDBUFFER_CHUNK
		delete GSendBufferManager;
#endif
		delete GGlobalQueue;
		delete GGlobalSendQueue;
		delete GJobTimer;
		SocketUtils::Clear();
	}
} GCoreGlobal;