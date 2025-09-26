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
    cout << "Player " << this->GetId() << " OnDamaged by Monster " << attacker->GetId() << " damage : " << damage + attacker->_statInfo.attack() << endl;
}

void Player::OnDead(ObjectRef attacker)
{
    auto room = GetRoom();
    if (room == nullptr)
        return;

    if (attacker->GetCreatureType() == CREATURE_TYPE_MONSTER)
        static_pointer_cast<Monster>(attacker)->SetPlayer(nullptr);

    S_DIE diePkt;
    diePkt.set_object_id(GetId());
    diePkt.set_attacker_id(attacker->GetId());

    auto sendBuffer = ServerPacketHandler::MakeSendBuffer(diePkt);
    room->Broadcast(sendBuffer);

    room->LeaveRoom(shared_from_this());
}