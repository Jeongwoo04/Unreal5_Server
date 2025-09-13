// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "S1Creature.h"
#include "S1Monster.generated.h"

class AS1Player;

UCLASS()
class S1_API AS1Monster : public AS1Creature
{
	GENERATED_BODY()

public:
	AS1Monster();
	virtual ~AS1Monster();

	virtual void Tick(float DeltaTime) override;
};
