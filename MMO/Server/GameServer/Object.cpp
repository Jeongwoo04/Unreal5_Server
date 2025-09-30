#include "pch.h"
#include "Object.h"
#include "GameMap.h"

Object::Object()
{
	_objectInfo.set_object_type(Protocol::OBJECT_TYPE_NONE);
	_posInfo.set_state(Protocol::STATE_MACHINE_IDLE);
}

Object::~Object()
{

}

void Object::Update(float deltaTime)
{

}

void Object::OnDamaged(ObjectRef attacker, int32 damage)
{
	auto room = _room.lock();
	if (!room)
		return;

	_statInfo.set_hp(_statInfo.hp() - damage);

	if (_statInfo.hp() <= 0)
	{
		OnDead(attacker);
		return;
	}
}

void Object::OnDead(ObjectRef attacker)
{

}

void Object::MoveToNextPos(const Vector3& destPos, Vector3* dir, Vector2Int* blocked)
{
	auto room = GetRoom();
	if (room == nullptr)
		return;

	Vector3 nextPos;
	Vector3 currentPos = _worldPos;
	Vector2Int currentGrid = _gridPos;

	nextPos = GetRoom()->GetGameMap()->GetSafePosRayCast(_worldPos, destPos, blocked);
	Vector3 direction;
	if (dir == nullptr)
		direction = (nextPos - currentPos).Normalized2D();
	else
		direction = *dir;

	if (direction.Length2D() > 1e-6f)
	{
		_posInfo.set_yaw(Vector3::DirToYaw2D(direction));
	}

	_posInfo.set_object_id(GetId());
	_posInfo.set_speed(_statInfo.speed());
	_posInfo.set_x(nextPos._x);
	_posInfo.set_y(nextPos._y);
	_worldPos = nextPos;
	_gridPos = Vector2Int(_posInfo);

	if (GetCreatureType() == CREATURE_TYPE_MONSTER)
		room->_monsterGrid.ApplyMove(static_pointer_cast<Monster>(shared_from_this()), currentGrid, _gridPos);
	else if (GetCreatureType() == CREATURE_TYPE_PLAYER)
		room->_playerGrid.ApplyMove(static_pointer_cast<Player>(shared_from_this()), currentGrid, _gridPos);
}

void Object::ChangeState(const Protocol::StateMachine& state)
{
	if (GetState() == state)
		return;

	_posInfo.set_state(state);
	auto room = GetRoom();
	if (room == nullptr)
		return;

	room->BroadcastMove(_posInfo, GetId());
}

void Object::SetId(uint64 id)
{
	_objectInfo.set_object_id(id);
	_posInfo.set_object_id(id);
}

void Object::AddBuff(const BuffInfo& buff)
{
	// 기존 Buff 중첩 처리
	auto it = find_if(_activeBuffs.begin(), _activeBuffs.end(), [&](const BuffInstance& b)
		{
			return b.buff == &buff;
		});
	
	if (it != _activeBuffs.end())
	{
		// 중첩 시 갱신
		it->duration = buff.duration;
	}
	else
	{
		// 새 Buff 추가
		BuffInstance buffInstance;
		buffInstance.target = shared_from_this();
		buffInstance.buff = &buff;
		buffInstance.duration = buff.duration;
		buffInstance.effectPerTick = buff.effectPerTick;
		_activeBuffs.push_back(move(buffInstance));
	}
}

void Object::UpdateBuffs()
{
	for (auto it = _activeBuffs.begin(); it != _activeBuffs.end(); )
	{
		BuffInstance& buff = *it;
		buff.duration -= 0.1f;

		// Tick 효과 적용
		ApplyBuffEffect(buff, 0.1f);
		if (buff.duration <= 0.f)
			it = _activeBuffs.erase(it);
		else
			++it;
	}
}

void Object::SetPosInfo(const PosInfo& posInfo)
{
	_posInfo.CopyFrom(posInfo);
	_objectInfo.mutable_pos_info()->CopyFrom(_posInfo);
	_gridPos = WorldToGrid(Vector3(posInfo));
	_worldPos = Vector3(posInfo);
}

void Object::SetSpawnPos(const Vector3& pos, float yaw)
{
	_posInfo.set_x(pos._x);
	_posInfo.set_y(pos._y);
	_posInfo.set_z(100.f);
	_posInfo.set_yaw(yaw);
	_objectInfo.mutable_pos_info()->CopyFrom(_posInfo);
	_gridPos = WorldToGrid(Vector3(_posInfo));
	_worldPos = Vector3(_posInfo);
}

void Object::SetSpawnRandomPos(Vector3 pos, float yaw)
{
	pos = pos + Utils::GetRandom(0.f, 500.f);
	SetSpawnPos(pos, yaw);
}