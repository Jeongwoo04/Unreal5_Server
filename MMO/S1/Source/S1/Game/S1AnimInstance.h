// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "S1AnimEnum.h"
#include "Protocol.pb.h"
#include "S1AnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class S1_API US1AnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    void SetAnimState(const EStateMachine& EState)
    {
        State = EState;
    }
    void SetAnimStateFromPosInfo(const Protocol::PosInfo& PosInfo, const FVector& NewVelocity);

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim")
    float GroundSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim")
    EStateMachine State;
};
