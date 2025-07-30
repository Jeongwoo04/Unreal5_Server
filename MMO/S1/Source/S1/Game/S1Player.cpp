// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/S1Player.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "UObject/ConstructorHelpers.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "S1MyPlayer.h"

//////////////////////////////////////////////////////////////////////////
// ATP_ThirdPersonCharacter

AS1Player::AS1Player()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// 모든 채널에 대해 우선 차단 안 함
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);

	// 필요한 것만 허용
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block); // 바닥 같은 것
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);   // Raytrace용

	// Monster 채널만 충돌되게
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block); // Monster

	// Player 끼리 충돌은 막음 (Pawn-Pawn은 기본 Ignore지만 명시해둠)
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	GetCharacterMovement()->bRunPhysicsWithNoController = true;

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
		UE_LOG(LogTemp, Error, TEXT("Player Failed to load AnimBPClass!"));
	}
}

AS1Player::~AS1Player()
{

}

void AS1Player::BeginPlay()
{
	// Call the base class
	Super::BeginPlay();

	// 처음 시작점과 Dest가 동일하게끔 보정.
	{
		FVector Location = GetActorLocation();
		PosInfo.set_x(Location.X);
		PosInfo.set_y(Location.Y);
		PosInfo.set_z(Location.Z);
		PosInfo.set_yaw(GetControlRotation().Yaw);
		PosInfo.set_state(Protocol::STATE_MACHINE_IDLE);
	}
}

void AS1Player::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector CurrentLocation = GetActorLocation();

	if (bHasReceivedMove && !IsMyPlayer())
	{
		PreviousLocation = CurrentLocation;

		// 보간 이동
		FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetPosition, DeltaTime, 10.f);
		SetActorLocation(NewLocation);

		FRotator NewRot = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, 10.f);
		SetActorRotation(NewRot);
	}
}

void AS1Player::Move(const Protocol::PosInfo& Info)
{
	TargetPosition = FVector(Info.x(), Info.y(), Info.z());
	TargetRotation = FRotator(0.f, Info.yaw(), 0.f);
	bHasReceivedMove = true;
}

void AS1Player::SetPosInfo(const Protocol::PosInfo& Info)
{
	if (PosInfo.object_id() != 0)
	{
		assert(PlayerInfo.object_id() == Info.object_id());
	}

	PosInfo.CopyFrom(Info);

	Move(Info);
}

bool AS1Player::IsMyPlayer()
{
	return Cast<AS1MyPlayer>(this) != nullptr;
}