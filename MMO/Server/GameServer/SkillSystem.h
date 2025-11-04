#pragma once

#include "GeometryUtil.h"

struct Skill;
struct ActionData;
struct MoveActionData;
struct AttackActionData;
struct SpawnActionData;
struct BuffActionData;

struct SkillInstance
{
	ObjectRef caster;
	const Skill* skill;
	Vector3 targetPos;

	int32 castId = 0;
	bool isCasting = false;
	bool canceled = false;

	float castElapsed = 0.f;
	float actionDelayElapsed = 0.f;
	int32 currentActionIndex = 0;
};

using SkillInstanceRef = shared_ptr<struct SkillInstance>;
class Room;

class SkillSystem
{
public:
	void Init();
	void ExecuteSkill(ObjectRef caster, int32 skillId, const Vector3& targetPos, int32 castId, uint64 clientSend = 0);

	void Update(float deltaTime);
	void CancelCasting(ObjectRef caster, int32 castId);

	RoomRef GetRoom() { return _room.lock(); }
	void SetRoom(RoomRef room) { _room = room; }

private:
	void HandleAction(ObjectRef caster, const Vector3& targetPos, ActionData* action, SkillInstanceRef instance);

	void HandleMoveAction(ObjectRef caster, const Vector3& targetPos, MoveActionData* action);
	void HandleAttackAction(ObjectRef caster, const Vector3& targetPos, AttackActionData* action);
	void HandleSpawnAction(ObjectRef caster, const Vector3& targetPos, SpawnActionData* action);
	void HandleBuffAction(ObjectRef caster, const Vector3& targetPos, BuffActionData* action);

private:
	weak_ptr<Room> _room;
	const unordered_map<int32, Skill>* skillDict = nullptr;
	vector<SkillInstanceRef> activeSkills;
};

