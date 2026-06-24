#include "pch.h"
#include "CoreTLS.h"

thread_local uint32				LThreadId = 0;
thread_local string				LThreadName = "";
thread_local uint64				LEndTickCount = 0;
thread_local std::stack<int32>	LLockStack;

thread_local int32              LBenchRound = 1;
thread_local uint64             LExecuteJobCount = 0;
thread_local uint64             LExecuteJobQueues = 0;
thread_local uint64             LWorkerActiveTime = 0;
thread_local uint64             LTimeSliceExceeded = 0;

thread_local SendBufferChunkRef  LSendBufferChunk;
thread_local JobQueue* LCurrentJobQueue = nullptr;