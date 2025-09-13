#pragma once

#include "GeometryUtil.h"

class SkillSystem
{
public:
	void Init();
	void ExecuteSkill(ObjectRef caster, int32 skillId, const Vector3& targetPos);

	void Update();

private:
	void HandleAction(ObjectRef caster, const Vector3& targetPos, const ActionData& action);

	void HandleMoveAction(ObjectRef caster, const Vector3& targetPos, const MoveActionData& action);
	void HandleAttackAction(ObjectRef caster, const Vector3& targetPos, const AttackActionData& action);
	void HandleSpawnAction(ObjectRef caster, const Vector3& targetPos, const SpawnActionData& action);
	void HandleBuffAction(ObjectRef caster, const Vector3& targetPos, const BuffActionData& action);

private:
	struct SkillInstance
	{
		ObjectRef caster;
		const Skill* skill;

		Vector3 targetPos;
		float elapsedTime = 0.f;
	};

private:
	const unordered_map<int32, Skill>* skillDict = nullptr;
	vector<SkillInstance> activeSkills;
};

