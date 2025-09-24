#include "pch.h"
#include "SkillSystem.h"
#include "SkillState.h"
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

	uint64 now = GetTickCount64();

	const Skill& skill = it->second;

	SkillInstance instance;
	instance.caster = caster;
	instance.skill = &skill;
	instance.targetPos = targetPos;
	instance.isCasting = (skill.castTime > 0.0f);
	instance.actionDelayElapsed = 0.f;
	instance.currentActionIndex = 0;

	if (instance.isCasting == true)
	{
		caster->_activeSkill = &instance;
		caster->_posInfo.set_yaw(Vector3::DirToYaw2D((targetPos - caster->_worldPos).Normalized2D()));
		caster->ChangeState(Protocol::STATE_MACHINE_CASTING);
		
		if (caster->GetCreatureType() == CREATURE_TYPE_PLAYER)
			static_pointer_cast<Player>(caster)->StartSkillCast(skillId, now, skill.castTime);
		else if (caster->GetCreatureType() == CREATURE_TYPE_MONSTER)
			static_pointer_cast<Monster>(caster)->StartSkillCast(skillId, now, skill.castTime);
		
		// 캐스팅시작 패킷전송
		Protocol::S_SKILL_CAST_START pkt;
		pkt.set_object_id(caster->GetId());
		pkt.set_casttime(instance.skill->castTime);
		pkt.mutable_info()->set_skillid(skillId);

		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
		caster->GetRoom()->Broadcast(sendBuffer, caster->GetId());
	}
	else
	{
		if (!skill.actions.empty())
		{
			HandleAction(caster, targetPos, skill.actions[0]);
			if (caster->GetCreatureType() == CREATURE_TYPE_PLAYER)
			{
				static_pointer_cast<Player>(caster)->StartSkillCooldown(skillId, now);
			}
			else if (caster->GetCreatureType() == CREATURE_TYPE_MONSTER)
			{
				static_pointer_cast<Monster>(caster)->StartSkillCooldown(skillId, now);
			}
		}
	}

	activeSkills.push_back(instance);
}

void SkillSystem::CancelCasting(ObjectRef caster)
{
	if (caster->_activeSkill && caster->_activeSkill->isCasting)
	{
		caster->_activeSkill->canceled = true;
		caster->_activeSkill = nullptr;

		// Update에서 제거
	}
}

void SkillSystem::Update(float deltaTime)
{
	uint64 now = GetTickCount64();

	for (auto it = activeSkills.begin(); it != activeSkills.end(); )
	{
		SkillInstance& inst = *it;

		// 취소된 스킬이면 제거
		if (inst.canceled)
		{
			it = activeSkills.erase(it);
			continue;
		}

		// 캐스팅 중이면 캐스팅 처리
		if (inst.isCasting)
		{
			bool ret = false;
			if (inst.caster->GetCreatureType() == CREATURE_TYPE_MONSTER)
				ret = (now >= static_pointer_cast<Monster>(inst.caster)->GetSkillState(inst.skill->id)->GetCastEndTime()) ? true : false;
			else if (inst.caster->GetCreatureType() == CREATURE_TYPE_PLAYER)
				ret = (now >= static_pointer_cast<Player>(inst.caster)->GetSkillState(inst.skill->id)->GetCastEndTime()) ? true : false;
			
			if (ret)
			{
				cout << "캐스팅 완료" << endl;
				inst.isCasting = false;
				inst.actionDelayElapsed = 0.f;
				inst.caster->ChangeState(Protocol::STATE_MACHINE_IDLE);
				// 캐스팅 완료 패킷 전송
				{
					S_SKILL_CAST_SUCCESS pkt;
					pkt.set_object_id(inst.caster->GetId());
					pkt.set_skillid(inst.skill->id);
					auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
					inst.caster->GetRoom()->Broadcast(sendBuffer, inst.caster->GetId());
				}

				if (inst.caster->GetCreatureType() == CREATURE_TYPE_PLAYER)
					static_pointer_cast<Player>(inst.caster)->StartSkillCooldown(inst.skill->id, now);
				else if (inst.caster->GetCreatureType() == CREATURE_TYPE_MONSTER)
					static_pointer_cast<Monster>(inst.caster)->StartSkillCooldown(inst.skill->id, now);

				// 캐스팅이 끝난 후 나머지 액션 실행
				if (!inst.skill->actions.empty())
				{
					ActionData* action = inst.skill->actions[inst.currentActionIndex];
					HandleAction(inst.caster, inst.targetPos, action);
					inst.currentActionIndex++;
					inst.actionDelayElapsed = 0.f;
				}
			}
			++it;
			continue;
		}

		// 캐스팅이 끝났다면 액션 실행
		if (inst.currentActionIndex < (int32)inst.skill->actions.size())
		{
			inst.actionDelayElapsed += deltaTime;
			ActionData* action = inst.skill->actions[inst.currentActionIndex];

			if (inst.actionDelayElapsed >= action->actionDelay)
			{
				HandleAction(inst.caster, inst.targetPos, action);

				inst.currentActionIndex++;
				inst.actionDelayElapsed = 0.f; // 다음 액션 준비
			}
		}

		if (inst.currentActionIndex >= (int32)inst.skill->actions.size())
		{
			cout << "스킬 완료" << endl;
			if (auto monster = dynamic_pointer_cast<Monster>(inst.caster))
			{
				monster->_selectedSkill = nullptr;
				monster->ChangeState(Protocol::STATE_MACHINE_IDLE);
			}

			it = activeSkills.erase(it);
		}
	}
}

void SkillSystem::HandleAction(ObjectRef caster, const Vector3& targetPos, ActionData* action)
{
	switch (action->actionType)
	{
	case ActionType::Move:
		HandleMoveAction(caster, targetPos, static_cast<MoveActionData*>(action));
		break;
	case ActionType::Attack:
		HandleAttackAction(caster, targetPos, static_cast<AttackActionData*>(action));
		break;
	case ActionType::SpawnProjectile:
		HandleSpawnAction(caster, targetPos, static_cast<SpawnActionData*>(action));
		break;
	case ActionType::SpawnField:
		HandleSpawnAction(caster, targetPos, static_cast<SpawnActionData*>(action));
		break;
	case ActionType::Buff:
		HandleBuffAction(caster, targetPos, static_cast<BuffActionData*>(action));
		break;
	default:
		break;
	}
}

void SkillSystem::HandleMoveAction(ObjectRef caster, const Vector3& targetPos, MoveActionData* action)
{
	caster->MoveToNextPos(targetPos);
}

void SkillSystem::HandleAttackAction(ObjectRef caster, const Vector3& targetPos, AttackActionData* action)
{
	Vector3 center = Vector3(caster->_posInfo);
	Vector3 forward = Vector3::YawToDir2D(caster->_posInfo.yaw());

	if (caster->GetCreatureType() == CREATURE_TYPE_MONSTER)
	{
		auto playerCandidates = caster->GetRoom()->_playerGrid.FindAroundFloat(Vector2Int(caster->_posInfo), action->radius);
		vector<PlayerRef> targetPlayers;

		switch (action->shape)
		{
		case ShapeType::Circle:
			targetPlayers = GeometryUtil::FindInCircle2D(playerCandidates, center, action->radius);
			break;
		case ShapeType::Cone:
			targetPlayers = GeometryUtil::FindInCone2D(playerCandidates, center, forward, action->angle, action->radius);
			break;
		case ShapeType::Rectangle:
			targetPlayers = GeometryUtil::FindInRectangle2D(playerCandidates, center, forward, action->width, action->length);
			break;
		case ShapeType::Line:
			targetPlayers = GeometryUtil::FindInLine2D(playerCandidates, center, targetPos, action->radius);
			break;
		default:
			break;
		}

		// ApplyDamage 시점에 ObjectRef로 변환
		for (auto& target : targetPlayers)
		{
			CombatSystem::Instance().ApplyDamage(caster, static_pointer_cast<Object>(target), action->damage);
		}
	}
	else if (caster->GetCreatureType() == CREATURE_TYPE_PLAYER)
	{
		auto monsterCandidates = caster->GetRoom()->_monsterGrid.FindAroundFloat(Vector2Int(caster->_posInfo), action->radius);
		vector<MonsterRef> targetMonsters;

		switch (action->shape)
		{
		case ShapeType::Circle:
			targetMonsters = GeometryUtil::FindInCircle2D(monsterCandidates, center, action->radius);
			break;
		case ShapeType::Cone:
			targetMonsters = GeometryUtil::FindInCone2D(monsterCandidates, center, forward, action->angle, action->radius);
			break;
		case ShapeType::Rectangle:
			targetMonsters = GeometryUtil::FindInRectangle2D(monsterCandidates, center, forward, action->width, action->length);
			break;
		case ShapeType::Line:
			targetMonsters = GeometryUtil::FindInLine2D(monsterCandidates, center, targetPos, action->radius);
			break;
		default:
			break;
		}

		for (auto target : targetMonsters)
		{
			CombatSystem::Instance().ApplyDamage(caster, static_pointer_cast<Object>(target), action->damage);
		}
	}
}

void SkillSystem::HandleSpawnAction(ObjectRef caster, const Vector3& targetPos, SpawnActionData* action)
{
	if (action->actionType == ActionType::SpawnProjectile)
	{
		caster->GetRoom()->SpawnProjectile(caster, action->dataId, Vector3(caster->_posInfo), (targetPos - caster->_worldPos).Normalized2D());
	}
	else if (action->actionType == ActionType::SpawnField)
		caster->GetRoom()->SpawnField(caster, action->dataId, targetPos);
}

void SkillSystem::HandleBuffAction(ObjectRef caster, const Vector3& targetPos, BuffActionData* action)
{
	// TODO : BuffSystem 추가
	BuffSystem::Instance().ApplyBuff(caster, action->buffId);
}