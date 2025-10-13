#include "pch.h"
#include "LoginSession.h"
#include "LoginSessionManager.h"
#include "ServerPacketHandler.h"
//#include "Room.h"

void LoginSession::OnConnected()
{
    GLoginSessionManager.Add(static_pointer_cast<LoginSession>(shared_from_this()));
}

void LoginSession::OnDisconnected()
{
    GLoginSessionManager.Remove(static_pointer_cast<LoginSession>(shared_from_this()));
}

void LoginSession::OnRecvPacket(BYTE* buffer, int32 len)
{
    PacketSessionRef session = GetPacketSessionRef();
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

    // TODO : packetId 대역 체크
    ServerPacketHandler::HandlePacket(session, buffer, len);
}

void LoginSession::OnSend(int32 len)
{
}
