#pragma once
#include "Object.h"
#include "DataManager.h"

using namespace Protocol;

class SkillState;
struct SkillInstance;
using SkillStateRef = shared_ptr<class SkillState>;
using SkillInstanceRef = shared_ptr<struct SkillInstance>;

class Creature : public Object
{
public:
	Creature();
	virtual ~Creature();

	virtual bool CanUseSkill(int32 skillId, uint64 now) const;
	virtual void StartSkillCast(int32 skillId, uint64 now, float castTime);
	virtual void StartSkillCooldown(int32 skillId, uint64 now);
	virtual void CancelActiveSkill(int32 skillId);

	SkillStateRef GetSkillState(int32 skillId) { return _skillStates[skillId]; }

	void SetActiveSkill(SkillInstanceRef activeSkill) { _activeSkill = activeSkill; }
	SkillInstanceRef GetActiveSkill() { return _activeSkill.lock(); }

protected:
	unordered_map<int32, SkillStateRef> _skillStates;
	weak_ptr<SkillInstance> _activeSkill;
};