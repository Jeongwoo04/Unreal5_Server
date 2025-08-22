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
#include "S1PlayerController.h"
#include "Data/S1DataManager.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "S1SkillBar.h"
#include "S1SkillSlot.h"

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
	LoadoutComponent = CreateDefaultSubobject<US1LoadoutComponent>(TEXT("LoadoutComponent"));
}

void AS1MyPlayer::BeginPlay()
{
	Super::BeginPlay();
}

//////////////////////////////////////////////////////////////////////////
// Input

void AS1MyPlayer::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AS1MyPlayer::InputMove);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AS1MyPlayer::InputMove);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AS1MyPlayer::InputLook);

		//Skill
		EnhancedInputComponent->BindAction(Skill1Action, ETriggerEvent::Started, this, &AS1MyPlayer::UseSkillSlot1);
		EnhancedInputComponent->BindAction(Skill2Action, ETriggerEvent::Started, this, &AS1MyPlayer::UseSkillSlot2);
		EnhancedInputComponent->BindAction(Skill3Action, ETriggerEvent::Started, this, &AS1MyPlayer::UseSkillSlot3);
		EnhancedInputComponent->BindAction(Skill4Action, ETriggerEvent::Started, this, &AS1MyPlayer::UseSkillSlot4);
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
		SetState(Protocol::STATE_MACHINE_MOVING);
		SendMovePacket();
		TimeSinceLastSend = 0.f;
	}

	// 멈춘 직후 한번 전송 (Idle 처리)
	if (DirtyFlag && InputVector.IsNearlyZero())
	{
		SetState(Protocol::STATE_MACHINE_IDLE);
		SendMovePacket();
		DirtyFlag = false;
		TimeSinceLastSend = 0.f;
	}

	TimeSinceLastSkill += DeltaTime;
}

void AS1MyPlayer::InputMove(const FInputActionValue& Value)
{
	// input is a Vector2D
	CacheVector = InputVector;
	InputVector = Value.Get<FVector2D>();
}

void AS1MyPlayer::InputLook(const FInputActionValue& Value)
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

void AS1MyPlayer::UseSkillSlot(int32 SlotIndex)
{
	int32 SkillID = LoadoutComponent->GetSkillSlot(SlotIndex);
	if (SkillID <= 0)
		return;

	if (SkillBar->CanUseSkill(SlotIndex))
	{
		UE_LOG(LogTemp, Log, TEXT("Use Skill ID: %d from Slot %d"), SkillID, SlotIndex);

		auto SkillIt = S1DataManager::Instance().SkillDict.find(SkillID);
		if (SkillIt == S1DataManager::Instance().SkillDict.end())
			return;

		Skill& SkillData = SkillIt->second;
		
		C_SKILL SkillPkt;
		SkillPkt.mutable_info()->set_skillid(SkillID);

		SEND_PACKET(SkillPkt);
		// TODO : 애니메이션 & S_Skill 패킷 받으면 SkillBar에 Cooldown 적용
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Skill Is Cooldown. ID: %d from Slot %d"), SkillID, SlotIndex);
	}
}

void AS1MyPlayer::UseSkillSlot1()
{
	UseSkillSlot(0);
}

void AS1MyPlayer::UseSkillSlot2()
{
	UseSkillSlot(1);
}

void AS1MyPlayer::UseSkillSlot3()
{
	UseSkillSlot(2);
}

void AS1MyPlayer::UseSkillSlot4()
{
	UseSkillSlot(3);
}

void AS1MyPlayer::InitSkillBar()
{
	if (!LoadoutComponent)
		return;

	TMap<int32, int32> DefaultSlotMap;
	const int32 MaxSlots = 4;
	for (int32 i = 0; i < MaxSlots; i++)
	{
		int32 SkillId = i + 1;

		if (S1DataManager::Instance().SkillDict.find(SkillId) != S1DataManager::Instance().SkillDict.end())
		{
			DefaultSlotMap.Add(i, SkillId);
		}
	}

	LoadoutComponent->InitializeDefaultSkills(DefaultSlotMap);

	for (int32 i = 0; i < MaxSlots; i++)
	{
		int32 SkillId = LoadoutComponent->GetSkillSlot(i);
		if (SkillId <= 0)
			continue;

		auto SkillIt = S1DataManager::Instance().SkillDict.find(SkillId);
		if (SkillIt == S1DataManager::Instance().SkillDict.end())
			continue;

		auto& Skill = SkillIt->second;

		UTexture2D* IconTexture = nullptr;
		if (!Skill.iconPath.empty())
		{
			FString IconPathFStr(Skill.iconPath.c_str());
			FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), IconPathFStr);
			IconTexture = UKismetRenderingLibrary::ImportFileAsTexture2D(GetWorld(), FullPath);
		}

		// 입력키 세팅: 1~4, 별도 InputMapping에서 가져올 수도 있음
		FText KeyText = FText::AsNumber(i + 1);
		SkillBar->SetSkillSlot(i, IconTexture, KeyText);
	}
}

void AS1MyPlayer::BindSkillBar(US1SkillBar* InSkillBar)
{
	SkillBar = InSkillBar;
}

void AS1MyPlayer::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AS1PlayerController* PC = Cast<AS1PlayerController>(NewController))
	{
		if (PC->IsLocalController())
		{
			// 한 틱 뒤에 시도 (Input 컴포넌트/서브시스템 안정화 후)
			GetWorld()->GetTimerManager().SetTimerForNextTick([this, PC]()
				{
					TrySetupInput(PC);
				});
		}
	}
}

void AS1MyPlayer::TrySetupInput(AS1PlayerController* PC)
{
	if (ULocalPlayer* LP = PC->GetLocalPlayer())
	{
		if (auto* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsys->ClearAllMappings();
			if (DefaultMappingContext)
				Subsys->AddMappingContext(DefaultMappingContext, 0);

			if (GEngine && IsLocallyControlled())
			{
				GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Blue,
					TEXT("Input Mapping Added (Local)"));
			}
		}
	}
	else
	{
		// 아직이면 다음 틱에 재시도
		GetWorldTimerManager().SetTimerForNextTick([this, PC]() { TrySetupInput(PC); });
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

	float Speed = GetVelocity().Size();
	PosInfo.set_speed(Speed);

	Info->CopyFrom(PosInfo);

	SEND_PACKET(MovePkt);

	DirtyFlag = false;
}
