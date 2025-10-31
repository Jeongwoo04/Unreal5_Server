#pragma once
#include "ClientPacketHandler.h"
#include "ThreadManager.h"
#include "ServerSessionManager.h"

static atomic<int32> _id = 0;

class ServerSession : public PacketSession
{
public:
	ServerSession()
	{
		
	}

	~ServerSession()
	{
		cout << "~ServerSession" << endl;
	}

	virtual void OnConnected() override
	{
		GSessionManager.Add(static_pointer_cast<ServerSession>(shared_from_this()));
		//cout << "OnConnected" << endl;

		Protocol::C_ENTER_GAME pkt;
		pkt.set_playerindex(0);

		// TEMP : MultiRoom 환경 테스트
		int32 roomid = (GetNextId() / 100) + 1;
		pkt.set_roomnumber(roomid);
		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
		Send(sendBuffer);
	}

	virtual void OnRecvPacket(BYTE* buffer, int32 len) override
	{
		PacketSessionRef session = GetPacketSessionRef();
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

		// TODO : packetId 대역 체크
		ClientPacketHandler::HandlePacket(session, buffer, len);
	}

	virtual void OnSend(int32 len) override
	{
		//cout << "OnSend Len = " << len << endl;
	}

	virtual void OnDisconnected() override
	{
		GSessionManager.Remove(static_pointer_cast<ServerSession>(shared_from_this()));
		//cout << "Disconnected" << endl;
	}

	int32 GetNextId() { return _id.fetch_add(1); }

public:
	Protocol::ObjectInfo _objectInfo;
	Protocol::PosInfo _posInfo;

	bool _enter = false;
	bool _registered = false;
};