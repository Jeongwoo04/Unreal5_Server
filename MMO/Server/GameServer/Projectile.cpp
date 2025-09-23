#include "pch.h"
#include "Projectile.h"

Projectile::Projectile()
{
	_objectInfo.set_object_type(Protocol::OBJECT_TYPE_PROJECTILE);
}

Projectile::~Projectile()
{

}

void Projectile::Update(float deltaTime)
{
    if (GetOwner() == nullptr || GetOwner()->GetRoom() == nullptr)
        return;

    auto room = GetRoom();

    if (_moveDistance >= _data->distance)
    {
        room->AddRemoveList(shared_from_this());
        return;
    }

    Vector3 currentPos = _worldPos;
    Vector3 destPos = _worldPos + (_dir * _data->speed * deltaTime);
    
    MoveToNextPos(destPos);
    _posInfo.set_state(Protocol::STATE_MACHINE_MOVING);
    room->BroadcastMove(_posInfo);

    ObjectRef target = nullptr;

    if (GetOwner()->GetCreatureType() == Protocol::CREATURE_TYPE_MONSTER)
    {
        PlayerRef playerTarget = room->_playerGrid.FindNearestOnPath(currentPos, destPos, _data->radius);
        if (playerTarget)
        {
            target = playerTarget;
        }
    }        
    else if (GetOwner()->GetCreatureType() == Protocol::CREATURE_TYPE_PLAYER)
    {
        MonsterRef monsterTarget = room->_monsterGrid.FindNearestOnPath(currentPos, destPos, _data->radius);
        if (monsterTarget)
        {
            target = monsterTarget;
        }
    }

    if (target)
    {
        target->OnDamaged(GetOwner(), GetOwner()->_statInfo.attack() + _data->damage);
        room->AddRemoveList(shared_from_this());
        return;
    }
    else
    {
        ;
    }

    if (_worldPos != destPos)
    {
        room->AddRemoveList(shared_from_this());
        return;
    }

    _moveDistance += _data->speed * deltaTime;
}
