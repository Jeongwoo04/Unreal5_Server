// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Protocol.pb.h"
#include "GameFramework/Actor.h"
#include "S1Field.generated.h"

class AS1Creature;

UCLASS()
class S1_API AS1Field : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AS1Field();
	virtual ~AS1Field();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void SetPosInfo(const Protocol::PosInfo& Info);
	Protocol::PosInfo GetPosInfo() const { return PosInfo; }

	void SetCaster(AS1Creature* InOwner);

	virtual void OnHit();

protected:
	UPROPERTY()
	float Range;

	Protocol::PosInfo PosInfo;

	TWeakObjectPtr<AS1Creature> Caster;
};
