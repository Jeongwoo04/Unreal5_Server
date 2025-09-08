#pragma once

#include "DataManager.h"

class Room;
class Player;

class CombatSystem
{
public:
	void ExecuteSkill(ObjectRef caster, int32 skillId);

private:
	void ExecuteAction(ObjectRef caster, const ActionData& actionData);

	void HandleMoveAction(ObjectRef caster, const ActionData& action);
	void HandleAttackAction(ObjectRef caster, ObjectRef target, const ActionData& action);
	void HandleSpawnProjectileAction(ObjectRef caster, ObjectRef target, const ActionData& action);
	void HandleSpawnFieldAction(ObjectRef caster, ObjectRef target, const ActionData& action);
	void HandleApplyStatusAction(ObjectRef caster, ObjectRef target, const ActionData& action);
	void HandleReactAction(ObjectRef target, const ActionData& action);

private:
	weak_ptr<Room> _room;
	const unordered_map<int32, Skill>* _skillDict;
};

