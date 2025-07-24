// Fill out your copyright notice in the Description page of Project Settings.

#include "S1Monster.h"
#include "S1GameInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
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

	if (!TargetPlayer)
		return;

	FVector Dir = (TargetPlayer->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	FVector DesiredMove = Dir * GetCharacterMovement()->MaxWalkSpeed * DeltaTime;
	FVector NewLocation = GetActorLocation() + DesiredMove;

	auto* MapManager = GetGameInstance<US1GameInstance>()->MapManager;
	if (!MapManager->IsBlocked(NewLocation.X, NewLocation.Y))
	{
		SetActorLocation(NewLocation);
	}

	// 회전
	FRotator NewRot = Dir.Rotation();
	SetActorRotation(NewRot);
}

void AS1Monster::SetMonsterInfo(const Protocol::ObjectInfo& Info)
{
	if (Info.object_id() != 0)
	{
		assert(MonsterInfo.object_id() == Info.object_id());
	}

	PosInfo.CopyFrom(Info);
}

void AS1Monster::SetPosInfo(const Protocol::PosInfo& Info)
{
	if (MonsterInfo.object_id() != 0)
	{
		assert(MonsterInfo.object_id() == Info.object_id());
	}

	PosInfo.CopyFrom(Info);

	//FVector Location(Info.x(), Info.y(), Info.z());
	//SetActorLocation(Location);
	FVector NewLocation(Info.x(), Info.y(), Info.z());
	FVector OldLocation = GetActorLocation();

	UE_LOG(LogTemp, Warning, TEXT("[AS1Monster] Old: (%.2f, %.2f, %.2f), New: (%.2f, %.2f, %.2f)"),
		OldLocation.X, OldLocation.Y, OldLocation.Z,
		NewLocation.X, NewLocation.Y, NewLocation.Z);


	SetActorLocation(NewLocation);
}

