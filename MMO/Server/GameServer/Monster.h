#pragma once
#include "Creature.h"
#include "Player.h"

using RoomRef = shared_ptr<class Room>;
using GameMapRef = shared_ptr<class GameMap>;

class Monster : public Creature
{
public:
	Monster();
	virtual ~Monster();

	// FSM
	virtual void Update(float deltaTime) override;

	virtual void UpdateIdle(float deltaTime);
	virtual void UpdatePatrol(float deltaTime);
	virtual void UpdateMoving(float deltaTime);
	virtual void UpdateSkill(float deltaTime);
	virtual void UpdateDead(float deltaTime);

	virtual void OnDamaged(ObjectRef attacker, int32 damage) override;
	virtual void OnDead(ObjectRef attacker) override;

	void BroadcastMove();

	void SetPlayer(const PlayerRef& player) { _targetPlayer = player; }
	PlayerRef GetPlayer() const { return _targetPlayer.lock(); }

public:
	vector<Vector2Int> _path;
	vector<Vector3> _simplifiedPath;
	int32 _simplifiedIndex = 0;

protected:
	float _searchRadius = 15.f;

	const Skill* _skillData;

	uint64 _nextPathUpdateTick = 0;
	uint64 _nextSearchTick = 0;
	uint64 _coolTick = 0;

	Vector3 _lastTargetPos = Vector3(-99999, -99999, -99999);

	weak_ptr<Player> _targetPlayer;
};

