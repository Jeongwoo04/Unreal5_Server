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

void SkillSystem::ExecuteSkill(ObjectRef caster, int32 skillId, const Vector3& targetPos, int32 castId)
{
	auto it = skillDict->find(skillId);
	if (it == skillDict->end())
		return;

	uint64 now = GetTickCount64();

	const Skill& skill = it->second;

	SkillInstanceRef instance = make_shared<SkillInstance>();
	instance->caster = caster;
	instance->skill = &skill;
	instance->targetPos = targetPos;
	instance->castId = castId;
	instance->isCasting = (skill.castTime > 0.0f);
	instance->actionDelayElapsed = 0.f;
	instance->currentActionIndex = 0;

	auto creature = static_pointer_cast<Creature>(caster);
	if (creature == nullptr)
		return;

	if (instance->isCasting == true)
	{
		creature->ChangeState(Protocol::STATE_MACHINE_CASTING);
		creature->StartSkillCast(skillId, now, skill.castTime);
		
		// 캐스팅시작 패킷전송
		Protocol::S_SKILL_CAST_START pkt;
		pkt.set_object_id(caster->GetId());
		pkt.set_skillid(skillId);
		pkt.set_castid(castId);
		pkt.set_servernow(now);
		pkt.set_castendtime(now + static_cast<uint64>(instance->skill->castTime * 1000));

		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
		if (auto room = GetRoom())
			room->BroadcastNearby(sendBuffer, caster->_gridPos, caster->GetId());
	}
	else
	{
		if (!skill.actions.empty())
		{
			ActionData* action = instance->skill->actions[instance->currentActionIndex];
			HandleAction(caster, targetPos, action, instance);
			
			creature->StartSkillCooldown(skillId, now);
			creature->ChangeState(Protocol::STATE_MACHINE_SKILL);
		}
	}

	creature->SetActiveSkill(instance);
	activeSkills.push_back(instance);
}

void SkillSystem::CancelCasting(ObjectRef caster, int32 castId)
{
	if (auto creature = static_pointer_cast<Creature>(caster))
	{
		if (creature->GetActiveSkill() == nullptr)
			return;

		Protocol::S_SKILL_CAST_CANCEL cancelPkt;
		cancelPkt.set_object_id(creature->GetId());
		cancelPkt.set_skillid(creature->GetActiveSkill()->skill->id);
		cancelPkt.set_castid(castId);

		creature->GetActiveSkill()->canceled = true;
		creature->SetActiveSkill(nullptr);

		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(cancelPkt);
		if (auto room = GetRoom())
			room->BroadcastNearby(sendBuffer, caster->_gridPos);
	}
}

void SkillSystem::Update(float deltaTime)
{
	uint64 now = GetTickCount64();

	for (auto it = activeSkills.begin(); it != activeSkills.end(); )
	{
		SkillInstanceRef instance = *it;
		auto creature = static_pointer_cast<Creature>(instance->caster);

		if (instance == nullptr || creature == nullptr)
		{
			it = activeSkills.erase(it);
			continue;
		}

		// 취소된 스킬이면 제거
		if (instance->canceled)
		{
			if (creature->GetActiveSkill() == instance)
				creature->SetActiveSkill(nullptr);

			it = activeSkills.erase(it);
			continue;
		}

		// 캐스팅 중이면 캐스팅 처리
		if (instance->isCasting)
		{
			if (now >= creature->GetSkillState(instance->skill->id)->GetCastEndTime())
			{
				//cout << instance->skill->name << " 캐스팅 완료" << endl;
				instance->isCasting = false;
				instance->actionDelayElapsed = 0.f;
				instance->caster->ChangeState(Protocol::STATE_MACHINE_IDLE);
				// 캐스팅 완료 패킷 전송
				{
					S_SKILL_CAST_SUCCESS pkt;
					pkt.set_object_id(instance->caster->GetId());
					pkt.set_skillid(instance->skill->id);
					pkt.set_castid(instance->castId);
					auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
					GetRoom()->BroadcastNearby(sendBuffer, instance->caster->_gridPos);
				}
				creature->StartSkillCooldown(instance->skill->id, now);

				// 캐스팅이 끝난 후 나머지 액션 실행
				if (!instance->skill->actions.empty())
				{
					ActionData* action = instance->skill->actions[instance->currentActionIndex];
					HandleAction(instance->caster, instance->targetPos, action, instance);
					instance->currentActionIndex++;
					instance->actionDelayElapsed = 0.f;
				}
			}
			++it;
			continue;
		}

		// 캐스팅이 끝났다면 액션 실행
		if (instance->currentActionIndex < (int32)instance->skill->actions.size())
		{
			instance->actionDelayElapsed += deltaTime;
			ActionData* action = instance->skill->actions[instance->currentActionIndex];

			if (instance->actionDelayElapsed >= action->actionDelay)
			{
				HandleAction(instance->caster, instance->targetPos, action, instance);
				instance->currentActionIndex++;
				instance->actionDelayElapsed = 0.f; // 다음 액션 준비
			}
		}

		if (instance->currentActionIndex >= (int32)instance->skill->actions.size())
		{
			//cout << instance->skill->name << "스킬 완료" << endl;
			creature->SetActiveSkill(nullptr);

			if (auto monster = dynamic_pointer_cast<Monster>(instance->caster))
			{
				monster->_currentSkillId = -1;
				monster->ChangeState(Protocol::STATE_MACHINE_IDLE);
			}

			it = activeSkills.erase(it);
		}
	}
}

void SkillSystem::HandleAction(ObjectRef caster, const Vector3& targetPos, ActionData* action, SkillInstanceRef instance)
{
	//cout << static_cast<int32>(action->actionType) << " Handle Action" << endl;

	int32 idx = instance->currentActionIndex;

	{
		Protocol::S_SKILL pkt;
		pkt.set_object_id(caster->GetId());
		pkt.mutable_skill_info()->set_skillid(instance->skill->id);
		pkt.mutable_skill_info()->set_actionindex(idx);
		pkt.mutable_skill_info()->mutable_targetpos()->set_x(targetPos._x);
		pkt.mutable_skill_info()->mutable_targetpos()->set_y(targetPos._y);
		pkt.mutable_skill_info()->mutable_targetpos()->set_z(targetPos._z);

		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
		if (auto room = GetRoom())
			room->BroadcastNearby(sendBuffer, caster->_gridPos);
	}

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
	if (auto room = GetRoom())
		room->BroadcastMove(caster->_posInfo);
}

void SkillSystem::HandleAttackAction(ObjectRef caster, const Vector3& targetPos, AttackActionData* action)
{
	Vector3 center = Vector3(caster->_posInfo);
	Vector3 forward = Vector3::YawToDir2D(caster->_posInfo.yaw());

	if (caster->GetCreatureType() == CREATURE_TYPE_MONSTER)
	{
		auto playerCandidates = GetRoom()->_playerGrid.FindAroundFloat(Vector2Int(caster->_posInfo), action->radius);
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
		auto monsterCandidates = GetRoom()->_monsterGrid.FindAroundFloat(Vector2Int(caster->_posInfo), action->radius);
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
		GetRoom()->SpawnProjectile(caster, action->dataId, Vector3(caster->_posInfo), (targetPos - caster->_worldPos).Normalized2D());
	}
	else if (action->actionType == ActionType::SpawnField)
		GetRoom()->SpawnField(caster, action->dataId, targetPos);
}

void SkillSystem::HandleBuffAction(ObjectRef caster, const Vector3& targetPos, BuffActionData* action)
{
	// TODO : BuffSystem 추가
	BuffSystem::Instance().ApplyBuff(caster, action->buffId);
}