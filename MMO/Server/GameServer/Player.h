#pragma once
#include "Creature.h"
#include "Protocol.pb.h"

class GameSession;
class SkillState;
using GameSessionRef = shared_ptr<class GameSession>;
using SkillStateRef = shared_ptr<class SkillState>;

using namespace Protocol;

class Player : public Creature
{
public:
	Player();
	virtual ~Player();

	virtual void OnDamaged(ObjectRef attacker, int32 damage) override;
	virtual void OnDead(ObjectRef attacker) override;

	GameSessionRef GetSession() { return _session.lock(); }
	void SetSession(GameSessionRef session) { _session = session; }

	virtual void AddMoveFlushQueue(ObjectRef obj) override;
	virtual void AddSkillFlushQueue(ObjectRef obj, const Protocol::CastState& state, const Protocol::S_SKILL_EVENT& event) override;

	virtual void FlushStateInit() override;

private:
	weak_ptr<GameSession> _session;
};

