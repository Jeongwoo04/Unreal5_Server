#pragma once
#include "Creature.h"
#include "Protocol.pb.h"

class GameSession;
using GameSessionRef = shared_ptr<class GameSession>;

using namespace Protocol;

class Player : public Creature
{
public:
	Player();
	virtual ~Player();

	void OnDamaged(ObjectRef attacker, int32 damage) override;
	void OnDead(ObjectRef attacker) override;

	GameSessionRef GetSession() { return _session.lock(); }
	void SetSession(GameSessionRef session) { _session = session; }

public:
	float _collisionRadius = 42.f;

private:
	weak_ptr<GameSession> _session;
};

