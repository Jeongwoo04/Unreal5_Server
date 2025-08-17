// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/S1Creature.h"
#include "Components//CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "S1Monster.h"

AS1Creature::AS1Creature()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
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
	TickServerMove(DeltaTime);
}

void AS1Creature::TickServerMove(float DeltaTime)
{
	if (MoveFlag)
	{
		PreviousLoc = GetActorLocation();

		// 보간 이동
		FVector NewLocation = FMath::VInterpTo(PreviousLoc, TargetPos, DeltaTime, 10.f);
		SetActorLocation(NewLocation);

		FRotator NewRot = FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaTime, 10.f);
		SetActorRotation(NewRot);

		if (FVector::DistSquared(NewLocation, TargetPos) < FMath::Square(1.f)) // 1cm 이내
		{
			MoveFlag = false;
			SetActorLocation(TargetPos);
			SetActorRotation(TargetRot);
		}
	}
}

void AS1Creature::Move(const Protocol::PosInfo& Info)
{
	TargetPos = FVector(Info.x(), Info.y(), Info.z());
	TargetRot = FRotator(0.f, Info.yaw(), 0.f);
	MoveFlag = true;
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

