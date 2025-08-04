#pragma once
#include "Object.h"

class Projectile : public Object
{
public:
	Projectile();
	virtual ~Projectile();

	virtual void Update();

	void SetData(Skill data) { _data = data; }
	Skill GetData() const { return _data; }

protected:
	Skill _data;
};