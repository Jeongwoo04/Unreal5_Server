#pragma once

class SkillState
{
public:
	SkillState(int32 id, float cooldown) : _skillId(id), _cooldownDuration(cooldown) { }

	// 절대시간 기반 업데이트
	bool IsOnCooldown(uint64 now) const;
	bool IsCasting(uint64 now) const;

	void StartCooldown(uint64 now);
	void StartCasting(uint64 now, float castTime);
	void CancelCasting();

	float GetRemainingCooldown(uint64 now) const { return max(0.f, static_cast<float>(_cooldownEndTime - now) / 1000.f); };
	uint64 GetCastEndTime() const { return _castEndTime; }

private:
	int32 _skillId;
	float _cooldownDuration = 0.f;

	uint64 _cooldownEndTime = 0; // now 기준 절대시간
	uint64 _castEndTime = 0;
};

