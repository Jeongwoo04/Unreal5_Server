// Fill out your copyright notice in the Description page of Project Settings.

#include "S1Monster.h"
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
		MonsterInfo.set_x(Location.X);
		MonsterInfo.set_y(Location.Y);
		MonsterInfo.set_z(Location.Z);
		MonsterInfo.set_yaw(GetControlRotation().Yaw);
	}
}

// Called every frame
void AS1Monster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AS1Monster::SetMonsterInfo(const Protocol::PosInfo& Info)
{

}

