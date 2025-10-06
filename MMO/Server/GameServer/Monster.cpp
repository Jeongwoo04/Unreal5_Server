#include "pch.h"
#include "Monster.h"
#include "DataManager.h"
#include "SkillState.h"
#include "SkillSystem.h"

Monster::Monster()
{
	_objectInfo.set_creature_type(Protocol::CREATURE_TYPE_MONSTER);

    // 스킬 데이터 가져오기 (ID 1 고정)
    for (auto& it : DataManager::Instance().SkillDict)
    {
        int32 id = it.first;
        //
        //if (id == 4)
        //    continue;
        //
        const Skill& s = it.second;
        _skillStates.emplace(id, make_shared<SkillState>(id, s.cooldown + 3.f));
    }
}

Monster::~Monster()
{
    //cout << "Monster " << GetId() << " Destructor" << endl;
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
    case StateMachine::STATE_MACHINE_CASTING:
        UpdateCasting(deltaTime);
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

    auto target = GetPlayer();
    if (target == nullptr || target->IsDead())
    {
        if (_nextSearchTick > tick)
            return;
        _nextSearchTick = tick + 500;

        target = GetRoom()->_playerGrid.FindNearest(_gridPos, static_cast<int32>(_searchRadius), _worldPos);

        if (target == nullptr || target->IsDead())
            return;

        SetPlayer(target);
        ChangeState(Protocol::STATE_MACHINE_MOVING);
        BroadcastMove();
        return;
    }

    if (_coolTick <= tick)
    {
        SelectSkill();
        if (_currentSkillId > 0)
        {
            DoSkill();
            return;
        }
        else
        {
            Vector3 dir = (target->_worldPos - _worldPos);
            if (dir.LengthSquared2D() < ((_chaseDistance * CELL_SIZE) * (_chaseDistance * CELL_SIZE)))
            {
                _posInfo.set_yaw(Vector3::DirToYaw2D(dir.Normalized2D()));
                ChangeState(Protocol::STATE_MACHINE_MOVING);
                BroadcastMove();
                return;
            }
        }
    }
}

void Monster::UpdatePatrol(float deltaTime)
{

}

void Monster::UpdateCasting(float deltaTime)
{

}

void Monster::UpdateMoving(float deltaTime)
{
    const uint64 tick = GetTickCount64();
    
    PlayerRef target = GetPlayer();
    if (target == nullptr || target->IsDead() || target->GetRoom() != GetRoom())
    {
        SetPlayer(nullptr);
        ChangeState(Protocol::STATE_MACHINE_IDLE);
        BroadcastMove();
        return;
    }

    auto room = GetRoom();
    if (room == nullptr)
        return;
    
    GameMapRef map = GetRoom()->GetGameMap();
    Vector3 myPos = _worldPos;
    Vector3 targetPos = target->_worldPos;
    Vector3 destPos;
    float distanceSq = (targetPos - myPos).LengthSquared2D();

    // 사정거리 초과 시 추적 종료
    if (distanceSq > ((_chaseDistance * CELL_SIZE) * (_chaseDistance * CELL_SIZE)))
    {
        SetPlayer(nullptr);
        ChangeState(Protocol::STATE_MACHINE_IDLE);
        BroadcastMove();
        return;
    }
    
    if (_coolTick <= tick)
    {
        SelectSkill();

        if (_currentSkillId > 0) // 스킬을 골랐다면
        {
            auto it = DataManager::Instance().SkillDict.find(_currentSkillId);
            if (it != DataManager::Instance().SkillDict.end())
            {
                const Skill& skillData = it->second;
                float skillDist = skillData.actions[0]->distance * CELL_SIZE;
                // 스킬 사정거리 도달 + 직선 추적 가능 → 스킬 전환
                if (distanceSq <= (skillDist * skillDist))
                {
                    DoSkill();
                    ChangeState(Protocol::STATE_MACHINE_SKILL);
                    return;
                }
            }
        }
    }

    // 직선 추적 가능 시 바로 이동
    if (map->HasLineOfSightRayCast(myPos, targetPos))
    {
        Vector3 dir = (targetPos - myPos).Normalized2D();
        float moveDist = _statInfo.speed() * deltaTime;
        if ((moveDist * moveDist) > distanceSq)
            moveDist = (targetPos - myPos).Length2D();

        destPos = _worldPos + dir * moveDist;

        MoveToNextPos(destPos, &dir);
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
            ChangeState(Protocol::STATE_MACHINE_IDLE);
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
        Vector3 dir = destPos - _worldPos;
        float segDistSq = dir.LengthSquared2D();
        float moveDist = _statInfo.speed() * deltaTime;

        if (moveDist * moveDist >= segDistSq)
        {
            MoveToNextPos(destPos, &dir);
            _simplifiedIndex++;
        }
        else
        {
            if (segDistSq > 0.f)
                dir = dir / sqrt(segDistSq);
            else
                return;

            destPos = _worldPos + dir * moveDist;
            MoveToNextPos(destPos, &dir);
        }
        BroadcastMove();
        return;
    }

    // 그 외: Idle 전환
    ChangeState(Protocol::STATE_MACHINE_IDLE);
}

void Monster::UpdateSkill(float deltaTime)
{

}

void Monster::UpdateDead(float deltaTime)
{

}

void Monster::OnDamaged(ObjectRef attacker, int32 damage)
{
    Object::OnDamaged(attacker, damage);
}

void Monster::OnDead(ObjectRef attacker)
{
    auto room = GetRoom();
    if (room == nullptr)
        return;

    //_selectedSkill = nullptr;
    room->_skillSystem->CancelCasting(shared_from_this(), _castId);

    S_DIE diePkt;
    diePkt.set_object_id(GetId());
    diePkt.set_attacker_id(attacker->GetId());

    //cout << "Monster is Dead" << endl;

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

void Monster::SelectSkill()
{
    uint64 now = ::GetTickCount64();

    vector<int32> availableSkills;
    for (auto& it : _skillStates)
    {
        if (this->CanUseSkill(it.first, now))
            availableSkills.push_back(it.first);
    }
    if (availableSkills.empty())
        return;

    int32 idx = Utils::GetRandom(0, static_cast<int32>(availableSkills.size() - 1));
    _currentSkillId = availableSkills[idx];
}

void Monster::DoSkill()
{
    PlayerRef target = GetPlayer();
    if (target == nullptr || target->IsDead())
        return;

    Vector3 dir = target->_worldPos - _worldPos;
    _posInfo.set_yaw(Vector3::DirToYaw2D(dir));
    _castId++;

    if (auto room = GetRoom())
    {
        BroadcastMove();
        room->_skillSystem->ExecuteSkill(shared_from_this(), _currentSkillId, target->_worldPos, _castId);
        _coolTick = ::GetTickCount64() + 3000;
    }
}

bool Monster::CanUseSkill(int32 skillId, uint64 now) const
{
    auto it = _skillStates.find(skillId);
    if (it == _skillStates.end())
        return false;

    SkillStateRef state = it->second;

    // 쿨타임 / 캐스팅 중 체크
    if (state->IsOnCooldown(now) || state->IsCasting(now))
        return false;

    auto skillIt = DataManager::Instance().SkillDict.find(skillId);
    const Skill* skill = &skillIt->second;

    PlayerRef target = GetPlayer();
    if (target == nullptr || target->IsDead())
        return false;

    // 사거리 및 시야 체크
    if ((target->_worldPos - _worldPos).Length2D() > skill->actions[0]->distance * CELL_SIZE)
        return false;

    auto map = GetRoom() ? GetRoom()->GetGameMap() : nullptr;
    if (map && !map->HasLineOfSightRayCast(_worldPos, target->_worldPos))
        return false;

    // 자원 체크

    return true;
}