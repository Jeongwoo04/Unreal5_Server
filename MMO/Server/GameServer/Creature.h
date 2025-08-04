#pragma once
#include "Object.h"
#include "DataManager.h"

using namespace Protocol;

class Creature : public Object
{
public:
	Creature();
	virtual ~Creature();

public:

	void SetTarget(const PlayerRef& player) { _targetPlayer = player; }
	PlayerRef GetTarget() const { return _targetPlayer.lock(); }

protected:
	weak_ptr<Player> _targetPlayer;
};