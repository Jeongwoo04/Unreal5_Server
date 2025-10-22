#pragma once
#include "ClientPacketHandler.h"
#include "ThreadManager.h"

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
		//cout << "OnConnected" << endl;

		Protocol::C_ENTER_GAME pkt;
		pkt.set_playerindex(0);

		// TEMP : MultiRoom 환경 테스트
		int32 roomid = (GetNextId() / 1000) + 1;
		pkt.set_roomnumber(roomid);
		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
		Send(sendBuffer);
	}

	virtual void OnRecvPacket(BYTE* buffer, int32 len) override
	{
		PacketSessionRef session = GetPacketSessionRef();
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

		if (_enter == true && _registered == false)
		{
			_registered = true;

			GThreadManager->Launch([this]()
				{
					this_thread::sleep_for(chrono::seconds(30));

					while (true)
					{
						this_thread::sleep_for(chrono::milliseconds(300));
						SendRandomMove();
					}
				});
		}

		// TODO : packetId 대역 체크
		ClientPacketHandler::HandlePacket(session, buffer, len);
	}

	virtual void OnSend(int32 len) override
	{
		//cout << "OnSend Len = " << len << endl;
	}

	virtual void OnDisconnected() override
	{
		//cout << "Disconnected" << endl;
	}

	void SendRandomMove()
	{
		float dx = (rand() % 3 - 1) * 100;
		float dy = (rand() % 3 - 1) * 100;

		_posInfo.set_x(dx);
		_posInfo.set_y(dy);

		if (_posInfo.x() + dx > 40 || _posInfo.x() + dx < -40)
			_posInfo.set_x(_posInfo.x() / 2);
		if (_posInfo.y() + dy > 40 || _posInfo.y() + dy < -40)
			_posInfo.set_y(_posInfo.y() / 2);

		Protocol::C_MOVE pkt;
		{
			pkt.mutable_info()->CopyFrom(_posInfo);
		}		

		auto sendBuffer = ClientPacketHandler::MakeSendBuffer(pkt);
		Send(sendBuffer);
		//cout << "SendRandomMove !\n";
	}

	int32 GetNextId() { return _id.fetch_add(1); }

public:
	Protocol::ObjectInfo _objectInfo;
	Protocol::PosInfo _posInfo;

	bool _enter = false;
	bool _registered = false;
};