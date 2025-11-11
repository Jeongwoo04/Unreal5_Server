// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/S1Creature.h"
#include "Components//CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "S1SkillComponent.h"

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

	PosInfo.set_state(NewState);

	//UpdateAnim();
}

void AS1Creature::UpdateIdle(float DeltaTime)
{
	// Idle 시에는 오차 보정만 수행
	FVector CurrentLoc = GetActorLocation();
	if (FVector::Dist(CurrentLoc, TargetPos) > 5.f)
	{
		SetActorRotation(TargetRot);
		SetActorLocation(FMath::VInterpTo(CurrentLoc, TargetPos, DeltaTime, 10.f));
	}
}

void AS1Creature::UpdateMoving(float DeltaTime)
{
	SetActorRotation(TargetRot);

	if (!bShouldInterp)
	{
		SetActorLocation(TargetPos);
		return;
	}

	FVector NewLoc = FMath::VInterpConstantTo(GetActorLocation(), TargetPos, DeltaTime, PosInfo.speed());
	SetActorLocation(NewLoc);

	if (FVector::Dist(NewLoc, TargetPos) < 1.f)
	{
		bShouldInterp = false;
	}
}

void AS1Creature::UpdateCasting(float DeltaTime)
{
	FVector NewLoc = FMath::VInterpConstantTo(GetActorLocation(), TargetPos, DeltaTime, PosInfo.speed());
	if (FVector::Dist(NewLoc, TargetPos) < 1.f)
	{
		return;
	}
	SetActorLocation(NewLoc);
}

void AS1Creature::UpdateSkill(float DeltaTime)
{

}

void AS1Creature::UpdateDead(float DeltaTime)
{

}

void AS1Creature::UpdateAnim()
{
	if (US1AnimInstance* AI = Cast<US1AnimInstance>(GetMesh()->GetAnimInstance()))
	{
		AI->SetAnimState(static_cast<EStateMachine>(PosInfo.state()));
		AI->GroundSpeed = PosInfo.speed();
		// MoveSpeed , AttackSpeed 등등 업데이트
	}
}

void AS1Creature::SetPosInfo(const Protocol::PosInfo& Info)
{
	if (PosInfo.object_id() != 0)
	{
		assert(PosInfo.object_id() == Info.object_id());
	}

	TargetPos = FVector(Info.x(), Info.y(), Info.z());
	TargetRot = FRotator(0.f, Info.yaw(), 0.f);

	PosInfo.CopyFrom(Info);

	bShouldInterp = FVector::Dist(GetActorLocation(), TargetPos) > KINDA_SMALL_NUMBER;
}

void AS1Creature::HandleActionPkt(const Protocol::S_ACTION& Pkt)
{
	SkillComponent->HandleActionPkt(Pkt);
}

