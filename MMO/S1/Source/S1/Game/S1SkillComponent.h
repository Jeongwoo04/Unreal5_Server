// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "S1SkillComponent.generated.h"

class AS1MarkerActor;
class AS1MyPlayer;
struct ClientAction;

USTRUCT()
struct FClientActionInstance
{
	GENERATED_BODY()

	const struct ClientAction* Action = nullptr;
	float Elapsed = 0.f;
	bool bTriggered = false;
};

USTRUCT()
struct FSkillState
{
	GENERATED_BODY()

	int32 SkillID = -1;
	FString name = "";
	float CastTime = 0.f;
	float CastElapsed = 0.f;
	bool bIsCasting = false;
	TArray<FClientActionInstance> ActionInstances;
	int32 CurrentActionIndex = 0;
	FVector TargetPos = FVector::ZeroVector;
	int32 CastID = 0;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class S1_API US1SkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	US1SkillComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	void BeginSkillTargeting(int32 SkillID, float Distance, float Range);
	void CancelSkillTargeting();
	void ConfirmSkillTargeting();

	bool IsSkillTargeting();

private:
	// 마커 업데이트
	void UpdateSkillMarkers();

	void UpdateSkillAreaMarker();
	void UpdateSkillRangeMarker();

	// 마커 삭제
	void ClearSkillMarkers();

	void TickSkillState(float DeltaTime);
	void ExecuteAction(FClientActionInstance& ActionInstance);

private:
	UPROPERTY()
	AS1MarkerActor* SkillAreaMarker = nullptr;

	UPROPERTY()
	AS1MarkerActor* SkillRangeMarker = nullptr;

	UPROPERTY(EditDefaultsOnly)
	UMaterialInterface* SkillAreaMaterial;

	UPROPERTY(EditDefaultsOnly)
	UMaterialInterface* SkillRangeMaterial;

	UPROPERTY()
	class AS1MyPlayer* CachedPlayer;

	FTimerHandle SkillAreaUpdateTimer;
	float SkillAreaUpdateInterval = 0.05f;

	FSkillState CurrentSkillState;

	int32 CurrentSkillID = -1;
	bool bIsSkillTargeting = false;

	float CurrentSkillDistance = 0.f; // distance
	float CurrentSkillRange = 0.f;
};
