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

    void SetAnimState(const EStateMachine& EState);

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim")
    EStateMachine State;

    // BS 모션 이동 속도
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim")
    float GroundSpeed;

    // 공격 속도 (Casting / Skill 재생 속도 조정용)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim")
    float AttackSpeed;

    //// 상태별 몽타주 재생 여부
    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim")
    //bool bPlayCasting;
    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim")
    //bool bPlaySkill;
    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim")
    //bool bPlayDead;
};
