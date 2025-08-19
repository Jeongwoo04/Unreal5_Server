// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "S1SkillBar.generated.h"

/**
 * 
 */
class US1SkillSlot;

UCLASS()
class S1_API US1SkillBar : public UUserWidget
{
	GENERATED_BODY()

protected:
    /** 접근하기 쉽게 배열로 묶음 */
    virtual void NativeConstruct() override;

public:
    /** 특정 슬롯에 스킬 세팅 */
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void SetSkillSlot(int32 SlotIndex, UTexture2D* InIcon, const FText& InKey);

    /** 특정 슬롯 쿨타임 시작 */
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void StartSlotCooldown(int32 SlotIndex, float CooldownTime);

protected:
    /** 디자이너에 있는 4개 슬롯 바인딩 */
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
