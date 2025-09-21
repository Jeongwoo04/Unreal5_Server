// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "S1CastingBar.generated.h"

/**
 * 
 */
struct FSkillState;

UCLASS()
class S1_API US1CastingBar : public UUserWidget
{
	GENERATED_BODY()
	
public:
    UPROPERTY(meta = (BindWidget))
    class UProgressBar* CastingBar_Fill;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* CastingBar_Text;

    void ShowCastingBar(const FSkillState& CurrentState);
    void CancelCasting();

protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    float CurrentCastTime = 0.f;
    float TotalCastTime = 0.f;
};
