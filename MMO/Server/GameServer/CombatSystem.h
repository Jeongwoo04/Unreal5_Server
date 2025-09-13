#pragma once

#include "DataManager.h"

class Room;
class Player;

class CombatSystem
{
public:
	static CombatSystem& Instance()
	{
		static CombatSystem instance;
		return instance;
	}

	void ApplyDamage(ObjectRef attacker, ObjectRef target, int32 damage)
	{
		target->OnDamaged(attacker, damage);
	}
};

