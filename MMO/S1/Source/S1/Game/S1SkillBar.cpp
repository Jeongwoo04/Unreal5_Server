// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/S1SkillBar.h"
#include "Game/S1SkillSlot.h"

void US1SkillBar::NativeConstruct()
{
    Super::NativeConstruct();

    SkillSlots.Empty();
    if (WBP_SkillSlot_0) SkillSlots.Add(WBP_SkillSlot_0);
    if (WBP_SkillSlot_1) SkillSlots.Add(WBP_SkillSlot_1);
    if (WBP_SkillSlot_2) SkillSlots.Add(WBP_SkillSlot_2);
    if (WBP_SkillSlot_3) SkillSlots.Add(WBP_SkillSlot_3);
}

void US1SkillBar::SetSkillSlot(int32 SlotIndex, UTexture2D* InIcon, const FText& InKey)
{
    if (SkillSlots.IsValidIndex(SlotIndex))
    {
        SkillSlots[SlotIndex]->SetSkill(InIcon, InKey);
    }
}

void US1SkillBar::StartSlotCooldown(int32 SlotIndex, float CooldownTime)
{
    if (SkillSlots.IsValidIndex(SlotIndex))
    {
        SkillSlots[SlotIndex]->StartCooldown(CooldownTime);
    }
}