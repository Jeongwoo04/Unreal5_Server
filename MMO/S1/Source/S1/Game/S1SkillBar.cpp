// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/S1SkillBar.h"
#include "Game/S1SkillSlot.h"


void US1SkillBar::NativeConstruct()
{
    Super::NativeConstruct();
    SkillSlots = { WBP_SkillSlot_0, WBP_SkillSlot_1, WBP_SkillSlot_2, WBP_SkillSlot_3 };
}

void US1SkillBar::SetSkillSlot(int32 SlotIndex, UTexture2D* InIcon, const FText& InKey)
{
    if (SkillSlots.IsValidIndex(SlotIndex))
    {
        SkillSlots[SlotIndex]->SetSkill(InIcon, InKey);
    }
}

bool US1SkillBar::CanUseSkill(int32 SlotIndex)
{
    return SkillSlots[SlotIndex]->CanUseSkill();
}

void US1SkillBar::StartSlotCooldown(int32 SlotIndex, float CooldownTime)
{
    if (SkillSlots.IsValidIndex(SlotIndex))
    {
        SkillSlots[SlotIndex]->StartCooldown(CooldownTime);
    }
}