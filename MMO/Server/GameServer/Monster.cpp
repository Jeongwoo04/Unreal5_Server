#include "pch.h"
#include "Monster.h"
#include "DataManager.h"

Monster::Monster()
{
	_objectInfo.set_creature_type(Protocol::CREATURE_TYPE_MONSTER);

    // 스킬 데이터 가져오기 (ID 1 고정)
    auto skillIt = DataManager::Instance().SkillDict.find(2);
    if (skillIt == DataManager::Instance().SkillDict.end())
        return;

    skillData = skillIt->second;

    _skillRange = static_cast<float>(skillData.distance);
    _coolTick = skillData.cooldown;

    _posInfo.set_state(Protocol::STATE_MACHINE_IDLE);
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

    PlayerRef target = GetRoom()->_playerGrid.FindNearest(_gridPos, static_cast<int32>(_searchRadius), _worldPos);

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
    
    // 스킬 사정거리 도달 + 직선 추적 가능 → 스킬 전환
    if (dist <= (_skillRange * CELL_SIZE))
    {
        _coolTick = 0;
        _posInfo.set_state(Protocol::STATE_MACHINE_SKILL);
        BroadcastMove(); // 상태 변경 알림
        return;
    }

    // 직선 추적 가능 시 바로 이동
    if (map->HasLineOfSightRayCast(myPos, targetPos))
    {
        Vector3 dir = (targetPos - myPos).Normalized();
        float moveDist = _speed * _deltaTime;
        if (moveDist > dist)
            moveDist = dist;

        _worldPos += dir * moveDist;
        _posInfo.set_yaw(atan2f(dir._y, dir._x) * 180.f / PI);
        _gridPos = WorldToGrid(_worldPos);

        _posInfo.set_state(Protocol::STATE_MACHINE_MOVING);
        ApplyVectorPos();
        GetRoom()->_monsterGrid.ApplyMove(static_pointer_cast<Monster>(shared_from_this()), WorldToGrid(myPos), _gridPos);
        BroadcastMove();
        return;
    }

    // 경로 필요 시 갱신
    bool needRepath = false;

    if (_simplifiedPath.empty() || _simplifiedIndex >= _simplifiedPath.size())
        needRepath = true;
    else if ((targetPos - _lastTargetPos).Length() > 50.f * CELL_SIZE)
        needRepath = true;

    if (needRepath && tick > _nextPathUpdateTick)
    {
        _nextPathUpdateTick = tick + 1000;

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
        _lastTargetPos = targetPos;
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
            _gridPos = WorldToGrid(_worldPos);
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
            _gridPos = WorldToGrid(_worldPos);
        }

        _posInfo.set_state(Protocol::STATE_MACHINE_MOVING);
        ApplyVectorPos();
        GetRoom()->_monsterGrid.ApplyMove(static_pointer_cast<Monster>(shared_from_this()), WorldToGrid(myPos), _gridPos);
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

        /*
        // 데미지 적용
        target->OnDamaged(shared_from_this(), skillData.damage + _statInfo.attack());

        // 스킬 사용 패킷 전송
        S_SKILL skillPkt;
        skillPkt.set_object_id(GetId());
        skillPkt.mutable_skill_info()->set_skillid(skillData.id);

        if (auto room = GetRoom())
        {
            auto sendBuffer = ServerPacketHandler::MakeSendBuffer(skillPkt);
            room->Broadcast(sendBuffer);
        }
        */

        // 쿨타임 설정
        _coolTick = tick + static_cast<int64>(1000 * skillData.cooldown);
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

    cout << "Monster Hp : " << _statInfo.hp() << endl;

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

    cout << "Monster is Dead" << endl;

    auto sendBuffer = ServerPacketHandler::MakeSendBuffer(diePkt);
    room->Broadcast(sendBuffer);

    room->DoTimer(room->GetSpawnTable(_spTableId)->respawnInterval, &Room::SpawnMonster, _spTableId);
    room->AddRemoveList(shared_from_this());
}