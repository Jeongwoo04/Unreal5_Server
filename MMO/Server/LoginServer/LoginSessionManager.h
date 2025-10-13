#pragma once

class LoginSession;

using LoginSessionRef = shared_ptr<LoginSession>;

class LoginSessionManager
{
public:
	void Add(LoginSessionRef session);
	void Remove(LoginSessionRef session);
	//void Broadcast(SendBufferRef sendBuffer);

private:
	USE_LOCK;
	set<LoginSessionRef> _sessions;
};

extern LoginSessionManager GLoginSessionManager;
