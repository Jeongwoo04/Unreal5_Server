// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "S1Creature.h"
#include "S1Player.generated.h"

UCLASS()
class S1_API AS1Player : public AS1Creature
{
	GENERATED_BODY()

public:
	AS1Player();
	virtual ~AS1Player();

public:
	bool IsMyPlayer();

protected:
	virtual void Tick(float DeltaTime) override;
};
