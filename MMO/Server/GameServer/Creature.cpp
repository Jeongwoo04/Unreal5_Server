#include "pch.h"
#include "Creature.h"
#include "SkillState.h"

Creature::Creature()
{
	_objectInfo.set_object_type(Protocol::OBJECT_TYPE_CREATURE);
	_collisionRadius = 42.f;
}

Creature::~Creature()
{

}


bool Creature::CanUseSkill(int32 skillId, uint64 now) const
{
    auto it = _skillStates.find(skillId);
    if (it == _skillStates.end())
        return false;

    SkillStateRef state = it->second;

    // 쿨타임 / 캐스팅 중 체크
    if (state->IsOnCooldown(now) || state->IsCasting(now))
        return false;

    // 자원 체크

    return true;
}

void Creature::StartSkillCast(int32 skillId, uint64 now, float castTime)
{
    auto it = _skillStates.find(skillId);
    if (it == _skillStates.end())
        return;

    it->second->StartCasting(now, castTime);
}

void Creature::StartSkillCooldown(int32 skillId, uint64 now)
{
    auto it = _skillStates.find(skillId);
    if (it == _skillStates.end())
        return;

    it->second->StartCooldown(now);
}

void Creature::CancelActiveSkill(int32 skillId)
{
    auto it = _skillStates.find(skillId);
    if (it == _skillStates.end())
        return;

    it->second->CancelCasting();
}
