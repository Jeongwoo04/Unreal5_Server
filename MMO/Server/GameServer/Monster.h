#pragma once
#include "Creature.h"
#include "Player.h"

using RoomRef = std::shared_ptr<class Room>;
using GameMapRef = std::shared_ptr<class GameMap>;

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

	vector<Vector3> SmoothPath(const std::vector<Vector3>& originalPath);

public:
	vector<Vector2Int> _path;
	int32 _pathIndex = 1;
	float _speed = 300.0f;
	float _deltaTime = 0.05f;
	float _reachThreshold = 50.f;

protected:
	float _searchRadius = 12.f;
	float _chaseCellDist = 20.f;
	float _skillRange = 5.f;

	uint64 _nextSearchTick = 0;
	uint64 _nextMoveTick = 0;
	uint64 _coolTick = 0;
};

