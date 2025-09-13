#include "pch.h"
#include "SkillSystem.h"
#include "ObjectManager.h"
#include "CombatSystem.h"
#include "BuffSystem.h"



void SkillSystem::Init()
{
	skillDict = &DataManager::Instance().SkillDict;
}

void SkillSystem::ExecuteSkill(ObjectRef caster, int32 skillId, const Vector3& targetPos)
{
	auto it = skillDict->find(skillId);
	if (it == skillDict->end())
		return;

	const Skill& skill = it->second;

	SkillInstance instance;
	instance.caster = caster;
	instance.skill = &skill;
	instance.targetPos = targetPos;

	activeSkills.push_back(instance);

	for (auto& action : skill.actions)
	{
		HandleAction(caster, targetPos, action);
	}
}

void SkillSystem::Update()
{
	for (auto it = activeSkills.begin(); it != activeSkills.end(); )
	{
		it->elapsedTime += 0.1f;
		if (it->elapsedTime >= it->skill->cooldown)
			it = activeSkills.erase(it);
		else
			++it;
	}
}

void SkillSystem::HandleAction(ObjectRef caster, const Vector3& targetPos, const ActionData& action)
{
	switch (action.actionType)
	{
	case ActionType::Move:
		HandleMoveAction(caster, targetPos, static_cast<const MoveActionData&>(action));
		break;
	case ActionType::Attack:
		HandleAttackAction(caster, targetPos, static_cast<const AttackActionData&>(action));
		break;
	case ActionType::SpawnProjectile:
		HandleSpawnAction(caster, targetPos, static_cast<const SpawnActionData&>(action));
		break;
	case ActionType::SpawnField:
		HandleSpawnAction(caster, targetPos, static_cast<const SpawnActionData&>(action));
		break;
	case ActionType::Buff:
		HandleBuffAction(caster, targetPos, static_cast<const BuffActionData&>(action));
		break;
	default:
		break;
	}
}

void SkillSystem::HandleMoveAction(ObjectRef caster, const Vector3& targetPos, const MoveActionData& action)
{
	Vector3 forward = Vector3::YawToDir2D(caster->_posInfo.yaw());
	Vector3 destPos = Vector3(caster->_posInfo) + forward * action.moveDistance;
	
	// TODO : Room::HandleMove 분리 + Object 별 Move 처리 분리.
	// caster->SetPos(destPos);
	// caster->GetRoom()->BroadcasterMove();
}

void SkillSystem::HandleAttackAction(ObjectRef caster, const Vector3& targetPos, const AttackActionData& action)
{
	Vector3 center = Vector3(caster->_posInfo);
	Vector3 forward = Vector3::YawToDir2D(caster->_posInfo.yaw());

	if (caster->GetCreatureType() == CREATURE_TYPE_MONSTER)
	{
		auto playerCandidates = caster->GetRoom()->_playerGrid.FindAroundFloat(Vector2Int(caster->_posInfo), action.radius);
		vector<PlayerRef> targetPlayers;

		switch (action.shape)
		{
		case ShapeType::Circle:
			targetPlayers = GeometryUtil::FindInCircle2D(playerCandidates, center, action.radius);
			break;
		case ShapeType::Cone:
			targetPlayers = GeometryUtil::FindInCone2D(playerCandidates, center, forward, action.angle, action.radius);
			break;
		case ShapeType::Rectangle:
			targetPlayers = GeometryUtil::FindInRectangle2D(playerCandidates, center, forward, action.width, action.length);
			break;
		case ShapeType::Line:
			targetPlayers = GeometryUtil::FindInLine2D(playerCandidates, center, targetPos, action.radius);
			break;
		default:
			break;
		}

		// ApplyDamage 시점에 ObjectRef로 변환
		for (auto& target : targetPlayers)
		{
			CombatSystem::Instance().ApplyDamage(caster, static_pointer_cast<Object>(target), action.damage);
		}
	}
	else if (caster->GetCreatureType() == CREATURE_TYPE_PLAYER)
	{
		auto monsterCandidates = caster->GetRoom()->_monsterGrid.FindAroundFloat(Vector2Int(caster->_posInfo), action.radius);
		vector<MonsterRef> hitMonsters;

		switch (action.shape)
		{
		case ShapeType::Circle:
			hitMonsters = GeometryUtil::FindInCircle2D(monsterCandidates, center, action.radius);
			break;
		case ShapeType::Cone:
			hitMonsters = GeometryUtil::FindInCone2D(monsterCandidates, center, forward, action.angle, action.radius);
			break;
		case ShapeType::Rectangle:
			hitMonsters = GeometryUtil::FindInRectangle2D(monsterCandidates, center, forward, action.width, action.length);
			break;
		case ShapeType::Line:
			hitMonsters = GeometryUtil::FindInLine2D(monsterCandidates, center, targetPos, action.radius);
			break;
		default:
			break;
		}

		for (auto m : hitMonsters)
		{
			CombatSystem::Instance().ApplyDamage(caster, ObjectRef(m), action.damage);
		}
	}
}

void SkillSystem::HandleSpawnAction(ObjectRef caster, const Vector3& targetPos, const SpawnActionData& action)
{
	if (action.actionType == ActionType::SpawnProjectile)
		caster->GetRoom()->_objectManager->Spawn(action.dataId, false, Vector3(caster->_posInfo), caster->_posInfo.yaw());
	else if (action.actionType == ActionType::SpawnField)
		caster->GetRoom()->_objectManager->Spawn(action.dataId, false, targetPos);
}

void SkillSystem::HandleBuffAction(ObjectRef caster, const Vector3& targetPos, const BuffActionData& action)
{
	// TODO : BuffSystem 추가
	BuffSystem::Instance().ApplyBuff(caster, action.buffId);
}
