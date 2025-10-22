#pragma once

#include "DataManager.h"

class Object;
using ObjectRef = shared_ptr<class Object>;

class CombatSystem
{
public:
	static CombatSystem& Instance()
	{
		static CombatSystem instance;
		return instance;
	}

	void ApplyDamage(ObjectRef attacker, ObjectRef target, int32 damage);
};

