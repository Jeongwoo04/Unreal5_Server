#pragma once
#include "Creature.h"
#include "Player.h"

class SkillState;
using SkillStateRef = shared_ptr<class SkillState>;

class Monster : public Creature
{
public:
	Monster();
	virtual ~Monster();

	// FSM
	virtual void Update(float deltaTime) override;

	virtual void UpdateIdle(float deltaTime);
	virtual void UpdatePatrol(float deltaTime);
	virtual void UpdateCasting(float deltaTime);
	virtual void UpdateMoving(float deltaTime);
	virtual void UpdateSkill(float deltaTime);
	virtual void UpdateDead(float deltaTime);

	virtual void OnDamaged(ObjectRef attacker, int32 damage) override;
	virtual void OnDead(ObjectRef attacker) override;

	void BroadcastMove();

	void SetPlayer(const PlayerRef& player) { _targetPlayer = player; }
	PlayerRef GetPlayer() const { return _targetPlayer.lock(); }

	void SelectSkill();
	void DoSkill();

	bool CanUseSkill(int32 skillId, uint64 now) const;
	void StartSkillCast(int32 skillId, uint64 now, float castTime);
	void StartSkillCooldown(int32 skillId, uint64 now);

	SkillStateRef GetSkillState(int32 skillId) { return _skillStates[skillId]; }
	SkillInstance* GetSkillInstance() { return _activeSkill; }

public:
	vector<Vector2Int> _path;
	vector<Vector3> _simplifiedPath;
	int32 _simplifiedIndex = 0;

	const Skill* _selectedSkill = nullptr;

private:
	float _searchRadius = 20.f;
	float _chaseDistance = 30.f;

	uint64 _nextPathUpdateTick = 0;
	uint64 _nextSearchTick = 0;
	uint64 _coolTick = 0;

	Vector3 _lastTargetPos = Vector3(-99999, -99999, -99999);

private:
	weak_ptr<Player> _targetPlayer;
	unordered_map<int32, SkillStateRef> _skillStates;
};

