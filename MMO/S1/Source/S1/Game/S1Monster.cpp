// Fill out your copyright notice in the Description page of Project Settings.

#include "S1Monster.h"
#include "S1GameInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

// Sets default values
AS1Monster::AS1Monster()
{
	// ObjectType : Monster로 지정 (Monster -> ECC_GameTraceChannel1)
	GetCapsuleComponent()->SetCollisionObjectType(ECC_GameTraceChannel1);

	// 기본 설정
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);

	// Player (Pawn)과만 충돌
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
}

AS1Monster::~AS1Monster()
{
}

void AS1Monster::BeginPlay()
{
	Super::BeginPlay();
	
	{
		FVector Location = GetActorLocation();
		PosInfo.set_x(Location.X);
		PosInfo.set_y(Location.Y);
		PosInfo.set_z(Location.Z);
		PosInfo.set_yaw(GetControlRotation().Yaw);
	}
}

// Called every frame
void AS1Monster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector CurrentLocation = GetActorLocation();

	if (bHasReceivedMove)
	{
		PreviousLocation = CurrentLocation;

		// 보간 이동
		FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetPosition, DeltaTime, 10.f);
		SetActorLocation(NewLocation);

		FRotator NewRot = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, 10.f);
		SetActorRotation(NewRot);
	}
}

void AS1Monster::Move(const Protocol::PosInfo& Info)
{
	TargetPosition = FVector(Info.x(), Info.y(), Info.z());
	TargetRotation = FRotator(0.f, Info.yaw(), 0.f);
	bHasReceivedMove = true;
}

void AS1Monster::SetPosInfo(const Protocol::PosInfo& Info)
{
	if (MonsterInfo.object_id() != 0)
	{
		assert(MonsterInfo.object_id() == Info.object_id());
	}

	PosInfo.CopyFrom(Info);

	Move(Info);
}

