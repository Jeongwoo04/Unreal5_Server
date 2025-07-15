#pragma once
#include "Creature.h"
#include "Protocol.pb.h"

class GameSession;

using namespace Protocol;

class Player : public Creature
{
public:
	Player();
	virtual ~Player();

	GameSessionRef GetSession() { return _session.lock(); }
	void SetSession(GameSessionRef session) { _session = session; }

private:
	weak_ptr<GameSession> _session;
};

