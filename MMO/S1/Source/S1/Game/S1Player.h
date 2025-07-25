// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Protocol.pb.h"
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

protected:
	UPROPERTY()
	FVector TargetPosition;

	UPROPERTY()
	FRotator TargetRotation;

	UPROPERTY()
	float LerpAlpha = 0.1f;

	bool bHasReceivedMove = false;

	class Protocol::PosInfo PosInfo;
};
