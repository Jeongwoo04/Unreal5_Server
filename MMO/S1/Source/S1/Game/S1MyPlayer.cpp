// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/S1MyPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "S1.h"
#include "Engine/LocalPlayer.h"
#include <Kismet/KismetMathLibrary.h>

AS1MyPlayer::AS1MyPlayer()
{
	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	GetCharacterMovement()->bOrientRotationToMovement = true;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AS1MyPlayer::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AS1MyPlayer::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AS1MyPlayer::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AS1MyPlayer::Move);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AS1MyPlayer::Look);

	}

}

// MOVE_PACKET_SEND_DELAY 마다 처리를하게 될 경우. State가 변할때 대응이 안됨.
void AS1MyPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!InputVector.IsNearlyZero())
	{
		FVector Dir = GetActorForwardVector() * InputVector.Y + GetActorRightVector() * InputVector.X;
		AddMovementInput(Dir.GetSafeNormal());
	}

	// 입력값 변화 확인
	if (InputVector != CacheVector)
	{
		DirtyFlag = true;
		CacheVector = InputVector;
	}

	// 이동 중이면 일정 주기로 위치 패킷 전송
	TimeSinceLastSend += DeltaTime;
	if (!InputVector.IsNearlyZero() && TimeSinceLastSend >= MoveSendInterval)
	{
		SendMovePacket();
		TimeSinceLastSend = 0.f;
	}

	// 멈춘 직후 한번 전송 (Idle 처리)
	if (DirtyFlag && InputVector.IsNearlyZero())
	{
		SendMovePacket();
		DirtyFlag = false;
		TimeSinceLastSend = 0.f;
	}
}

void AS1MyPlayer::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	CacheVector = InputVector;
	InputVector = Value.Get<FVector2D>();
}

void AS1MyPlayer::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AS1MyPlayer::SendMovePacket()
{
	FVector Loc = GetActorLocation();
	FRotator Rot = GetActorRotation();

	Protocol::C_MOVE MovePkt;
	auto Info = MovePkt.mutable_info();

	PosInfo.set_object_id(PosInfo.object_id()); // 기존 ID 유지
	PosInfo.set_x(Loc.X);
	PosInfo.set_y(Loc.Y);
	PosInfo.set_z(Loc.Z);
	PosInfo.set_yaw(Rot.Yaw);
	PosInfo.set_move_type(Protocol::MOVE_STATE_WALK);
	PosInfo.set_state(Protocol::STATE_MACHINE_MOVING);

	Info->CopyFrom(PosInfo);

	SEND_PACKET(MovePkt);

	DirtyFlag = false;
}
