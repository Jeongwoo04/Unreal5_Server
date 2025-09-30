#pragma once
#include "ClientPacketHandler.h"
#include "ThreadManager.h"

class ServerSession : public PacketSession
{
public:
	~ServerSession()
	{
		cout << "~ServerSession" << endl;
	}

	virtual void OnConnected() override
	{
		cout << "OnConnected" << endl;

		Protocol::C_ENTER_GAME pkt;
		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
		Send(sendBuffer);
	}

	virtual void OnRecvPacket(BYTE* buffer, int32 len) override
	{
		PacketSessionRef session = GetPacketSessionRef();
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

		if (_enter == true && _registered == false)
		{
			GThreadManager->Launch([this]()
				{
					while (true)
					{
						this_thread::sleep_for(chrono::milliseconds(300));
						SendRandomMove();
					}
				});

			_registered = true;
		}

		// TODO : packetId 대역 체크
		ClientPacketHandler::HandlePacket(session, buffer, len);
	}

	virtual void OnSend(int32 len) override
	{
		cout << "OnSend Len = " << len << endl;
	}

	virtual void OnDisconnected() override
	{
		cout << "Disconnected" << endl;
	}

	void SendRandomMove()
	{

		int32 dx = (rand() % 3 - 1) * 100;
		int32 dy = (rand() % 3 - 1) * 100;

		Protocol::C_MOVE pkt;
		_posInfo.set_x(dx);
		_posInfo.set_y(dy);
		pkt.mutable_info()->CopyFrom(_posInfo);

		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
		Send(sendBuffer);
	}

public:
	Protocol::ObjectInfo _objectInfo;
	Protocol::PosInfo _posInfo;

	bool _enter = false;
	bool _registered = false;
};