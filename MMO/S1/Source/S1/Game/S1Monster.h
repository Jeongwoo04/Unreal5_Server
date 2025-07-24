// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Protocol.pb.h"
#include "S1Player.h"
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
	void SetTargetPlayer(AS1Player* Player) {}

	void SetMonsterInfo(const Protocol::ObjectInfo& Info);
	void SetPosInfo(const Protocol::PosInfo& Info);
	Protocol::PosInfo GetPosInfo() const { return PosInfo; }

protected:
	Protocol::PosInfo PosInfo;
	Protocol::ObjectInfo MonsterInfo;
	AS1Player* TargetPlayer = nullptr;
};
