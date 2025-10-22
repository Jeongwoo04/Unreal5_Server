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

void JobQueue::Execute()
{
	LCurrentJobQueue = this;

	bool isBCQueue = (_flag == JobQueueFlag::BCQ);
	uint64 startTick = 0;
	if (isBCQueue)
		startTick = GetTickCount64();

	while (true)
	{
		vector<JobRef> jobs;
		_jobs.PopAll(OUT jobs);

		const int32 jobCount = static_cast<int32>(jobs.size());
		//if (isBCQueue)
		//	cout << "jobCount = " << jobCount << endl;
		for (int32 i = 0; i < jobCount; i++)
			jobs[i]->Execute();

		// 남은 일감이 0개라면 종료
		if (_jobCount.fetch_sub(jobCount) == jobCount)
		{
			LCurrentJobQueue = nullptr;
			//if (isBCQueue)
			//	cout << "End : " << GetTickCount64() - startTick << " tick\n";

			return;
		}

		const uint64 now = ::GetTickCount64();
		if (now >= LEndTickCount)
		{
			LCurrentJobQueue = nullptr;

			//if (isBCQueue)
			//	cout << "Push : " << now - startTick << " tick\n";
			// GlobalQueue로 넘겨 여유있는 Worker가 실행
			GGlobalQueue->Push(shared_from_this());

			break;
		}			
	}
}
