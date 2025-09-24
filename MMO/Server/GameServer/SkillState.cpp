#include "pch.h"
#include "SkillState.h"

bool SkillState::IsOnCooldown(uint64 now) const
{
	return now < _cooldownEndTime;
}

bool SkillState::IsCasting(uint64 now) const
{
	return now < _castEndTime;
}

void SkillState::StartCooldown(uint64 now)
{
	_cooldownEndTime = now + static_cast<uint64>(_cooldownDuration * 1000);
	cout << "쿨타임 적용 : " << _cooldownEndTime - now << " 초 후 완료" << endl;
}

void SkillState::StartCasting(uint64 now, float castTime)
{
	_castEndTime = now + static_cast<uint64>(castTime * 1000);
	cout << "캐스팅 시작 : " << _castEndTime - now << " 초 후 완료" << endl;
}

void SkillState::CancelCasting()
{
	_castEndTime = 0;
}
