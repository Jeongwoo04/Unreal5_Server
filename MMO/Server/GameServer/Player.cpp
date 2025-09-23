#include "pch.h"
#include "Player.h"
#include "SkillState.h"

Player::Player()
{
	_objectInfo.set_creature_type(Protocol::CREATURE_TYPE_PLAYER);

    for (auto& it : DataManager::Instance().SkillDict)
    {
        int32 id = it.first;
        const Skill& s = it.second;
        _skillStates.emplace(id, make_shared<SkillState>(id, s.cooldown));
    }
}

Player::~Player()
{

}

void Player::OnDamaged(ObjectRef attacker, int32 damage)
{
    Object::OnDamaged(attacker, damage);
    cout << "Player " << this->GetId() << " OnDamaged by Monster " << attacker->GetId() << " damage : " << damage + attacker->_statInfo.attack();
}

void Player::OnDead(ObjectRef attacker)
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

bool Player::CanUseSkill(int32 skillId, uint64 now) const
{
    auto it = _skillStates.find(skillId);
    if (it == _skillStates.end())
        return false;

    SkillStateRef state = it->second;

    // 쿨타임 / 캐스팅 중 체크
    if (state->IsOnCooldown(now) || state->IsCasting(now))
        return false;

    // 자원 체크

    return true;
}

void Player::StartSkillCast(int32 skillId, uint64 now, float castTime)
{
    auto it = _skillStates.find(skillId);
    if (it == _skillStates.end())
        return;

    it->second->StartCasting(now, castTime);
}

void Player::StartSkillCooldown(int32 skillId, uint64 now)
{
    auto it = _skillStates.find(skillId);
    if (it == _skillStates.end())
        return;

    it->second->StartCooldown(now);
}