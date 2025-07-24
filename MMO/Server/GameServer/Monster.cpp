#include "pch.h"
#include "Monster.h"

Monster::Monster()
{
	_objectInfo.set_creature_type(Protocol::CREATURE_TYPE_MONSTER);

	// TODO : Stat
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
    _nextSearchTick = tick + 1000;

    PlayerRef target = GetRoom()->FindPlayer([&](const ObjectRef& p)
        {
            Vector2Int dir = p->GetCellPos() - GetCellPos();
            return dir.cellDist() <= _searchCellDist;
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

    //const int32 moveTick = static_cast<int32>(1000 / _statInfo()->speed());
    const int32 moveTick = 500;
    _nextMoveTick = tick + moveTick;

    PlayerRef target = GetPlayer();
    if (target == nullptr || target->GetRoom() != GetRoom())
    {
        SetPlayer(nullptr);
        _posInfo.set_state(Protocol::STATE_MACHINE_IDLE);
        BroadcastMove();
        return;
    }

    Vector2Int dir = target->GetCellPos() - GetCellPos();
    int32 dist = dir.cellDist();
    if (dist == 0 || dist > _chaseCellDist)
    {
        SetPlayer(nullptr);
        _posInfo.set_state(Protocol::STATE_MACHINE_IDLE);
        BroadcastMove();
        return;
    }

    vector<Vector2Int> path = GetRoom()->GetGameMap()->FindPath(GetCellPos(), target->GetCellPos(), /*checkObjects=*/false);
    if (path.size() < 2 || path.size() > _chaseCellDist)
    {
        SetPlayer(nullptr);
        _posInfo.set_state(Protocol::STATE_MACHINE_IDLE);
        BroadcastMove();
        return;
    }

    // Skill 가능 여부 체크
    if (dist <= _skillRange && (dir._x == 0 || dir._y == 0))
    {
        _coolTick = 0;
        _posInfo.set_state(Protocol::STATE_MACHINE_SKILL);
        return;
    }

    // 이동
    _gridPos = path[1];
    GetRoom()->GetGameMap()->ApplyMove(shared_from_this(), path[1]);

    // 다른 플레이어에 알림
    GridToWorld(_gridPos);
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
        //if (target == nullptr || target->GetRoom() != GetRoom() || target->_statInfo()->hp() == 0)
        if (target == nullptr || target->GetRoom() != GetRoom())
        {
            SetPlayer(nullptr);
            _posInfo.set_state(Protocol::STATE_MACHINE_IDLE);
            BroadcastMove();
            return;
        }

        // 스킬 사용 가능한지
        Vector2Int dir = (target->GetCellPos() - GetCellPos());
        int32 dist = dir.cellDist();
        bool canUseSkill = (dist <= _skillRange && (dir._x == 0 || dir._y == 0));
        if (canUseSkill == false)
        {
            SetPlayer(nullptr);
            _posInfo.set_state(Protocol::STATE_MACHINE_IDLE);
            BroadcastMove();
            return;
        }

        // 타겟 방향 주시
        //MoveDir lookDir = GetDirFromVec(dir);
        //if (_posInfo()->movedir() != lookDir)
        //{
        //    _posInfo()->set_movedir(lookDir);
        //    BroadcastMove();
        //}

        //SkillRef skillData = nullptr;
        //auto it = DataManager::Instance().SkillDict.find(1);
        //if (it == DataManager::Instance().SkillDict.end())
        //    return;
        //skillData = it->second;

        //// 데미지 판정
        //target->OnDamaged(shared_from_this(), skillData->damage + _statInfo()->attack());

        //// 스킬 사용 Broadcast
        //S_Skill skillPkt;
        //skillPkt.set_objectid(GetId());
        //skillPkt.mutable_info()->set_skillid(skillData->id);

        //if (auto room = GetRoom())
        //{
        //    auto sendBuffer = ServerPacketHandler::MakeSendBuffer(skillPkt);
        //    room->Broadcast(sendBuffer);
        //}

        // 스킬 쿨타임 적용
        //_coolTick = tick + static_cast<int64>(1000 * skillData->cooldown);
        _coolTick = tick + 1000;
    }

    if (_coolTick > tick)
        return;

    _coolTick = 0;
}

void Monster::UpdateDead()
{

}

void Monster::OnDamaged(ObjectRef attacker, int32 damage)
{

}

void Monster::OnDead(ObjectRef attacker)
{
    auto room = GetRoom();
    if (room == nullptr)
        return;

    //S_Die diePkt;
    //diePkt.set_objectid(GetId());
    //diePkt.set_attackerid(attacker->GetId());

    //auto sendBuffer = ServerPacketHandler::MakeSendBuffer(diePkt);
    //room->Broadcast(sendBuffer);

    //room->LeaveGame(GetId());

    //_statInfo()->set_hp(_statInfo()->maxhp());
    //_posInfo.set_state(Protocol::STATE_MACHINE_IDLE);
    //_posInfo.set_posx(5);
    //_posInfo.set_posy(5);

    //room->DoAsync(&Room::EnterGame, shared_from_this());
}