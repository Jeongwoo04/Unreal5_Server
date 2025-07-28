// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Protocol.pb.h"
#include "S1AnimEnum.h"
#include "S1AnimInstance.h"
#include "S1Player.generated.h"

UCLASS()
class S1_API AS1Player : public ACharacter
{
	GENERATED_BODY()

public:
	AS1Player();
	virtual ~AS1Player();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	void Move(const Protocol::PosInfo& Info);

	void SetPosInfo(const Protocol::PosInfo& Info);
	Protocol::PosInfo GetPosInfo() { return PosInfo; }

	bool IsMyPlayer();

public:
	void SetState(const Protocol::StateMachine& State) { PosInfo.set_state(State); }
	EStateMachine GetCurrentState() const { return StaticCast<EStateMachine>(PosInfo.state()); }

protected:
	UPROPERTY()
	FVector TargetPosition;

	UPROPERTY()
	FRotator TargetRotation;

	bool bHasReceivedMove = false;

	FVector PreviousLocation;

	class Protocol::PosInfo PosInfo;
};
