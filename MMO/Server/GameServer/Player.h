#pragma once
#include "Object.h"

class GameSession;
class Room;

class Player : public Object
{
public:
	Player();
	virtual ~Player();

public:
	Protocol::PlayerInfo info;
	weak_ptr<GameSession> _session;

public:
	weak_ptr<Room> _room; // 스마트포인터는 set 할때 멀티스레드에서 위험
};

