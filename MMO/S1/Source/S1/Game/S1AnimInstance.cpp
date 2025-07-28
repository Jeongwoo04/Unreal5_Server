// Fill out your copyright notice in the Description page of Project Settings.


#include "S1AnimInstance.h"
#include "S1Player.h"
#include "S1MyPlayer.h"
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

    APawn* Pawn = TryGetPawnOwner();
    if (!Pawn)
        return;

    FVector CurrentVelocity = Pawn->GetVelocity();

    AS1Player* Player = Cast<AS1Player>(Pawn); // 공통 부모
    if (Player)
    {
        // 서버에서 받은 상태 기준으로 설정 (Player or Monster 공통)
        State = Player->GetCurrentState(); // ← virtual 함수
        GroundSpeed = Player->GetPosInfo().speed(); // ← PosInfo는 공통 멤버
        UE_LOG(LogTemp, Warning, TEXT("AnimInstance: PosInfo speed: %f"), Player->GetPosInfo().speed());
    }
    else
    {
        // Fallback: 비정상 Actor일 경우 속도 기반 계산
        GroundSpeed = FVector(CurrentVelocity.X, CurrentVelocity.Y, 0.f).Size();
        UE_LOG(LogTemp, Warning, TEXT("Pawn: %s | Velocity: %s | IsValid Player: %s"),
            *Pawn->GetName(), *CurrentVelocity.ToString(), Player ? TEXT("Yes") : TEXT("No"));
    }
}

void US1AnimInstance::SetAnimStateFromPosInfo(const Protocol::PosInfo& PosInfo, const FVector& NewVelocity)
{
    State = static_cast<EStateMachine>(PosInfo.state());
    GroundSpeed = PosInfo.speed();
}