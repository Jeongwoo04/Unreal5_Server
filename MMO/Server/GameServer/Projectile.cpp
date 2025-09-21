#include "pch.h"
#include "Projectile.h"

Projectile::Projectile()
{
	_objectInfo.set_object_type(Protocol::OBJECT_TYPE_PROJECTILE);
}

Projectile::~Projectile()
{

}

void Projectile::Update()
{
    if (GetOwner() == nullptr || GetOwner()->GetRoom() == nullptr)
        return;

    constexpr float deltaTime = 0.1f;

    auto room = GetRoom();

    if (_moveDistance >= _data->distance)
    {
        room->AddRemoveList(shared_from_this());
        return;
    }

    Vector2Int blocked;
    Vector3 from = _worldPos;
    Vector3 to = room->GetGameMap()->GetSafePosRayCast(from, _worldPos + (_dir * _data->speed * deltaTime), &blocked);

    ObjectRef target = nullptr;

    if (GetOwner()->GetCreatureType() == Protocol::CREATURE_TYPE_MONSTER)
    {
        PlayerRef playerTarget = room->_playerGrid.FindNearestOnPath(from, to, _data->radius);
        if (playerTarget)
        {
            target = playerTarget;
        }
    }        
    else if (GetOwner()->GetCreatureType() == Protocol::CREATURE_TYPE_PLAYER)
    {
        MonsterRef monsterTarget = room->_monsterGrid.FindNearestOnPath(from, to, _data->radius);
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

    if (!room->GetGameMap()->CanGo(blocked))
    {
        room->AddRemoveList(shared_from_this());
        return;
    }

    _posInfo.set_x(to._x);
    _posInfo.set_y(to._y);
    _worldPos = to;
    _gridPos = Vector2Int(_posInfo);

    S_MOVE movePkt;
    movePkt.mutable_info()->CopyFrom(_posInfo);
    {
        auto sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);
        room->Broadcast(sendBuffer);
    }
}
