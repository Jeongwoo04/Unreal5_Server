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

	bool CanUseSkill(int32 skillId, uint64 now) const;
	void StartSkillCast(int32 skillId, uint64 now, float castTime);
	void StartSkillCooldown(int32 skillId, uint64 now);

	SkillStateRef GetSkillState(int32 skillId) { return _skillStates[skillId]; }
	SkillInstance* GetSkillInstance() { return _activeSkill; }

private:
	weak_ptr<GameSession> _session;
	unordered_map<int32, SkillStateRef> _skillStates;
};

