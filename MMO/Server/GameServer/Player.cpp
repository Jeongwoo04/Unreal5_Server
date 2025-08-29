#include "pch.h"
#include "Player.h"

Player::Player()
{
	_objectInfo.set_creature_type(Protocol::CREATURE_TYPE_PLAYER);

	_posInfo.set_state(Protocol::STATE_MACHINE_IDLE);
}

Player::~Player()
{

}

void Player::OnDamaged(ObjectRef attacker, int32 damage)
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