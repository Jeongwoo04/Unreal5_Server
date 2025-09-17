// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Protocol.pb.h"
#include "S1AnimEnum.h"
#include "S1AnimInstance.h"
#include "GameFramework/Character.h"
#include "S1Creature.generated.h"

UCLASS()
class S1_API AS1Creature : public ACharacter
{
	GENERATED_BODY()

public:
	AS1Creature();
	virtual ~AS1Creature();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	virtual void UpdateState(float DeltaTime);
	virtual void ChangeState(Protocol::StateMachine NewState);

	virtual void UpdateIdle(float DeltaTime);
	virtual void UpdateMoving(float DeltaTime);
	virtual void UpdateCasting(float DeltaTime);
	virtual void UpdateSkill(float DeltaTime);
	virtual void UpdateDead(float DeltaTime);

	virtual void UpdateAnim();

public:
	virtual void Move(const Protocol::PosInfo& Info);

	virtual void SetPosInfo(const Protocol::PosInfo& Info);
	Protocol::PosInfo GetPosInfo() const { return PosInfo; }

	EStateMachine GetCurrentState() const { return StaticCast<EStateMachine>(PosInfo.state()); }

protected:
	UPROPERTY()
	FVector TargetPos;
	UPROPERTY()
	FRotator TargetRot;

	Protocol::PosInfo PosInfo;
};
