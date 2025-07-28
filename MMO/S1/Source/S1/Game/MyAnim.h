// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MyAnim.generated.h"

/**
 * 
 */
UCLASS()
class S1_API UMyAnim : public UAnimInstance
{
	GENERATED_BODY()
	
public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool ShouldMove;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float Speed;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FVector Velocity;
};
