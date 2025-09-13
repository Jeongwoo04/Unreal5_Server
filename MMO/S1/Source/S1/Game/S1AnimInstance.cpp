// Fill out your copyright notice in the Description page of Project Settings.


#include "S1AnimInstance.h"
#include "S1Player.h"
#include "S1MyPlayer.h"
#include "S1Monster.h"
#include "S1.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/Character.h"

void US1AnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();
}

void US1AnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    //APawn* Pawn = TryGetPawnOwner();
    //if (!Pawn)
    //    return;

    //FVector CurrentVelocity = Pawn->GetVelocity();

    //AS1Player* Player = Cast<AS1Player>(Pawn); // 공통 부모
    //if (Player)
    //{
    //    State = Player->GetCurrentState();
    //    GroundSpeed = Player->GetPosInfo().speed();
    //}
    //else
    //{
    //    AS1Monster* Monster = Cast<AS1Monster>(Pawn);

    //    if (Monster)
    //    {
    //        State = Monster->GetCurrentState();
    //        GroundSpeed = Monster->GetPosInfo().speed();
    //    }
    //}
}

void US1AnimInstance::SetAnimState(const EStateMachine& NewState)
{
    State = NewState;
}