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
	virtual void Tick(float DeltaSeconds) override;

public:
	virtual void ApplyServerPos(const Protocol::PosInfo& NewInfo);

	void SetPosInfo(const Protocol::PosInfo& Info);
	Protocol::PosInfo GetPosInfo() { return PosInfo; }

	bool IsMyPlayer();

protected:
	class Protocol::PosInfo PosInfo;
};
