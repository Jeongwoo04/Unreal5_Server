// Fill out your copyright notice in the Description page of Project Settings.

#include "S1Monster.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

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

	// Skeletal Mesh 로드
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny"));
	if (MeshAsset.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MeshAsset.Object);
	}

	// Anim Blueprint 로드
	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimBP(TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C"));
	if (AnimBP.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(AnimBP.Class);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Monster Failed to load AnimBPClass!"));
	}
}

AS1Monster::~AS1Monster()
{

}

void AS1Monster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AS1Monster::UpdateIdle(float DeltaTime)
{
	if (!GetActorRotation().Equals(TargetRot, 1.f))
		SetActorRotation(TargetRot);
}