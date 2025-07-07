#pragma once
#include "Creature.h"
#include "Protocol.pb.h"

class GameSession;
class Room;

class Player : public Creature
{
public:
	Player();
	virtual ~Player();

	weak_ptr<GameSession> _session;
};

