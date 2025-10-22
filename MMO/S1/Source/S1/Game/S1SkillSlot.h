// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "S1SkillSlot.generated.h"

/**
 * 
 */
class UImage;
class UTextBlock;
class UProgressBar;
class UTexture2D;

UCLASS()
class S1_API US1SkillSlot : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetSkill(UTexture2D* InIcon, const FText& InKey);
    void StartCooldown(uint64 CooldownTick);
    bool CanUseSkill();

protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    UPROPERTY(meta = (BindWidget))
    UImage* Img_Icon;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Txt_Key;

    UPROPERTY(meta = (BindWidget))
    UProgressBar* ProgressBar;

private:
    bool bIsOnCooldown = false;
    uint64 CooldownStartTick = 0;
    uint64 CooldownDurationTick = 0;
};
