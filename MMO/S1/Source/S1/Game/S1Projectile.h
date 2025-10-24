// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Protocol.pb.h"
#include "GameFramework/Actor.h"
#include "S1Projectile.generated.h"

class AS1Creature;

UCLASS()
class S1_API AS1Projectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AS1Projectile();
	virtual ~AS1Projectile();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void Move(const Protocol::PosInfo& Info);

	virtual void SetPosInfo(const Protocol::PosInfo& Info);
	Protocol::PosInfo GetPosInfo() const { return PosInfo; }

	void SetCaster(AS1Creature* InOwner);

	virtual void OnHit();

protected:
	UPROPERTY()
	FVector TargetPos;

	UPROPERTY()
	FRotator TargetRot;

	bool MoveFlag = false;

	Protocol::PosInfo PosInfo;

	TWeakObjectPtr<AS1Creature> Caster;
};
