#pragma once
#include "Projectile.h"

class Arrow : public Projectile
{
public:
	void SetOwner(const ObjectRef& object) { _owner = object; }
	ObjectRef GetOwner() { return _owner.lock(); }

	virtual void Update() override;

private:
	weak_ptr<Object> _owner;
	uint64 _nextMoveTick = 0;
};

