#pragma once

extern class ThreadManager* GThreadManager;
extern class MemoryManager* GMemoryManager;
extern class LocalMemoryManager* GLocalMemoryManager;
extern class SendBufferManager* GSendBufferManager;
extern class GlobalQueue* GGlobalQueue;
extern class GlobalQueue* GGlobalSendQueue;
extern class JobTimer* GJobTimer;

extern std::atomic<uint32> GIOPendingCounts;

extern uint64 GServerStartTick;