#pragma once
#include "Object.h"

class Projectile : public Object
{
public:
	Projectile();
	virtual ~Projectile();

	virtual void Update();

private:

};