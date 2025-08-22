// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/S1LoadoutComponent.h"

US1LoadoutComponent::US1LoadoutComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void US1LoadoutComponent::InitializeDefaultSkills(const TMap<int32, int32>& SlotToSkillMap)
{
    SlotSkillMap = SlotToSkillMap;
}

int32 US1LoadoutComponent::GetSkillSlot(int32 SlotIndex) const
{
    if (const int32* FoundSkill = SlotSkillMap.Find(SlotIndex))
    {
        return *FoundSkill;
    }
    return -1;
}