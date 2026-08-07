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
#ifdef USE_OPTIMIZED_MEMORY_POOLING
	_skillPools = make_shared<ChunkList>();
	_skillPools->Init<SkillInstance>();
#endif
}

void SkillSystem::ExecuteSkill(ObjectRef caster, int32 skillId, const Vector3& targetPos, int32 castId, uint64 clientSend)
{
	auto it = skillDict->find(skillId);
	if (it == skillDict->end())
		return;

	uint64 now = ::GetTickCount64();

	const Skill& skill = it->second;
#ifdef USE_OPTIMIZED_MEMORY_POOLING
	SkillInstanceRef instance = _skillPools->AllocShared<SkillInstance>();
#else
	SkillInstanceRef instance = make_shared<SkillInstance>();
#endif
	
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
			ParseEvent(creature, instance, CastState::CAST_START, event);
			instance->caster->AddSkillFlushQueue(caster, CastState::CAST_START, event);
		}
	}
	else
	{
		instance->cooldownEndTime = now + static_cast<uint64>(instance->skill->cooldown * 1000);
		creature->StartSkillCooldown(skillId, now);
	}

	_activeSkills.push_back(instance);
}

void SkillSystem::CancelCasting(ObjectRef caster, int32 castId)
{
	//printf("[Server] SkillSystem: CancelCasting caster ID = %llu\n", caster->GetId());
	if (auto creature = static_pointer_cast<Creature>(caster))
	{
		SkillInstanceRef instance = creature->GetActiveSkill();

		if (instance == nullptr) // TODO
			return;

		if (creature->IsDead())
		{
			creature->GetActiveSkill()->canceled = true;
			creature->SetActiveSkill(nullptr);
			return;
		}

		{
			Protocol::S_SKILL_EVENT event;
			ParseEvent(caster, instance, CastState::CAST_CANCEL, event);
			caster->AddSkillFlushQueue(caster, CastState::CAST_CANCEL, event);

			creature->GetActiveSkill()->canceled = true;
			creature->SetActiveSkill(nullptr);
		}
	}
}

void SkillSystem::Update()
{
#ifdef USE_OPTIMIZED_MEMORY_POOLING
	uint64 now = ::GetTickCount64();
	
	for (int32 i = 0; i < _activeSkills.size(); i++)
	{
		SkillInstanceRef instance = _activeSkills[i];
		if (instance == nullptr || instance->caster == nullptr)
		{
			_removePendings.push_back(i);
			continue;
		}

		auto creature = static_pointer_cast<Creature>(instance->caster);
		if (creature->IsDead() || instance->canceled)
		{
			creature->SetActiveSkill(nullptr);
			_removePendings.push_back(i);

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
					ParseEvent(creature, instance, CastState::CAST_SUCCESS, event);
					instance->caster->AddSkillFlushQueue(instance->caster, CastState::CAST_SUCCESS, event);
				}
				creature->StartSkillCooldown(instance->skill->id, now);
			}
		}
		// 액션 처리 시간 Update 후 Index에 따라 HandleAction호출

		const auto& actions = instance->skill->actions;
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
			//instance->canceled = true;
			creature->SetActiveSkill(nullptr);
			creature->ChangeState(Protocol::STATE_MACHINE_IDLE);

			if (auto monster = dynamic_pointer_cast<Monster>(instance->caster))
			{
				monster->_currentSkillId = -1;
			}

			_removePendings.push_back(i);
		}
	}

	DeferRemoveInstance();
#else
	uint64 now = ::GetTickCount64();

	for (auto it = _activeSkills.begin(); it != _activeSkills.end(); )
	{
		SkillInstanceRef instance = *it;
		if (instance == nullptr || instance->caster == nullptr)
		{
			it = _activeSkills.erase(it);
			continue;
		}

		auto creature = static_pointer_cast<Creature>(instance->caster);
		const auto& actions = instance->skill->actions;

		// 취소된 스킬이면 제거
		if (instance->canceled)
		{
			if (creature->GetActiveSkill() == instance)
				creature->SetActiveSkill(nullptr);

			it = _activeSkills.erase(it);
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
					ParseEvent(creature, instance, CastState::CAST_SUCCESS, event);
					instance->caster->AddSkillFlushQueue(instance->caster, CastState::CAST_SUCCESS, event);
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
		while (instance->currentActionIndex < (int32)actions.size())
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

			it = _activeSkills.erase(it);
			continue;
		}

		++it;
	}
#endif
}

void SkillSystem::AddRemovePendings(int32 index)
{
	_removePendings.push_back(index);
}

void SkillSystem::DeferRemoveInstance()
{
	// TODO : _activeSkills 의 idx 포인터 제거를 다 하고
	// 남은 포인터들만 압축해서 들고있게.
	// 그리고 SkillInstance->Release 호출까지 해야됨
	if (_removePendings.empty())
		return;

	int32 currentSize = static_cast<int32>(_activeSkills.size());
	int32 pendingSize = static_cast<int32>(_removePendings.size());

	vector<SkillInstanceRef> tempSkills;
	tempSkills.reserve(currentSize > pendingSize ? currentSize - pendingSize : 0);

	std::sort(_removePendings.begin(), _removePendings.end());

	int32 cursor = 0;
	for (int32 i = 0; i < _activeSkills.size(); ++i)
	{
		if (cursor < pendingSize && _removePendings[cursor] == i)
		{
			if (_activeSkills[i])
			{
				_activeSkills[i]->caster = nullptr;
				_activeSkills[i] = nullptr;
			}
			cursor++;
			continue;
		}

		tempSkills.push_back(_activeSkills[i]);
	}

	_activeSkills.swap(tempSkills);

	_removePendings.clear();
}

void SkillSystem::ParseEvent(ObjectRef object, SkillInstanceRef instance, const Protocol::CastState& state, OUT Protocol::S_SKILL_EVENT& event)
{
	auto creature = static_pointer_cast<Creature>(object);
	if (!creature)
		return;

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
	//auto creature = static_pointer_cast<Creature>(caster);
	//if (!creature)
	//	return;

	{
		Protocol::S_SKILL_EVENT event;
		ParseEvent(caster, instance, CastState::ACTION, event);
		caster->AddSkillFlushQueue(caster, CastState::ACTION, event);
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