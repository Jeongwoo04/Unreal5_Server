#include "pch.h"
#include "Arrow.h"

void Arrow::Update()
{
    if (GetData().id == 0 || GetData().projectile.name == "" || GetOwner() == nullptr || GetOwner()->GetRoom() == nullptr)
        return;

    const uint64 tick = GetTickCount64();
    if (_nextMoveTick > tick)
        return;

    const float speed = GetData().projectile.speed;
    const uint64 moveTick = static_cast<uint64>(1000 / speed);
    _nextMoveTick = tick + moveTick;

    auto room = GetRoom();
    if (room == nullptr)
        return;

    // 현재 위치 및 이동 계산
    Vector3 currentPos = _worldPos;
    float yawRad = GetOwner()->_posInfo.yaw() * PI / 180.f;
    Vector3 dir = Vector3(cosf(yawRad), sinf(yawRad));
    Vector3 nextPos = currentPos + dir * GetData().projectile.speed * 0.05f;

    // 명중 거리 체크 (float 기반)
    const float kHitDistance = 50.f; // 0.5 셀 * 100 셀 크기
    float hitDistSq = kHitDistance * kHitDistance;

    //for (auto& obj : room->GetGameMap()->GetObjects())
    //{
    //    if (obj == nullptr || obj->GetType() != ObjectType::Monster)
    //        continue;

    //    Vector3 targetPos = obj->_worldPos;
    //    float distSq = (nextPos - targetPos).Length(); // float 거리 비교

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
