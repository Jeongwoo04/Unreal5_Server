#pragma once
#include "Projectile.h"

class Arrow : public Projectile
{
public:
	virtual void Update() override;

public:
	int32 _tickCount = 0;
};

