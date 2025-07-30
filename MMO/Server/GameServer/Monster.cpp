#include "pch.h"
#include "Monster.h"
#include "DataManager.h"

Monster::Monster()
{
	_objectInfo.set_creature_type(Protocol::CREATURE_TYPE_MONSTER);

	// TODO : Stat
	_posInfo.set_state(Protocol::STATE_MACHINE_IDLE);
    _posInfo.set_speed(_speed);
}

Monster::~Monster()
{

}

void Monster::Update()
{
    switch (_posInfo.state())
    {
    case StateMachine::STATE_MACHINE_IDLE:
        UpdateIdle();
        break;
    case StateMachine::STATE_MACHINE_PATROL:
        UpdatePatrol();
        break;
    case StateMachine::STATE_MACHINE_MOVING:
        UpdateMoving();
        break;
    case StateMachine::STATE_MACHINE_SKILL:
        UpdateSkill();
        break;
    case StateMachine::STATE_MACHINE_DEAD:
        UpdateDead();
        break;
    }
}

void Monster::UpdateIdle()
{
    const uint64 tick = GetTickCount64();
    if (_nextSearchTick > tick)
        return;
    _nextSearchTick = tick + 500;

    PlayerRef target = GetRoom()->FindPlayer([&](const ObjectRef& p)
        {
            float dist = (Vector3(_posInfo) - Vector3(p->_posInfo)).Length();
            return dist <= _searchRadius * CELL_SIZE;
        });

    if (target == nullptr)
        return;

    SetPlayer(target);
    _posInfo.set_state(Protocol::STATE_MACHINE_MOVING);
}

void Monster::UpdatePatrol()
{

}

void Monster::UpdateMoving()
{
    const uint64 tick = GetTickCount64();
    if (_nextMoveTick > tick)
        return;

    //const int32 moveTick = static_cast<int32>(1000 / _statInfo.speed());
    const int32 moveTick = 50;
    _nextMoveTick = tick + moveTick;

    PlayerRef target = GetPlayer();
    if (target == nullptr || target->GetRoom() != GetRoom())
    {
        SetPlayer(nullptr);
        _posInfo.set_state(Protocol::STATE_MACHINE_IDLE);
        BroadcastMove();
        return;
    }

    GameMapRef map = GetRoom()->GetGameMap();
    Vector3 myPos = _worldPos;
    Vector3 targetPos = target->_worldPos;
    float dist = (targetPos - myPos).Length();

    // 사정거리 초과 시 추적 종료
    if (dist > _chaseCellDist * CELL_SIZE)
    {
        SetPlayer(nullptr);
        _posInfo.set_state(Protocol::STATE_MACHINE_IDLE);
        BroadcastMove();
        return;
    }
    
    //// 스킬 사정거리 도달 + 직선 추적 가능 → 스킬 전환
    //if (dist <= static_cast<float>(_skillRange * CELL_SIZE) && map->HasLineOfSightRayCast(myPos, targetPos))
    //{
    //    _coolTick = 0;
    //    _posInfo.set_state(Protocol::STATE_MACHINE_SKILL);
    //    BroadcastMove(); // 상태 변경 알림
    //    return;
    //}

    // 직선 추적 가능 시 바로 이동
    if (map->HasLineOfSightRayCast(myPos, targetPos))
    {
        Vector3 dir = (targetPos - myPos).Normalized();
        float moveDist = _speed * _deltaTime;
        if (moveDist > dist)
            moveDist = dist;

        _worldPos += dir * moveDist;
        _posInfo.set_yaw(atan2f(dir._y, dir._x) * 180.f / PI);
        _gridPos = GameMap::WorldToGrid(_worldPos);

        _posInfo.set_state(Protocol::STATE_MACHINE_MOVING);
        ApplyPos();
        map->ApplyMove(shared_from_this(), _gridPos);
        BroadcastMove();
        return;
    }

    // 경로 필요 시 갱신
    static Vector3 lastTargetPos = Vector3(-99999, -99999);
    bool needRepath = false;

    if (_simplifiedPath.empty() || _simplifiedIndex >= _simplifiedPath.size())
        needRepath = true;
    else if ((targetPos - lastTargetPos).Length() > 10.f * CELL_SIZE)
        needRepath = true;

    if (needRepath)
    {
        _path = map->FindPath(_gridPos, target->_gridPos, false);
        if (_path.size() < 2)
        {
            SetPlayer(nullptr);
            _posInfo.set_state(Protocol::STATE_MACHINE_IDLE);
            BroadcastMove();
            return;
        }

        _simplifiedPath = map->SimplifyPathRaycast(_worldPos, _path);
        _simplifiedIndex = 1;
        lastTargetPos = targetPos;
    }

    // 간소화 경로 따라 이동
    if (_simplifiedIndex < _simplifiedPath.size())
    {
        Vector3 next = _simplifiedPath[_simplifiedIndex];
        Vector3 dir = next - _worldPos;
        float segDist = dir.Length();

        if (segDist < 0.01f)
        {
            _worldPos = next;
            _gridPos = GameMap::WorldToGrid(_worldPos);
            _simplifiedIndex++;
        }
        else
        {
            dir = dir / segDist;
            float moveDist = _speed * _deltaTime;
            if (moveDist > segDist)
                moveDist = segDist;

            _worldPos += dir * moveDist;
            _posInfo.set_yaw(atan2f(dir._y, dir._x) * 180.f / PI);
            _gridPos = GameMap::WorldToGrid(_worldPos);
        }

        _posInfo.set_state(Protocol::STATE_MACHINE_MOVING);
        ApplyPos();
        map->ApplyMove(shared_from_this(), _gridPos);
        BroadcastMove();
        return;
    }

    // 그 외: Idle 전환
    _posInfo.set_state(Protocol::STATE_MACHINE_IDLE);
    BroadcastMove();
}

void Monster::BroadcastMove()
{
    S_MOVE movePkt;
    auto info = movePkt.mutable_info();

    info->CopyFrom(_posInfo);

    if (auto room = GetRoom())
    {
        auto sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);
        room->Broadcast(sendBuffer, GetId());
    }
}

void Monster::UpdateSkill()
{
    const uint64 tick = GetTickCount64();
    if (_coolTick == 0)
    {
        // 유효한 타겟 체크
        PlayerRef target = GetPlayer();
        if (target == nullptr || target->GetRoom() != GetRoom() || target->_statInfo.hp() == 0)
        {
            SetPlayer(nullptr);
            _posInfo.set_state(Protocol::STATE_MACHINE_IDLE);
            BroadcastMove();
            return;
        }

        // 타겟과 거리 확인
        float dist = (target->_worldPos - _worldPos).Length();
        if (dist > _skillRange * CELL_SIZE)
        {
            SetPlayer(nullptr);
            _posInfo.set_state(Protocol::STATE_MACHINE_IDLE);
            BroadcastMove(); // 상태 전환 알림
            return;
        }

        Vector3 dir = (target->_worldPos - _worldPos).Normalized();
        float yaw = atan2f(dir._y, dir._x) * 180.f / PI;
        _posInfo.set_yaw(yaw);
        BroadcastMove();

        // 스킬 데이터 가져오기 (ID 1 고정)
        SkillRef skillData = nullptr;
        auto it = DataManager::Instance().SkillDict.find(1);
        if (it == DataManager::Instance().SkillDict.end())
            return;

        skillData = it->second;

        // 데미지 적용
        target->OnDamaged(shared_from_this(), skillData->damage + _statInfo.attack());

        // 스킬 사용 패킷 전송
        S_SKILL skillPkt;
        skillPkt.set_object_id(GetId());
        skillPkt.mutable_skill_info()->set_skillid(skillData->id);

        if (auto room = GetRoom())
        {
            auto sendBuffer = ServerPacketHandler::MakeSendBuffer(skillPkt);
            room->Broadcast(sendBuffer);
        }

        // 쿨타임 설정
        _coolTick = tick + static_cast<int64>(1000 * skillData->cooldown);
    }
    else
    {
        // 쿨타임 끝
        _coolTick = 0;
    }
}

void Monster::UpdateDead()
{

}

void Monster::OnDamaged(ObjectRef attacker, int32 damage)
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

    S_CHANGE_HP changeHpPkt;
    changeHpPkt.set_object_id(GetId());
    changeHpPkt.set_hp(_statInfo.hp());

    auto sendBuffer = ServerPacketHandler::MakeSendBuffer(changeHpPkt);
    room->Broadcast(sendBuffer);
}

void Monster::OnDead(ObjectRef attacker)
{
    auto room = GetRoom();
    if (room == nullptr)
        return;

    S_DIE diePkt;
    diePkt.set_object_id(GetId());
    diePkt.set_attacker_id(attacker->GetId());

    auto sendBuffer = ServerPacketHandler::MakeSendBuffer(diePkt);
    room->Broadcast(sendBuffer);

    room->LeaveRoom(shared_from_this());
}