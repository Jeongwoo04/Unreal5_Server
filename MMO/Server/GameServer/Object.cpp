#include "pch.h"
#include "Object.h"
#include "Monster.h"
#include "Player.h"
#include "GameMap.h"

Object::Object()
{
	_objectInfo.set_object_type(Protocol::OBJECT_TYPE_NONE);
	_posInfo.set_state(Protocol::STATE_MACHINE_IDLE);
}

Object::~Object()
{

}

void Object::Update()
{

}

void Object::OnDamaged(ObjectRef attacker, int32 damage)
{
	auto room = _room.lock();
	if (!room)
		return;

	_statInfo.set_hp(_statInfo.hp() - damage);
	_attackerId = attacker->GetId();

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

	nextPos = room->GetGameMap()->GetSafePosRayCast(_worldPos, destPos, _statInfo.speed(), blocked);

	float diff = (destPos - nextPos).Length2D();
	if (diff > 0.001f)
	{
		cout << "[Server] Collision Fail. Move GetSafePos.\n";
	}

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

	AddMoveFlushQueue(shared_from_this());

	if (GetCreatureType() == CREATURE_TYPE_MONSTER)
	{
		room->_monsterGrid.ApplyMove(static_pointer_cast<Monster>(shared_from_this()), currentGrid, _gridPos);
	}
	else if (GetCreatureType() == CREATURE_TYPE_PLAYER)
	{
		auto player = static_pointer_cast<Player>(shared_from_this());
		room->_playerGrid.ApplyMove(player, currentGrid, _gridPos);
		
		player->_isDirty = true;
		player->_hasMove = true;
	}
}

void Object::ChangeState(const Protocol::StateMachine& state)
{
	if (GetState() == state)
		return;

	_posInfo.set_state(state);
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
	_lastFlushPos = _worldPos;
}

void Object::SetSpawnRandomPos(Vector3 pos, float yaw, int32 range)
{
	pos._x = pos._x + Utils::GetRandom(-3000.f, 3000.f);
	pos._y = pos._y + Utils::GetRandom(-3000.f, 3000.f);
	SetSpawnPos(pos, yaw);
}

void Object::AddSpawnFlushQueue(ObjectRef obj)
{
	auto room = GetRoom();
	if (!room)
		return;

	room->_deferFlushQueue.push_back({ obj, Type::SPAWN });
}

void Object::AddMoveFlushQueue(ObjectRef obj)
{
	auto room = GetRoom();
	if (!room)
		return;

	room->_deferFlushQueue.push_back({ obj, Type::MOVE });
}

void Object::AddSkillFlushQueue(ObjectRef obj, const Protocol::CastState& state, const Protocol::S_SKILL_EVENT& event)
{
	auto room = GetRoom();
	if (!room)
		return;

	switch (state)
	{
	case CastState::CAST_START:
	{
		room->_deferFlushQueue.push_back({ obj, Type::CAST_START, event });
	} break;
	case CastState::CAST_CANCEL:
	{
		room->_deferFlushQueue.push_back({ obj, Type::CAST_CANCEL, event });
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

void Object::AddHitFlushQueue(ObjectRef obj)
{
	auto room = GetRoom();
	if (!room)
		return;

	room->_deferFlushQueue.push_back({ obj, Type::HIT });
}

void Object::AddDieFlushQueue(ObjectRef obj)
{
	auto room = GetRoom();
	if (!room)
		return;

	room->_deferFlushQueue.push_back({ obj, Type::DIE });
}

void Object::AddDespawnFlushQueue(ObjectRef obj)
{
	auto room = GetRoom();
	if (!room)
		return;

	room->_deferFlushQueue.push_back({ obj, Type::DESPAWN });
}

void Object::FlushStateInit()
{
	_lastFlushPos = _worldPos;
}

bool Object::IsMoveBatch()
{
	return !IsDead() && (_lastFlushPos - _worldPos).LengthSquared2D() >= 0.0025;
}

void Object::ApplyBuffEffect(BuffInstance& buff, float deltaTime)
{
	int32 hp = _statInfo.hp();
	hp += static_cast<int32>(buff.effectPerTick);

	_statInfo.set_hp(hp);
	// TODO: 다른 스탯, 효과 처리
}