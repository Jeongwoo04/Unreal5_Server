#pragma once
#include "Creature.h"
#include "Player.h"
#include "Room.h"

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

	virtual bool DoPatrol();

	virtual void OnDamaged(ObjectRef attacker, int32 damage) override;
	virtual void OnDead(ObjectRef attacker) override;

	void BroadcastMoveUpdate();

	void SetTarget(const PlayerRef& player) { _target = player; }
	PlayerRef GetTarget() const { return _target.lock(); }

	void SetLastPos();
	Protocol::PosInfo GetLastPos() { return _lastPos; }

protected:
	weak_ptr<Player> _target;
	uint64 _nextSearchTick = 0;
	uint64 _nextMoveTick = 0;

	Protocol::PosInfo _lastPos;
};

