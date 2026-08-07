#pragma once
#include <stack>
#include <vector>

extern thread_local uint32				LThreadId;
extern thread_local std::string			LThreadName;
extern thread_local uint64				LEndTickCount;
extern thread_local std::stack<int32>	LLockStack;

extern thread_local uint64              LCachedTick;
extern thread_local std::vector<double> LQueueingDelay;
extern thread_local std::vector<double> LKernelDelivery;
extern thread_local std::vector<double> LTotalDelay;

extern thread_local int32               LBenchRound;
extern thread_local uint64              LImmediateEmpty;
extern thread_local uint64              LDeferEmpty;
extern thread_local uint64              LExecuteJobCount;
extern thread_local uint64              LExecuteJobQueues;
extern thread_local uint64              LWorkerActiveTime;
extern thread_local uint64              LTimeSliceExceeded;

extern thread_local SendBufferChunkRef  LSendBufferChunk;
extern thread_local class JobQueue* LCurrentJobQueue;