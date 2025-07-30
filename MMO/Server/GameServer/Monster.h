#pragma once
#include "Creature.h"
#include "Player.h"

using RoomRef = std::shared_ptr<class Room>;
using GameMapRef = std::shared_ptr<class GameMap>;
using SkillRef = std::shared_ptr<class Skill>;

#define PI 3.1415926f

class Monster : public Creature
{
public:
	Monster();
	virtual ~Monster();

	// FSM
	virtual void Update() override;

	virtual void UpdateIdle();
	virtual void UpdatePatrol();
	virtual void UpdateMoving();
	virtual void UpdateSkill();
	virtual void UpdateDead();

	virtual void OnDamaged(ObjectRef attacker, int32 damage) override;
	virtual void OnDead(ObjectRef attacker) override;

	void BroadcastMove();

	void SetPlayer(const PlayerRef& player) { _targetPlayer = player; }
	PlayerRef GetPlayer() const { return _targetPlayer.lock(); }

public:
	vector<Vector2Int> _path;
	vector<Vector3> _simplifiedPath;
	int32 _simplifiedIndex = 0;
	Vector3 _lastTargetPos;

	float _speed = 300.0f;
	float _deltaTime = 0.1f;

protected:
	float _searchRadius = 20.f;
	float _chaseCellDist = 30.f;
	float _skillRange = 1.f;

	uint64 _nextSearchTick = 0;
	uint64 _nextMoveTick = 0;
	uint64 _coolTick = 0;
};

