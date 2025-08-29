#include "pch.h"
#include "Arrow.h"
#include "Player.h"
#include "Monster.h"

void Arrow::Update()
{
    //if (GetData().id == 0 || GetData().projectile.name == "" || GetOwner() == nullptr || GetOwner()->GetRoom() == nullptr)
    //    return;

    //const uint64 tick = GetTickCount64();
    //if (_nextMoveTick > tick)
    //    return;

    //const uint64 moveTick = static_cast<uint64>(1000 / _statInfo.speed());
    //_nextMoveTick = tick + moveTick;

    //auto room = GetRoom();
    //if (room == nullptr)
    //    return;

    //constexpr float deltaTime = 0.1f; // Room Tick

    //if (_distance > _data.projectile.range * CELL_SIZE * 1.5f)
    //{
    //    room->AddRemoveList(shared_from_this());
    //    return;
    //}

    //Vector3 dir = Vector3::YawToDir(_posInfo.yaw());
    //Vector3 move = dir * _statInfo.speed() * deltaTime;
    //_destPos = _worldPos + move;

    //ObjectRef target = nullptr;

    //if (GetOwner()->GetCreatureType() == Protocol::CREATURE_TYPE_MONSTER)
    //{
    //    PlayerRef playerTarget = room->_playerGrid.FindNearestOnPath(_worldPos, _destPos, _radius);
    //    if (playerTarget)
    //    {
    //        target = playerTarget;
    //    }
    //}        
    //else if (GetOwner()->GetCreatureType() == Protocol::CREATURE_TYPE_PLAYER)
    //{
    //    MonsterRef monsterTarget = room->_monsterGrid.FindNearestOnPath(_worldPos, _destPos, _radius);
    //    if (monsterTarget)
    //    {
    //        target = monsterTarget;
    //    }
    //}

    //if (target)
    //{
    //    target->OnDamaged(GetOwner(), GetOwner()->_statInfo.attack() + _data.damage);
    //    room->AddRemoveList(shared_from_this());
    //    return;
    //}
    //else
    //{
    //    ;
    //}

    //if (room->GetGameMap() && !room->GetGameMap()->CanGo(WorldToGrid(_destPos)))
    //{
    //    room->AddRemoveList(shared_from_this());
    //    return;
    //}

    //_posInfo.set_x(_destPos._x);
    //_posInfo.set_y(_destPos._y);
    //_worldPos = _destPos;
    //_gridPos = Vector2Int(_posInfo);
    //_distance += move.Length();

    //S_MOVE movePkt;
    //movePkt.mutable_info()->CopyFrom(_posInfo);
    //{
    //    auto sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);
    //    room->BroadcastMove(sendBuffer);
    //}

    // ------------------------------------

    //    if (distSq * distSq <= hitDistSq)
    //    {
    //        // 명중 처리
    //        obj->OnDamaged(shared_from_this(), GetOwner()->_statInfo.attack() + GetData().damage);

    //        room->LeaveRoom(shared_from_this()); // 소멸
    //        return;
    //    }
    //}

    //// 충돌 검사 (정적 충돌만 체크 - 객체 충돌은 제외)
    //Vector2Int gridPos = Vector2Int(
    //    static_cast<int32>(round(nextPos._x / CELL_SIZE)),
    //    static_cast<int32>(round(nextPos._y / CELL_SIZE))
    //);

    //if (room->GetGameMap()->CanGo(gridPos, false)) // false = 객체 충돌은 무시
    //{
    //    // 위치 갱신
    //    _worldPos = nextPos;

    //    S_MOVE movePkt;
    //    movePkt.set_objectid(GetId());

    //    Protocol::PosInfo* pos = movePkt.mutable_pos();
    //    pos->set_x(nextPos._x);
    //    pos->set_y(nextPos._y);

    //    auto sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);
    //    room->Broadcast(sendBuffer);
    //}
    //else
    //{
    //    // 벽이나 장애물에 충돌 - 소멸
    //    room->LeaveRoom(shared_from_this());
    //}
}
