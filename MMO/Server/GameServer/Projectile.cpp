#include "pch.h"
#include "Projectile.h"
#include "Monster.h"
#include "Player.h"

Projectile::Projectile()
{
	_objectInfo.set_object_type(Protocol::OBJECT_TYPE_PROJECTILE);
}

Projectile::~Projectile()
{
	//cout << "Projectile " << GetId() << " Destructor" << endl;
}

void Projectile::Update()
{
	auto room = GetRoom();
	if (room == nullptr)
		return;

	if (GetOwner() == nullptr)
	{
		room->AddRemoveList(shared_from_this());
		return;
	}

	if (_moveDistance >= _data->distance)
	{
		room->AddRemoveList(shared_from_this());
		return;
	}

	Vector3 currentPos = _worldPos;
	Vector3 destPos = _worldPos + (_dir * _data->speed * ServerTickInterval);
	Vector2Int blocked;
	
	MoveToNextPos(destPos, &_dir, &blocked);
	AddMoveFlushQueue(shared_from_this());

	ObjectRef target = nullptr;

	if (GetOwner()->GetCreatureType() == Protocol::CREATURE_TYPE_MONSTER)
	{
		PlayerRef playerTarget = room->_playerGrid.FindNearestOnPath(currentPos, destPos, _data->radius);
		if (playerTarget && !playerTarget->IsDead())
		{
			target = playerTarget;
		}
	}        
	else if (GetOwner()->GetCreatureType() == Protocol::CREATURE_TYPE_PLAYER)
	{
		MonsterRef monsterTarget = room->_monsterGrid.FindNearestOnPath(currentPos, destPos, _data->radius);
		if (monsterTarget && !monsterTarget->IsDead())
		{
			target = monsterTarget;
		}
	}

	if (target && !target->IsDead())
	{
		target->OnDamaged(GetOwner(), GetOwner()->_statInfo.attack() + _data->damage);
		
		target->AddHitFlushQueue(target);
		room->AddRemoveList(shared_from_this());
		return;
	}
	else
	{
		;
	}

	if (!room->GetGameMap()->CanGo(blocked))
	{
		room->AddRemoveList(shared_from_this());
		return;
	}

	_moveDistance += _data->speed * ServerTickInterval;
}
