#pragma once

#include "GeometryUtil.h"

struct SkillInstance
{
	ObjectRef caster;
	const Skill* skill;
	Vector3 targetPos;

	bool isCasting = false;
	bool canceled = false;

	float castElapsed = 0.f;
	float actionDelayElapsed = 0.f;
	int32 currentActionIndex = 0;
};

class SkillSystem
{
public:
	void Init();
	void ExecuteSkill(ObjectRef caster, int32 skillId, const Vector3& targetPos);

	void Update(float deltaTime);
	void CancelCasting(ObjectRef caster);

private:
	void HandleAction(ObjectRef caster, const Vector3& targetPos, ActionData* action);

	void HandleMoveAction(ObjectRef caster, const Vector3& targetPos, MoveActionData* action);
	void HandleAttackAction(ObjectRef caster, const Vector3& targetPos, AttackActionData* action);
	void HandleSpawnAction(ObjectRef caster, const Vector3& targetPos, SpawnActionData* action);
	void HandleBuffAction(ObjectRef caster, const Vector3& targetPos, BuffActionData* action);

private:
	const unordered_map<int32, Skill>* skillDict = nullptr;
	vector<SkillInstance> activeSkills;
};

