#include "pch.h"
#include "Monster.h"

Monster::Monster()
{
	_objectInfo.set_creature_type(Protocol::CREATURE_TYPE_MONSTER);

	// TODO : Stat
	_posInfo.set_state(Protocol::STATE_MACHINE_IDLE);

}

Monster::~Monster()
{

}

void Monster::Update()
{
	switch (_posInfo.state())
	{
	case STATE_MACHINE_IDLE:
		UpdateIdle();
		break;
	case STATE_MACHINE_PATROL:
		UpdatePatrol();
		break;
	case STATE_MACHINE_MOVING:
		UpdateMoving();
		break;
	case STATE_MACHINE_SKILL:
		UpdateSkill();
		break;
	case STATE_MACHINE_DEAD:
		UpdateDead();
		break;
	}
}

void Monster::UpdateIdle()
{
	const uint64 tick = GetTickCount64();
	if (_nextSearchTick > tick)
		return;

	_nextSearchTick = tick + 3000;

	// TODO : target find 로직 + 못찾으면 Patrol AI
	PlayerRef target = nullptr;

	if (target == nullptr)
		_posInfo.set_state(Protocol::STATE_MACHINE_PATROL);

	if (target)
	{
		SetTarget(target);
		_posInfo.set_state(Protocol::STATE_MACHINE_MOVING);
		return;
	}

	if (DoPatrol())
		_posInfo.set_state(Protocol::STATE_MACHINE_PATROL);
}

void Monster::UpdatePatrol()
{
	const uint64 tick = GetTickCount64();

	if (_nextMoveTick > tick)
		return;

	// TODO : stat 정보 추가하기
	const int32 speed = 5;
	const int32 moveTick = static_cast<int32>(1000 / speed);
	_nextMoveTick = tick + moveTick;

	// DoPatrol
	PlayerRef target = nullptr;
	{
		if (target == nullptr || target->GetRoom() != GetRoom())
		{
			SetTarget(nullptr);
			_posInfo.set_state(Protocol::STATE_MACHINE_IDLE);
			BroadcastMoveUpdate();
		}
	}
}

void Monster::UpdateMoving()
{
	switch (_posInfo.move_type())
	{
	case Protocol::MOVE_STATE_RUN:
		break;
	case Protocol::MOVE_STATE_JUMP:
		break;
	}
}

void Monster::UpdateSkill()
{

}

void Monster::UpdateDead()
{

}

bool Monster::DoPatrol()
{

	return true;
}

void Monster::OnDamaged(ObjectRef attacker, int32 damage)
{
}

void Monster::OnDead(ObjectRef attacker)
{
}

void Monster::BroadcastMoveUpdate()
{
	S_MOVE movePkt;
	movePkt.CopyFrom(_posInfo);

	if (auto room = GetRoom())
	{
		auto sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);
		room->BroadcastMove(sendBuffer);
	}
}

void Monster::SetLastPos()
{
	_lastPos.CopyFrom(_posInfo);
}
