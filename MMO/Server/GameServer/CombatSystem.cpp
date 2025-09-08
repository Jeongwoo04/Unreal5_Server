#include "pch.h"
#include "CombatSystem.h"

void CombatSystem::ExecuteSkill(ObjectRef caster, int32 skillId)
{
	auto skillIt = _skillDict->find(skillId);
	if (skillIt == _skillDict->end())
		return;

	Skill skill = skillIt->second;
	for (const auto& action : skill.actions)
	{
		ExecuteAction(caster, action);
	}
}

void CombatSystem::ExecuteAction(ObjectRef caster, const ActionData& action)
{
	//switch (action.type)
	//{
	//case ActionType::Move:
	//	HandleMoveAction();
	//	break;
	//case ActionType::Attack:
	//	HandleAttackAction();
	//	break;
	//case ActionType::SpawnProjectile:
	//	HandleSpawnProjectileAction();
	//	break;
	//case ActionType::SpawnField:
	//	HandleSpawnFieldAction();
	//	break;
	//case ActionType::ApplyStatus:
	//	HandleApplyStatusAction();
	//	break;
	//case ActionType::React:
	//	HandleReactAction();
	//	break;
	//}
}

void CombatSystem::HandleMoveAction(ObjectRef caster, const ActionData& action)
{
	Vector3 dir = Vector3::YawToDir(caster->_posInfo.yaw());
	Vector3 destPos = Vector3(caster->_posInfo) + dir * action.value;

	//if (!(caster->GetRoom()->GetGameMap()->CanGo(Vector2Int(destPos), false)))
	//	return;

	caster->SetPos(destPos);
	caster->GetRoom()->BroadcastMove(caster->_posInfo);
}

void CombatSystem::HandleAttackAction(ObjectRef caster, ObjectRef target, const ActionData& action)
{

}

void CombatSystem::HandleSpawnProjectileAction(ObjectRef caster, ObjectRef target, const ActionData& action)
{
	auto room = caster->GetRoom();
	if (room == nullptr)
		return;

	room->SpawnProjectile(action.dataId, Vector3(target->_posInfo));
}

void CombatSystem::HandleSpawnFieldAction(ObjectRef caster, ObjectRef target, const ActionData& action)
{
	auto room = caster->GetRoom();
	if (room == nullptr)
		return;

	room->SpawnField(action.dataId, Vector3(target->_posInfo));
}

void CombatSystem::HandleApplyStatusAction(ObjectRef caster, ObjectRef target, const ActionData& action)
{

}

void CombatSystem::HandleReactAction(ObjectRef target, const ActionData& action)
{

}
