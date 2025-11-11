// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Protocol.pb.h"
#include "S1AnimEnum.h"
#include "S1AnimInstance.h"
#include "GameFramework/Character.h"
#include "S1Creature.generated.h"

class US1SkillComponent;

UCLASS()
class S1_API AS1Creature : public ACharacter
{
	GENERATED_BODY()

public:
	AS1Creature();
	virtual ~AS1Creature();

	virtual void ChangeState(Protocol::StateMachine NewState);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	virtual void UpdateState(float DeltaTime);

	virtual void UpdateIdle(float DeltaTime);
	virtual void UpdateMoving(float DeltaTime);
	virtual void UpdateCasting(float DeltaTime);
	virtual void UpdateSkill(float DeltaTime);
	virtual void UpdateDead(float DeltaTime);

	virtual void UpdateAnim();

public:
	virtual void SetPosInfo(const Protocol::PosInfo& Info);
	Protocol::PosInfo GetPosInfo() const { return PosInfo; }

	void SetYaw(float Yaw) { PosInfo.set_yaw(Yaw); }

	EStateMachine GetCurrentState() const { return StaticCast<EStateMachine>(PosInfo.state()); }

	void HandleActionPkt(const Protocol::S_ACTION& Pkt);

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	US1SkillComponent* SkillComponent;

protected:
	UPROPERTY()
	FVector TargetPos;
	UPROPERTY()
	FRotator TargetRot;

	float ServerTickInterval = 0.1f;
	bool bShouldInterp = false;

	Protocol::PosInfo PosInfo;
};
