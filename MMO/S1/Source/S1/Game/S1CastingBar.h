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
    void StartCasting(const FSkillState& SkillState, uint64 CastEndTick = 0);
    void CancelCasting();

protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    void InitializeCast(const FSkillState& SkillState, uint64 CastEndTick);
    void UpdateCastBar(float DeltaTime);

private:
    UPROPERTY(meta = (BindWidget))
    class UProgressBar* CastingBar_Fill;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* CastingBar_Text;

    float ElapsedCastTime = 0.f;    // 경과 시간
    float CastDuration = 0.f;       // 총 캐스팅 시간
    bool bIsCasting = false;        // 캐스팅 중 여부
};
