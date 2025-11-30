#include "pch.h"
#include "Player.h"
#include "Monster.h"
#include "SkillState.h"

Player::Player()
{
	_objectInfo.set_creature_type(Protocol::CREATURE_TYPE_PLAYER);

	for (auto& it : DataManager::Instance().SkillDict)
	{
		int32 id = it.first;
		const Skill& s = it.second;
		_skillStates.emplace(id, make_shared<SkillState>(id, s.cooldown));
	}
}

Player::~Player()
{

}

void Player::OnDamaged(ObjectRef attacker, int32 damage)
{
	Object::OnDamaged(attacker, damage);
}

void Player::OnDead(ObjectRef attacker)
{
	auto room = GetRoom();
	if (room == nullptr)
		return;

	_posInfo.set_state(Protocol::STATE_MACHINE_DEAD);
	if (attacker->GetCreatureType() == CREATURE_TYPE_MONSTER)
		static_pointer_cast<Monster>(attacker)->SetPlayer(nullptr);

	AddDieFlushQueue(shared_from_this());
	room->AddRemoveList(shared_from_this());
}

void Player::AddMoveFlushQueue(ObjectRef obj)
{
	auto room = GetRoom();
	if (!room)
		return;

	if (_isDirty == false)
		room->_immediateFlushQueue.push_back({ obj, Type::MOVE });
	else
		return;
}

void Player::AddSkillFlushQueue(ObjectRef obj, const Protocol::CastState& state, const Protocol::S_SKILL_EVENT& event)
{
	auto room = GetRoom();
	if (!room)
		return;

	switch (state)
	{
	case CastState::CAST_START:
	{
		room->_immediateFlushQueue.push_back({ obj, Type::CAST_START, event });
	} break;
	case CastState::CAST_CANCEL:
	{
		room->_immediateFlushQueue.push_back({ obj, Type::CAST_CANCEL, event });
	} break;
	case CastState::CAST_SUCCESS:
	{
		room->_deferFlushQueue.push_back({ obj, Type::CAST_SUCCESS, event });
	} break;
	case CastState::ACTION:
	{
		room->_deferFlushQueue.push_back({ obj, Type::SKILL_ACTION, event });
	} break;
	default:
		break;
	}
}

void Player::FlushStateInit()
{
	Object::FlushStateInit();
	_isDirty = false;
}
