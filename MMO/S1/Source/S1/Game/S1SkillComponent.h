// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "S1SkillComponent.generated.h"

class AS1MarkerActor;
class AS1Creature;
class AS1MyPlayer;
struct ClientAction;

USTRUCT()
struct FClientActionInstance
{
	GENERATED_BODY()

	const struct ClientAction* Action = nullptr;
	uint64 StartTick = 0;
	uint64 Elapsed = 0;
	bool bTriggered = false;
};

USTRUCT()
struct FSkillState
{
	GENERATED_BODY()

	int32 SkillID = -1;
	FString name = "";
	int32 CastID = 0;
	uint64 CastStartTick = 0;
	uint64 CastDuration = 0;
	uint64 CooldownStartTick = 0;
	uint64 CooldownDuration = 0;
	bool bIsCasting = false;
	bool bIsCooldown = false;
	TArray<FClientActionInstance> ActionInstances;
	int32 CurrentActionIndex = 0;
	FVector TargetPos = FVector::ZeroVector;
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
	// 로컬 입력
	void BeginSkillTargeting(int32 SkillID, float Distance, float Range);
	void CancelSkillTargeting();
	void ConfirmSkillTargeting(int32 SkillID);

	bool IsSkillTargeting();

public:
	bool CanUseSkill(int32 SkillID);
	bool IsCasting();
	void LocalCancelCasting();
	void ServerCancelCasting(int32 SkillID);

	void HandleActionPkt(const Protocol::S_SKILL& Pkt);

private:
	// 마커 업데이트
	void UpdateSkillMarkers();

	void UpdateSkillAreaMarker();
	void UpdateSkillRangeMarker();

	// 마커 삭제
	void ClearSkillMarkers();

	void DoCastTick(float DeltaTime);
	void DoSkillStart(int32 SkillID);
	void DoSkillStateTick(float DeltaTime);

	void HandleExecuteAction(FClientActionInstance& ActionInstance);
	void ExecuteAction(const ClientAction& Action, const FVector& TargetPos);

public:
	FSkillState* GetCurrentSkillState() { return SkillStates.Find(CurrentSkillID); }

	FSkillState* GetSkillState(int32 SkillID) { return SkillStates.Find(SkillID); }
	void AddSkillState(int32 SkillID, FSkillState State) { SkillStates.Add(SkillID, State); }

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
	class AS1Creature* OwnerCreature;

	UPROPERTY()
	class AS1MyPlayer* CachedPlayer;

	UPROPERTY()
	UWorld* World;

	FTimerHandle SkillAreaUpdateTimer;
	float SkillAreaUpdateInterval = 0.05f;

	TMap<int32, FSkillState> SkillStates;

	TQueue<int32> PendingSkillQueue;
	int32 CurrentSkillID = -1;

	bool bIsSkillTargeting = false;

	float CurrentSkillDistance = 0.f; // distance
	float CurrentSkillRange = 0.f;

	FVector CachedTargetLoc;
};
