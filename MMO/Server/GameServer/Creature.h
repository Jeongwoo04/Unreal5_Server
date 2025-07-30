#pragma once
#include "Object.h"

using namespace Protocol;

class Creature : public Object
{
public:
	Creature();
	virtual ~Creature();

public:

	void SetTarget(const PlayerRef& player) { _targetPlayer = player; }
	PlayerRef GetTarget() const { return _targetPlayer.lock(); }

public:
	StatInfo _statInfo;

protected:
	weak_ptr<Player> _targetPlayer;
};