// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "S1LoadoutComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class S1_API US1LoadoutComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    US1LoadoutComponent();

    void InitializeDefaultSkills(const TMap<int32, int32>& SlotToSkillMap);
    int32 GetSkillIdWithSlot(int32 SlotIndex) const;
    int32 GetSkillSlotWithId(int32 SkillID) const;

private:
    // SlotIndex (0~3) ¡æ SkillID
    UPROPERTY()
    TMap<int32, int32> SlotSkillMap;
};
