
#pragma once
#include "Object.h"

class Projectile : public Object
{
public:
	Projectile();
	virtual ~Projectile();

	virtual void Update(float deltaTime) override;

	void SetData(const ProjectileInfo* data) { _data = data; }
	void SetOwner(ObjectRef owner) { _owner = owner; }
	void SetDir(const Vector3& dir) { _dir = dir; }

	ObjectRef GetOwner() { return _owner.lock(); }

private:
	weak_ptr<Object> _owner;
	const ProjectileInfo* _data = nullptr;
	float _moveDistance = 0.f;

	Vector3 _dir;
};