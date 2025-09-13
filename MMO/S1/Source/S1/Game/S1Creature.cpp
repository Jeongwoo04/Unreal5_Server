// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/S1Creature.h"
#include "Components//CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "S1Monster.h"
#include "S1MyPlayer.h"

AS1Creature::AS1Creature()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 100.f, 0.f);
}

AS1Creature::~AS1Creature()
{

}

void AS1Creature::BeginPlay()
{
	Super::BeginPlay();

	{
		FVector Location = GetActorLocation();
		PosInfo.set_x(Location.X);
		PosInfo.set_y(Location.Y);
		PosInfo.set_z(Location.Z);
		PosInfo.set_yaw(GetControlRotation().Yaw);
		PosInfo.set_state(Protocol::STATE_MACHINE_IDLE);
	}
}

void AS1Creature::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateState(DeltaTime);
	UpdateAnim(DeltaTime);
}

void AS1Creature::UpdateState(float DeltaTime)
{
	switch (PosInfo.state())
	{
	case Protocol::STATE_MACHINE_IDLE:
		UpdateIdle(DeltaTime);
		// 이동/스킬 대기
		break;
	case Protocol::STATE_MACHINE_MOVING:
		UpdateMoving(DeltaTime);
		// 서버 위치 기준 이동 + 애니메이션
		break;
	case Protocol::STATE_MACHINE_CASTING:
		UpdateCasting(DeltaTime);
		// Casting 시간 체크, 완료 후 Skill 상태로
		break;
	case Protocol::STATE_MACHINE_SKILL:
		UpdateSkill(DeltaTime);
		// 스킬 효과 처리
		break;
	case Protocol::STATE_MACHINE_DEAD:
		UpdateDead(DeltaTime);
		// 사망 처리
		break;
	}
}

void AS1Creature::ChangeState(Protocol::StateMachine NewState)
{
	if (PosInfo.state() == NewState)
		return;

	OnExitState();

	PosInfo.set_state(NewState);

	OnEnterState();
}

void AS1Creature::UpdateIdle(float DeltaTime)
{
}

void AS1Creature::UpdateMoving(float DeltaTime)
{
	PreviousLoc = GetActorLocation();

	// 보간 이동
	FVector NewLocation = FMath::VInterpTo(PreviousLoc, TargetPos, DeltaTime, 10.f);
	SetActorLocation(NewLocation);

	FRotator NewRot = FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaTime, 10.f);
	SetActorRotation(NewRot);

	// Square 오차범위가 크면. Moving 초반에 애니메이션 동기화 깨짐
	if (FVector::DistSquared(NewLocation, TargetPos) < FMath::Square(5.f)) // 1cm 이내
	{
		SetActorLocation(TargetPos);
		SetActorRotation(TargetRot);
	}
}

void AS1Creature::UpdateCasting(float DeltaTime)
{
}

void AS1Creature::UpdateSkill(float DeltaTime)
{
}

void AS1Creature::UpdateDead(float DeltaTime)
{
}

void AS1Creature::UpdateAnim(float DeltaTime)
{
	if (US1AnimInstance* AI = Cast<US1AnimInstance>(GetMesh()->GetAnimInstance()))
	{
		AI->SetAnimState(static_cast<EStateMachine>(PosInfo.state()));
		AI->GroundSpeed = PosInfo.speed();
	}
}

void AS1Creature::Move(const Protocol::PosInfo& Info)
{
	TargetPos = FVector(Info.x(), Info.y(), Info.z());
	TargetRot = FRotator(0.f, Info.yaw(), 0.f);
}

void AS1Creature::SetPosInfo(const Protocol::PosInfo& Info)
{
	if (PosInfo.object_id() != 0)
	{
		assert(PosInfo.object_id() == Info.object_id());
	}

	PosInfo.CopyFrom(Info);

	Move(Info);
}

