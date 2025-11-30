#include "pch.h"
#include "JobQueue.h"
#include "GlobalQueue.h"

/*--------------
	JobQueue
---------------*/

void JobQueue::Push(JobRef job, bool pushOnly)
{
	const int32 prevCount = _jobCount.fetch_add(1);
	_jobs.Push(job); // WRITE_LOCK

	// 첫번째 Job을 넣은 쓰레드가 실행 fast-path
	if (prevCount == 0)
	{
		// 실행중인 JobQueue가 없으면 실행
		if (LCurrentJobQueue == nullptr && pushOnly == false)
		{
			Execute();
		}
		else
		{
			// GlobalQueue로 넘겨 여유있는 Worker가 실행
			GGlobalQueue->Push(shared_from_this());
		}
	}
}

void JobQueue::PushSendJob(JobRef sendJob, bool pushOnly)
{
	const int32 prevCount = _jobCount.fetch_add(1);
	_jobs.Push(sendJob); // WRITE_LOCK

	// 첫번째 Job을 넣은 쓰레드가 실행 fast-path
	if (prevCount == 0)
	{
		// 실행중인 JobQueue가 없으면 실행
		if (LCurrentJobQueue == nullptr && pushOnly == false)
		{
			ExecuteSendJob();
		}
		else
		{
			// GlobalQueue로 넘겨 여유있는 Worker가 실행
			GGlobalSendQueue->Push(shared_from_this());
		}
	}
}

/* MPCS Queue 기반 */
//void JobQueue::Execute()
//{
//	LCurrentJobQueue = this;
//
//	while (true)
//	{
//		std::vector<JobRef> jobs;
//		JobRef job;
//		while (_jobs.Pop(job))
//			jobs.push_back(job);
//
//		const int32_t jobCount = static_cast<int32_t>(jobs.size());
//		for (int32_t i = 0; i < jobCount; i++)
//			jobs[i]->Execute();
//
//		if (_jobCount.fetch_sub(jobCount, std::memory_order_relaxed) == jobCount)
//		{
//			LCurrentJobQueue = nullptr;
//			return;
//		}
//
//		const uint64_t now = ::GetTickCount64();
//		if (now >= LEndTickCount)
//		{
//			LCurrentJobQueue = nullptr;
//			GGlobalQueue->Push(shared_from_this());
//			break;
//		}
//	}
//}

//void JobQueue::ExecuteSendJob()
//{
//	LCurrentJobQueue = this;
//
//	while (true)
//	{
//		std::vector<JobRef> jobs;
//		JobRef job;
//		while (_jobs.Pop(job))
//			jobs.push_back(job);
//
//		const int32_t jobCount = static_cast<int32_t>(jobs.size());
//		for (int32_t i = 0; i < jobCount; i++)
//			jobs[i]->Execute();
//
//		if (_jobCount.fetch_sub(jobCount, std::memory_order_relaxed) == jobCount)
//		{
//			LCurrentJobQueue = nullptr;
//			return;
//		}
//
//		const uint64_t now = ::GetTickCount64();
//		if (now >= LEndTickCount)
//		{
//			LCurrentJobQueue = nullptr;
//			GGlobalSendQueue->Push(shared_from_this());
//			break;
//		}
//	}
//}

/* LockQueue 기반 */
void JobQueue::Execute()
{
	LCurrentJobQueue = this;

	while (true)
	{
		vector<JobRef> jobs;
		_jobs.PopAll(OUT jobs);

		const int32 jobCount = static_cast<int32>(jobs.size());
		for (int32 i = 0; i < jobCount; i++)
			jobs[i]->Execute();

		// 남은 일감이 0개라면 종료
		if (_jobCount.fetch_sub(jobCount) == jobCount)
		{
			LCurrentJobQueue = nullptr;

			return;
		}

		const uint64 now = ::GetTickCount64();
		if (now >= LEndTickCount)
		{
			LCurrentJobQueue = nullptr;
			GGlobalQueue->Push(shared_from_this());

			break;
		}			
	}
}

void JobQueue::ExecuteSendJob()
{
	LCurrentJobQueue = this;

	while (true)
	{
		vector<JobRef> jobs;
		_jobs.PopAll(OUT jobs);

		const int32 jobCount = static_cast<int32>(jobs.size());
		for (int32 i = 0; i < jobCount; i++)
			jobs[i]->Execute();

		// 남은 일감이 0개라면 종료
		if (_jobCount.fetch_sub(jobCount) == jobCount)
		{
			LCurrentJobQueue = nullptr;

			return;
		}

		const uint64 now = ::GetTickCount64();
		if (now >= LEndTickCount)
		{
			LCurrentJobQueue = nullptr;
			GGlobalSendQueue->Push(shared_from_this());

			break;
		}
	}
}
