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
    if (GetData().id == 0 || GetOwner() == nullptr || GetOwner()->GetRoom() == nullptr)
        return;

    const uint64 tick = GetTickCount64();
    if (_nextMoveTick > tick)
        return;

    const uint64 moveTick = static_cast<uint64>(1000 / _statInfo.speed());
    _nextMoveTick = tick + moveTick;

    auto room = GetRoom();
    if (room == nullptr)
        return;

    constexpr float deltaTime = 0.1f; // Room Tick

    if (_distance > _projectileInfo.range() * CELL_SIZE * 1.5f)
    {
        room->AddRemoveList(shared_from_this());
        return;
    }

    Vector3 dir = Vector3::YawToDir2D(_posInfo.yaw());
    Vector3 move = dir * _statInfo.speed() * deltaTime;
    _destPos = _worldPos + move;

    ObjectRef target = nullptr;

    if (GetOwner()->GetCreatureType() == Protocol::CREATURE_TYPE_MONSTER)
    {
        PlayerRef playerTarget = room->_playerGrid.FindNearestOnPath(_worldPos, _destPos, _radius);
        if (playerTarget)
        {
            target = playerTarget;
        }
    }        
    else if (GetOwner()->GetCreatureType() == Protocol::CREATURE_TYPE_PLAYER)
    {
        MonsterRef monsterTarget = room->_monsterGrid.FindNearestOnPath(_worldPos, _destPos, _radius);
        if (monsterTarget)
        {
            target = monsterTarget;
        }
    }

    if (target)
    {
        //target->OnDamaged(GetOwner(), GetOwner()->_statInfo.attack() + _data.damage);
        room->AddRemoveList(shared_from_this());
        return;
    }
    else
    {
        ;
    }

    if (room->GetGameMap() && !room->GetGameMap()->CanGo(WorldToGrid(_destPos)))
    {
        room->AddRemoveList(shared_from_this());
        return;
    }

    _posInfo.set_x(_destPos._x);
    _posInfo.set_y(_destPos._y);
    _worldPos = _destPos;
    _gridPos = Vector2Int(_posInfo);
    _distance += move.Length();

    S_MOVE movePkt;
    movePkt.mutable_info()->CopyFrom(_posInfo);
    {
        auto sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);
        room->Broadcast(sendBuffer);
    }
}