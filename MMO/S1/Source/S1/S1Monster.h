// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Protocol.pb.h"
#include "S1Monster.generated.h"

UCLASS()
class S1_API AS1Monster : public ACharacter
{
	GENERATED_BODY()

public:
	AS1Monster();
	virtual ~AS1Monster();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	void SetMonsterInfo(const Protocol::PosInfo& Info);
	Protocol::PosInfo GetMonsterInfo() const { return MonsterInfo; }

protected:
	Protocol::PosInfo MonsterInfo;
};
