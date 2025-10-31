#pragma once

class ServerSession;

using ServerSessionRef = shared_ptr<ServerSession>;

class ServerSessionManager
{
public:
	void Add(ServerSessionRef session);
	void Remove(ServerSessionRef session);
	void Broadcast(SendBufferRef sendBuffer);

	void SendRandomPos();

private:
	USE_LOCK;
	set<ServerSessionRef> _sessions;
};

extern ServerSessionManager GSessionManager;
