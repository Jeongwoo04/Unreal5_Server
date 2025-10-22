#include "pch.h"
#include "CombatSystem.h"

void CombatSystem::ApplyDamage(ObjectRef attacker, ObjectRef target, int32 damage)
{
	// TEMP
	target->OnDamaged(attacker, damage);
}
