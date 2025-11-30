#pragma once
#include "Job.h"
#include "LockQueue.h"
#include "MPSCQueue.h"
#include "JobTimer.h"

/*--------------
	JobQueue
---------------*/

enum JobQueueFlag
{
	DEFAULT = 0,
	SQ = 1
};

class JobQueue : public enable_shared_from_this<JobQueue>
{
	//TEMP
public:
	JobQueue(string name = "JobQueue") : _queueName(move(name))
	{

	}

public:
	void DoAsync(CallbackType&& callback)
	{
		Push(make_shared<Job>(std::move(callback)));
	}

	template<typename T, typename Ret, typename... Args>
	void DoAsync(Ret(T::*memFunc)(Args...), Args... args)
	{
		shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());
		Push(make_shared<Job>(owner, memFunc, std::forward<Args>(args)...));
	}

	void DoAsyncSendJob(CallbackType&& callback)
	{
		PushSendJob(make_shared<Job>(std::move(callback)), true);
	}

	template<typename T, typename Ret, typename... Args>
	void DoAsyncSendJob(Ret(T::* memFunc)(Args...), Args... args)
	{
		shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());
		PushSendJob(make_shared<Job>(owner, memFunc, std::forward<Args>(args)...), true);
	}

	void DoAsyncPushOnly(CallbackType&& callback)
	{
		Push(make_shared<Job>(std::move(callback)), true);
	}

	template<typename T, typename Ret, typename... Args>
	void DoAsyncPushOnly(Ret(T::* memFunc)(Args...), Args... args)
	{
		shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());
		Push(make_shared<Job>(owner, memFunc, std::forward<Args>(args)...), true);
	}

	void DoTimer(uint64 tickAfter, CallbackType&& callback)
	{
		JobRef job = make_shared<Job>(std::move(callback));
		GJobTimer->Reserve(tickAfter, shared_from_this(), job);
	}

	template<typename T, typename Ret, typename... Args>
	void DoTimer(uint64 tickAfter, Ret(T::* memFunc)(Args...), Args... args)
	{
		shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());
		JobRef job = make_shared<Job>(owner, memFunc, std::forward<Args>(args)...);
		GJobTimer->Reserve(tickAfter, shared_from_this(), job);
	}

	void					ClearJobs() { _jobs.Clear(); }

public:
	void					Push(JobRef job, bool pushOnly = false);
	void					PushSendJob(JobRef sendJob, bool pushOnly = false);
	void					Execute();
	void					ExecuteSendJob();

protected:
	LockQueue<JobRef>		_jobs;
	//MPSCQueue<JobRef>		_jobs;
	atomic<int32>			_jobCount = 0;

protected:
	string _queueName;
};

