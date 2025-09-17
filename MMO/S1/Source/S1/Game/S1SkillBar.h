// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "S1SkillBar.generated.h"

/**
 * 
 */
class US1SkillSlot;
class US1CastingBar;

UCLASS()
class S1_API US1SkillBar : public UUserWidget
{
	GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

public:
    void SetSkillSlot(int32 SlotIndex, UTexture2D* InIcon, const FText& InKey);
    bool CanUseSkill(int32 SlotIndex);
    void StartSlotCooldown(int32 SlotIndex, float CooldownTime);

private:
    UPROPERTY(meta = (BindWidget))
    US1CastingBar* CastingBar;

    UPROPERTY(meta = (BindWidget))
    US1SkillSlot* WBP_SkillSlot_0;

    UPROPERTY(meta = (BindWidget))
    US1SkillSlot* WBP_SkillSlot_1;

    UPROPERTY(meta = (BindWidget))
    US1SkillSlot* WBP_SkillSlot_2;

    UPROPERTY(meta = (BindWidget))
    US1SkillSlot* WBP_SkillSlot_3;

    UPROPERTY()
    TArray<US1SkillSlot*> SkillSlots;
};