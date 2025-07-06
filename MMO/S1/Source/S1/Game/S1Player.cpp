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
#include "S1MyPlayer.h"
#include <Kismet/GameplayStatics.h>

//////////////////////////////////////////////////////////////////////////
// ATP_ThirdPersonCharacter

AS1Player::AS1Player()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	GetCharacterMovement()->bRunPhysicsWithNoController = true;
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
		DestInfo.set_x(Location.X);
		DestInfo.set_y(Location.Y);
		DestInfo.set_z(Location.Z);
		DestInfo.set_yaw(GetControlRotation().Yaw);
		SetMoveState(Protocol::MOVE_STATE_IDLE);
	}
}

void AS1Player::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	{
		FVector Location = GetActorLocation();
		PlayerInfo.set_x(Location.X);
		PlayerInfo.set_y(Location.Y);
		PlayerInfo.set_z(Location.Z);
		PlayerInfo.set_yaw(GetControlRotation().Yaw);
	}

	// OtherPlayer 일 경우 보정
	if (IsMyPlayer() == false)
	{
		//FVector Location = GetActorLocation();
		//FVector DestLocation = FVector(DestInfo.x(), DestInfo.y(), DestInfo.z());

		//FVector MoveDir = (DestLocation - Location);
		//const float DistToDest = MoveDir.Length();
		//MoveDir.Normalize();

		//// 방향 단위벡터 * 이동속도 * DeltaSeconds * 이동거리
		//float MoveDist = (MoveDir * 600.f * DeltaSeconds).Length();
		//// 거의 도달한경우 목적 위치를 초과하는 문제가 생길 수 있음. 왔다갔다 하는 현상
		//MoveDist = FMath::Min(MoveDist, DistToDest);
		//FVector NextLocation = Location + MoveDir * MoveDist;

		//SetActorLocation(NextLocation);
		const Protocol::MoveState State = PlayerInfo.state();

		if (State == Protocol::MOVE_STATE_RUN)
		{
			SetActorRotation(FRotator(0, DestInfo.yaw(), 0));
			AddMovementInput(GetActorForwardVector());
		}
		else
		{

		}
	}
}

bool AS1Player::IsMyPlayer()
{
	return Cast<AS1MyPlayer>(this) != nullptr;
}

void AS1Player::SetMoveState(Protocol::MoveState State)
{
	if (PlayerInfo.state() == State)
		return;

	PlayerInfo.set_state(State);
}

void AS1Player::SetPlayerInfo(const Protocol::PlayerInfo& Info)
{
	if (PlayerInfo.object_id() != 0)
	{
		assert(PlayerInfo.object_id() == Info.object_id());
	}

	PlayerInfo.CopyFrom(Info);

	FVector Location(Info.x(), Info.y(), Info.z());
	SetActorLocation(Location);
}

void AS1Player::SetDestInfo(const Protocol::PlayerInfo& Info)
{
	if (PlayerInfo.object_id() != 0)
	{
		assert(PlayerInfo.object_id() == Info.object_id());
	}

	DestInfo.CopyFrom(Info);
	SetMoveState(Info.state());
}
