#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "ServerPacketHandler.h"
//#include "Room.h"

void GameSession::OnConnected()
{
	GSessionManager.Add(static_pointer_cast<GameSession>(shared_from_this()));
}

void GameSession::OnDisconnected()
{
	GSessionManager.Remove(static_pointer_cast<GameSession>(shared_from_this()));
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
    static int packetCount = 0;
    static DWORD64 lastLog = GetTickCount64();

    packetCount++;

    DWORD64 now = GetTickCount64();
    if (now - lastLog >= 1000) // 1초가 지났으면
    {
        std::cout << "[TPS] Packets per second: " << packetCount << std::endl;
        packetCount = 0;      // 카운트 초기화
        lastLog = now;        // 마지막 로그 시간 갱신
    }


	PacketSessionRef session = GetPacketSessionRef();
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

	// TODO : packetId 대역 체크
	ServerPacketHandler::HandlePacket(session, buffer, len);
}

void GameSession::OnSend(int32 len)
{
}
