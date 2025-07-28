#pragma once

#include "CoreMinimal.h"
#include "S1AnimEnum.generated.h"

UENUM(BlueprintType)
enum class EStateMachine : uint8
{
	STATE_MACHINE_NONE    UMETA(DisplayName = "None"),
	STATE_MACHINE_IDLE    UMETA(DisplayName = "Idle"),
	STATE_MACHINE_PATROL  UMETA(DisplayName = "Patrol"),
	STATE_MACHINE_MOVING  UMETA(DisplayName = "Moving"),
	STATE_MACHINE_SKILL   UMETA(DisplayName = "Skill"),
	STATE_MACHINE_DEAD    UMETA(DisplayName = "Dead")
};