#include "pch.h"
#include "SkillSystem.h"
#include "SkillState.h"
#include "ObjectManager.h"
#include "DataManager.h"
#include "CombatSystem.h"
#include "BuffSystem.h"
#include "Player.h"
#include "Monster.h"
#include "Room.h"

void SkillSystem::Init()
{
	skillDict = &DataManager::Instance().SkillDict;
}

void SkillSystem::ExecuteSkill(ObjectRef caster, int32 skillId, const Vector3& targetPos, int32 castId, uint64 clientSend)
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
	instance->clientSend = clientSend;
	instance->serverNow = now;
	instance->castEndTime = now + static_cast<uint64>(instance->skill->castTime * 1000);
	instance->isCasting = (skill.castTime > 0.0f);
	instance->actionDelayElapsed = 0.f;
	instance->currentActionIndex = 0;

	auto creature = static_pointer_cast<Creature>(caster);
	if (creature == nullptr)
		return;

	creature->SetActiveSkill(instance);

	if (instance->isCasting == true)
	{
		creature->ChangeState(Protocol::STATE_MACHINE_CASTING);
		creature->StartSkillCast(skillId, now, skill.castTime);
		
		{
			instance->caster->_hasMove = false;
			Protocol::S_SKILL_EVENT event;
			ParseEvent(creature, CastState::CAST_START, event);
			instance->caster->AddSkillFlushQueue(caster, CastState::CAST_START, event);
		}

		// 캐스팅시작 패킷전송
		//Protocol::S_SKILL_CAST_START pkt;
		//pkt.set_object_id(caster->GetId());
		//pkt.set_skillid(skillId);
		//pkt.set_castid(castId);
		//pkt.set_clientsend(clientSend);
		//pkt.set_servernow(now);
		//pkt.set_castendtime(now + static_cast<uint64>(instance->skill->castTime * 1000));
		//pkt.set_yaw(caster->_posInfo.yaw());

		//if (creature->GetCreatureType() == Protocol::CREATURE_TYPE_MONSTER)
			//printf("[Server] SkillSystem: Monster SkillCastStart\n");
		//else
			//printf("[Server] SkillSystem: Player SkillCastStart\n");

		//auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
		//if (auto room = GetRoom())
		//	room->BroadcastNearby(sendBuffer, caster->_worldPos);
	}
	else
	{
		instance->cooldownEndTime = now + static_cast<uint64>(instance->skill->cooldown * 1000);
		creature->StartSkillCooldown(skillId, now);
	}

	activeSkills.push_back(instance);
}

void SkillSystem::CancelCasting(ObjectRef caster, int32 castId)
{
	//printf("[Server] SkillSystem: CancelCasting caster ID = %llu\n", caster->GetId());
	if (auto creature = static_pointer_cast<Creature>(caster))
	{
		if (creature->GetActiveSkill() == nullptr)
			return;

		if (caster->IsDead())
			return;

		{
			Protocol::S_SKILL_EVENT event;
			ParseEvent(caster, CastState::CAST_CANCEL, event);
			caster->AddSkillFlushQueue(caster, CastState::CAST_CANCEL, event);

			creature->GetActiveSkill()->canceled = true;
			creature->SetActiveSkill(nullptr);
		}

		//Protocol::S_SKILL_CAST_CANCEL cancelPkt;
		//cancelPkt.set_object_id(creature->GetId());
		//cancelPkt.set_skillid(creature->GetActiveSkill()->skill->id);
		//cancelPkt.set_castid(castId);

		//creature->GetActiveSkill()->canceled = true;
		//creature->SetActiveSkill(nullptr);

		//auto sendBuffer = ServerPacketHandler::MakeSendBuffer(cancelPkt);
		//if (auto room = GetRoom())
		//	room->BroadcastNearby(sendBuffer, caster->_worldPos);
	}
}

void SkillSystem::Update()
{
	uint64 now = GetTickCount64();
	
	for (auto it = activeSkills.begin(); it != activeSkills.end(); )
	{
		SkillInstanceRef instance = *it;
		if (instance == nullptr || instance->caster == nullptr)
		{
			it = activeSkills.erase(it);
			continue;
		}

		auto creature = static_pointer_cast<Creature>(instance->caster);
		const auto& actions = instance->skill->actions;

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
				instance->isCasting = false;
				instance->actionDelayElapsed = 0.f;
				// 캐스팅 완료 패킷 전송
				{
					Protocol::S_SKILL_EVENT event;
					instance->cooldownEndTime = now + static_cast<uint64>(instance->skill->cooldown * 1000);
					ParseEvent(creature, CastState::CAST_SUCCESS, event);
					instance->caster->AddSkillFlushQueue(instance->caster, CastState::CAST_SUCCESS, event);

					//S_SKILL_CAST_SUCCESS pkt;
					//pkt.set_object_id(instance->caster->GetId());
					//pkt.set_skillid(instance->skill->id);
					//pkt.set_castid(instance->castId);
					//pkt.set_servernow(now);
					//pkt.set_cooldownendtime(now + static_cast<uint64>(instance->skill->cooldown * 1000));
					//if (!actions.empty())
					//{
					//	pkt.mutable_skill_info()->set_actionindex(instance->currentActionIndex);
					//	pkt.mutable_skill_info()->mutable_targetpos()->set_x(instance->targetPos._x);
					//	pkt.mutable_skill_info()->mutable_targetpos()->set_y(instance->targetPos._y);
					//	pkt.mutable_skill_info()->mutable_targetpos()->set_z(instance->targetPos._z);
					//}
					////printf("[Server] SkillSystem: SkillCastSuccess %s [%d]\n", instance->skill->name.c_str(), instance->currentActionIndex);

					//auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
					//if (auto room = GetRoom())
					//	room->BroadcastNearby(sendBuffer, instance->caster->_worldPos);
				}
				creature->StartSkillCooldown(instance->skill->id, now);
			}
			else
			{
				++it;
				continue;
			}
		}
		// 액션 처리 시간 Update 후 Index에 따라 HandleAction호출
		while(instance->currentActionIndex < (int32)actions.size())
		{
			ActionData* action = actions[instance->currentActionIndex];
			instance->actionDelayElapsed += ServerTickInterval;

			if (instance->actionDelayElapsed < action->actionDelay)
				break;

			HandleAction(instance->caster, instance->targetPos, action, instance);
			instance->currentActionIndex++;
			instance->actionDelayElapsed = 0.f;
		}
		// Action 종료 -> 제거
		if (instance->currentActionIndex >= (int32)actions.size())
		{
			instance->canceled = true;
			creature->SetActiveSkill(nullptr);
			creature->ChangeState(Protocol::STATE_MACHINE_IDLE);

			if (auto monster = dynamic_pointer_cast<Monster>(instance->caster))
			{
				monster->_currentSkillId = -1;
			}

			it = activeSkills.erase(it);
			continue;
		}

		++it;
	}
}

void SkillSystem::ParseEvent(ObjectRef object, const Protocol::CastState& state, OUT Protocol::S_SKILL_EVENT& event)
{
	auto creature = static_pointer_cast<Creature>(object);
	if (!creature)
		return;

	SkillInstanceRef instance = creature->GetActiveSkill();

	switch (state)
	{
	case Protocol::CAST_START:
	{
		Protocol::S_SKILL_CAST_START pkt;
		pkt.set_object_id(object->GetId());
		pkt.set_skillid(instance->skill->id);
		pkt.set_castid(instance->castId);
		pkt.set_clientsend(instance->clientSend);
		pkt.set_servernow(instance->serverNow);
		pkt.set_castendtime(instance->castEndTime);
		pkt.mutable_pos()->CopyFrom(object->_posInfo);

		event.mutable_cast_start()->CopyFrom(pkt);
		event.set_caststate(state);
	} break;
	case Protocol::CAST_CANCEL:
	{
		Protocol::S_SKILL_CAST_CANCEL pkt;
		pkt.set_object_id(object->GetId());
		pkt.set_skillid(instance->skill->id);
		pkt.set_castid(instance->castId);

		event.mutable_cast_cancel()->CopyFrom(pkt);
		event.set_caststate(state);
	} break;
	case Protocol::CAST_SUCCESS:
	{
		Protocol::S_SKILL_CAST_SUCCESS pkt;
		pkt.set_object_id(object->GetId());
		pkt.set_skillid(instance->skill->id);
		pkt.set_castid(instance->castId);
		pkt.set_servernow(instance->serverNow);
		pkt.set_cooldownendtime(instance->cooldownEndTime);
		pkt.set_actionindex(0);
		pkt.mutable_targetpos()->set_x(instance->targetPos._x);
		pkt.mutable_targetpos()->set_y(instance->targetPos._y);
		pkt.mutable_targetpos()->set_z(instance->targetPos._z);

		event.mutable_cast_success()->CopyFrom(pkt);
		event.set_caststate(state);
	} break;
	case Protocol::ACTION:
	{
		Protocol::S_ACTION pkt;
		pkt.set_object_id(object->GetId());
		pkt.set_skillid(instance->skill->id);
		pkt.set_actionindex(instance->currentActionIndex);
		pkt.mutable_targetpos()->set_x(instance->targetPos._x);
		pkt.mutable_targetpos()->set_y(instance->targetPos._y);
		pkt.mutable_targetpos()->set_z(instance->targetPos._z);

		if (instance->currentActionIndex == 0)
			pkt.set_cooldownendtime(instance->cooldownEndTime);

		event.mutable_action()->CopyFrom(pkt);
		event.set_caststate(state);
	} break;
	default:
		break;
	}
}

// TODO: struct Action -> class Action
void SkillSystem::HandleAction(ObjectRef caster, const Vector3& targetPos, ActionData* action, SkillInstanceRef instance)
{
	int32 idx = instance->currentActionIndex;
	caster->ChangeState(Protocol::STATE_MACHINE_SKILL);
	auto creature = static_pointer_cast<Creature>(caster);
	if (!creature)
		return;

	//if (caster->GetCreatureType() == Protocol::CREATURE_TYPE_MONSTER)
	//	printf("[Server] Monster SkillSystem: HandleAction %s [%d]\n", instance->skill->name.c_str(), idx);
	//else
	//	printf("[Server] Player SkillSystem: HandleAction %s [%d]\n", instance->skill->name.c_str(), idx);

	{
		Protocol::S_SKILL_EVENT event;
		ParseEvent(creature, CastState::ACTION, event);
		caster->AddSkillFlushQueue(caster, CastState::ACTION, event);
	}

	//if (idx != 0)
	//{
		//Protocol::S_SKILL pkt;
		//pkt.set_object_id(caster->GetId());
		//pkt.mutable_skill_info()->set_skillid(instance->skill->id);
		//pkt.mutable_skill_info()->set_actionindex(idx);
		//pkt.mutable_skill_info()->mutable_targetpos()->set_x(targetPos._x);
		//pkt.mutable_skill_info()->mutable_targetpos()->set_y(targetPos._y);
		//pkt.mutable_skill_info()->mutable_targetpos()->set_z(targetPos._z);

		//auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
		//if (auto room = GetRoom())
		//	room->BroadcastNearby(sendBuffer, caster->_worldPos);
	//}	

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
	Vector2Int blocked;
	caster->MoveToNextPos(targetPos, nullptr, &blocked);
	// Flush?
	if (auto room = GetRoom())
		room->BroadcastMove(caster->_posInfo);
}

void SkillSystem::HandleAttackAction(ObjectRef caster, const Vector3& targetPos, AttackActionData* action)
{
	Vector3 center = caster->_worldPos;
	Vector3 forward = Vector3::YawToDir2D(caster->_posInfo.yaw());

	auto room = GetRoom();
	if (room == nullptr)
		return;

	if (caster->GetCreatureType() == CREATURE_TYPE_MONSTER)
	{
		auto playerCandidates = room->_playerGrid.FindAroundFloat(caster->_worldPos, action->radius);
		vector<PlayerRef> targetPlayers;

		switch (action->shape)
		{
		case ShapeType::Circle:
			targetPlayers = playerCandidates;
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

		for (auto target : targetPlayers)
		{
			CombatSystem::Instance().ApplyDamage(caster, target, action->damage);

			target->AddHitFlushQueue(target);
		}
	}
	else if (caster->GetCreatureType() == CREATURE_TYPE_PLAYER)
	{
		auto monsterCandidates = room->_monsterGrid.FindAroundFloat(caster->_worldPos, action->radius);
		vector<MonsterRef> targetMonsters;

		switch (action->shape)
		{
		case ShapeType::Circle:
			targetMonsters = monsterCandidates;
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
			CombatSystem::Instance().ApplyDamage(caster, target, action->damage);

			target->AddHitFlushQueue(target);
		}
	}
}

void SkillSystem::HandleSpawnAction(ObjectRef caster, const Vector3& targetPos, SpawnActionData* action)
{
	if (action->actionType == ActionType::SpawnProjectile)
	{
		auto room = GetRoom();
		if (room)
		{
			Vector3 dir = (targetPos - caster->_worldPos).Normalized2D();
			room->SpawnProjectile(caster, action->dataId, caster->_worldPos, dir);
		}
	}
	else if (action->actionType == ActionType::SpawnField)
	{
		auto room = GetRoom();
		if (room)
		{
			room->SpawnField(caster, action->dataId, targetPos);
		}
	}
}

void SkillSystem::HandleBuffAction(ObjectRef caster, const Vector3& targetPos, BuffActionData* action)
{
	// TODO : BuffSystem 추가
	BuffSystem::Instance().ApplyBuff(caster, action->buffId);
}