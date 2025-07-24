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

protected:
	int32 _searchCellDist = 10;
	int32 _chaseCellDist = 20;
	int32 _skillRange = 1;

	uint64 _nextSearchTick = 0;
	uint64 _nextMoveTick = 0;
	uint64 _coolTick = 0;
};

