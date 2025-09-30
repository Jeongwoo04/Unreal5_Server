#include "pch.h"
#include "Projectile.h"

Projectile::Projectile()
{
	_objectInfo.set_object_type(Protocol::OBJECT_TYPE_PROJECTILE);
}

Projectile::~Projectile()
{
    //cout << "Projectile " << GetId() << " Destructor" << endl;
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
    Vector2Int blocked;
    
    MoveToNextPos(destPos, &_dir, &blocked);
    room->BroadcastMove(_posInfo, GetId());

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
        {
            Protocol::HpChange change;
            change.set_object_id(target->GetId());
            change.set_hp(target->_statInfo.hp());
            Protocol::S_CHANGE_HP pkt;
            *pkt.add_changes() = change;

            auto sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);
            room->Broadcast(sendBuffer);
        }
        
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

    _moveDistance += _data->speed * deltaTime;
}
