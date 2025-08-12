#pragma once
#include "Object.h"

class Projectile : public Object
{
public:
	Projectile();
	virtual ~Projectile();

	virtual void Update();

	void SetOwner(const ObjectRef& object) { _owner = object; }
	ObjectRef GetOwner() { return _owner.lock(); }

	void SetData(const Skill& data) { _data = data; }
	Skill GetData() { return _data; }

protected:
	weak_ptr<Object> _owner;

	Vector3 _destPos;
	float _radius = 20.f;

	float _distance = 0.f;

	uint64 _nextMoveTick = 0;
	Skill _data;
};