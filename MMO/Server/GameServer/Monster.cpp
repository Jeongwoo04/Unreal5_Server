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

    _skillData = &(skillIt->second);
}

Monster::~Monster()
{

}

void Monster::Update(float deltaTime)
{
    switch (_posInfo.state())
    {
    case StateMachine::STATE_MACHINE_IDLE:
        UpdateIdle(deltaTime);
        break;
    case StateMachine::STATE_MACHINE_PATROL:
        UpdatePatrol(deltaTime);
        break;
    case StateMachine::STATE_MACHINE_MOVING:
        UpdateMoving(deltaTime);
        break;
    case StateMachine::STATE_MACHINE_SKILL:
        UpdateSkill(deltaTime);
        break;
    case StateMachine::STATE_MACHINE_DEAD:
        UpdateDead(deltaTime);
        break;
    }
}

void Monster::UpdateIdle(float deltaTime)
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
    BroadcastMove();
}

void Monster::UpdatePatrol(float deltaTime)
{

}

void Monster::UpdateMoving(float deltaTime)
{
    const uint64 tick = GetTickCount64();
    
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
    Vector3 destPos;
    float distance = (targetPos - myPos).Length2D();

    // 사정거리 초과 시 추적 종료
    if (distance > _searchRadius * 1.5 * CELL_SIZE)
    {
        SetPlayer(nullptr);
        _posInfo.set_state(Protocol::STATE_MACHINE_IDLE);
        BroadcastMove();
        return;
    }
    
    // 스킬 사정거리 도달 + 직선 추적 가능 → 스킬 전환
    if (distance <= (_skillData->actions[0]->distance * CELL_SIZE))
    {
        _coolTick = 0;
        _posInfo.set_state(Protocol::STATE_MACHINE_SKILL);
        BroadcastMove(); // 상태 변경 알림
        return;
    }

    // 직선 추적 가능 시 바로 이동
    if (map->HasLineOfSightRayCast(myPos, targetPos))
    {
        Vector3 dir = (targetPos - myPos).Normalized2D();
        float moveDist = _statInfo.speed() * deltaTime;
        if (moveDist > distance)
            moveDist = distance;

        destPos = dir * moveDist;
        MoveToNextPos(destPos);

        _posInfo.set_yaw(atan2f(dir._y, dir._x) * 180.f / PI);
        _posInfo.set_state(Protocol::STATE_MACHINE_MOVING);
        BroadcastMove();
        return;
    }

    // 경로 필요 시 갱신
    bool needRepath = false;

    if (_simplifiedPath.empty() || _simplifiedIndex >= _simplifiedPath.size())
        needRepath = true;
    else if ((targetPos - _lastTargetPos).Length2D() > 5.f * CELL_SIZE)
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
        destPos = _simplifiedPath[_simplifiedIndex];
        _dir = destPos - _worldPos;
        float segDist = _dir.Length2D();

        _posInfo.set_state(Protocol::STATE_MACHINE_MOVING);
        if (segDist < 0.01f)
        {
            MoveToNextPos(destPos);
            _simplifiedIndex++;
        }
        else
        {
            _dir = _dir / segDist;
            float moveDist = _statInfo.speed() * deltaTime;
            if (moveDist > segDist)
                moveDist = segDist;

            Vector3 destPos = _worldPos + _dir * moveDist;
            MoveToNextPos(destPos);
            _posInfo.set_yaw(atan2f(_dir._y, _dir._x) * 180.f / PI);
        }

        BroadcastMove();
        return;
    }

    // 그 외: Idle 전환
    _posInfo.set_state(Protocol::STATE_MACHINE_IDLE);
    BroadcastMove();
}

void Monster::UpdateSkill(float deltaTime)
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
        float dist = (target->_worldPos - _worldPos).Length2D();
        if (dist > _skillData->actions[0]->distance * CELL_SIZE)
        {
            SetPlayer(nullptr);
            _posInfo.set_state(Protocol::STATE_MACHINE_IDLE);
            BroadcastMove(); // 상태 전환 알림
            return;
        }

        Vector3 dir = (target->_worldPos - _worldPos).Normalized2D();
        float yaw = atan2f(dir._y, dir._x) * 180.f / PI;
        _posInfo.set_yaw(yaw);
        BroadcastMove();

        // 데미지 적용
        //

        // 스킬 사용 패킷 전송
        S_SKILL skillPkt;
        skillPkt.set_object_id(GetId());
        skillPkt.mutable_skill_info()->set_skillid(_skillData->id);

        if (auto room = GetRoom())
        {
            auto sendBuffer = ServerPacketHandler::MakeSendBuffer(skillPkt);
            room->Broadcast(sendBuffer);
        }

        // 쿨타임 설정
        _coolTick = tick + static_cast<int64>(1000 * _skillData->cooldown);
    }
    else
    {
        // 쿨타임 끝
        _coolTick = 0;
    }
}

void Monster::UpdateDead(float deltaTime)
{

}

void Monster::OnDamaged(ObjectRef attacker, int32 damage)
{
    Object::OnDamaged(attacker, damage);
    cout << "Monster " << this->GetId() << " OnDamaged by Player " << attacker->GetId() << " damage : " << damage + attacker->_statInfo.attack();
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

void Monster::BroadcastMove()
{
    auto room = GetRoom();
    if (room == nullptr)
        return;

    room->BroadcastMove(_posInfo, GetId());
}
