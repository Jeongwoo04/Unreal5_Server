// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/S1MyPlayer.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "S1.h"
#include "S1PlayerController.h"
#include "S1LoadoutComponent.h"
#include "S1MarkerActor.h"
#include "S1SkillComponent.h"
#include "S1SkillBar.h"
#include "Data/S1DataManager.h"

#include <NavigationSystem.h>
#include "Kismet/KismetRenderingLibrary.h"

AS1MyPlayer::AS1MyPlayer()
{
	CameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CameraRoot"));
	CameraRoot->SetupAttachment(RootComponent);
	
	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(CameraRoot);
	CameraBoom->TargetArmLength = 800.0f; // The camera follows at this distance behind the character	
	CameraBoom->SetRelativeRotation(FRotator(-50.f, 0.f, 0.f));

	CameraBoom->bDoCollisionTest = false;

	CameraBoom->bUsePawnControlRotation = false;   // Pawn 회전 무시
	CameraBoom->bInheritPitch = false;             // 부모 회전 무시
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
	LoadoutComponent = CreateDefaultSubobject<US1LoadoutComponent>(TEXT("LoadoutComponent"));
	SkillComponent = CreateDefaultSubobject<US1SkillComponent>(TEXT("SkillComponent"));
}

void AS1MyPlayer::BeginPlay()
{
	Super::BeginPlay();

	NextMoveLocation = GetActorLocation();
}

//////////////////////////////////////////////////////////////////////////
// Input

void AS1MyPlayer::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {

		//Moving
		EnhancedInputComponent->BindAction(RightClickMoveAction, ETriggerEvent::Started, this, &AS1MyPlayer::InputRightClickMove);
		
		//Skill
		EnhancedInputComponent->BindAction(Skill1Action, ETriggerEvent::Started, this, &AS1MyPlayer::OnSkillSlot1Pressed);
		EnhancedInputComponent->BindAction(Skill1Action, ETriggerEvent::Completed, this, &AS1MyPlayer::OnSkillSlot1Released);
		EnhancedInputComponent->BindAction(Skill2Action, ETriggerEvent::Started, this, &AS1MyPlayer::OnSkillSlot2Pressed);
		EnhancedInputComponent->BindAction(Skill2Action, ETriggerEvent::Completed, this, &AS1MyPlayer::OnSkillSlot2Released);
		EnhancedInputComponent->BindAction(Skill3Action, ETriggerEvent::Started, this, &AS1MyPlayer::OnSkillSlot3Pressed);
		EnhancedInputComponent->BindAction(Skill3Action, ETriggerEvent::Completed, this, &AS1MyPlayer::OnSkillSlot3Released);
		EnhancedInputComponent->BindAction(Skill4Action, ETriggerEvent::Started, this, &AS1MyPlayer::OnSkillSlot4Pressed);
		EnhancedInputComponent->BindAction(Skill4Action, ETriggerEvent::Completed, this, &AS1MyPlayer::OnSkillSlot4Released);
	}
}

// MOVE_PACKET_SEND_DELAY 마다 처리를하게 될 경우. State가 변할때 대응이 안됨.
void AS1MyPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AS1MyPlayer::UpdateIdle(float DeltaTime)
{

}

void AS1MyPlayer::UpdateMoving(float DeltaTime)
{
	if (!DirtyFlag)
		return;

	if (TargetLocation.IsZero())
		return;

	if (SkillComponent && SkillComponent->IsCasting())
		HandleLocalCancelCasting();

	FVector CurrentLocation = GetActorLocation();
	FVector ToTarget = TargetLocation - CurrentLocation;

	// 2D 이동: Z 고정
	FVector2D ToTarget2D(ToTarget.X, ToTarget.Y);
	float Dist2D = ToTarget2D.Size();

	if (Dist2D < KINDA_SMALL_NUMBER)
	{
		// 이미 거의 도착
		NextMoveLocation = FVector(TargetLocation.X, TargetLocation.Y, CurrentLocation.Z);
		SetActorLocation(FVector(TargetLocation.X, TargetLocation.Y, CurrentLocation.Z));
		ChangeState(Protocol::STATE_MACHINE_IDLE);
		SendMovePacket();
		DirtyFlag = false;
		TimeSinceLastSend = 0.f;
		return;
	}

	// 일정 속도로 이동
	float MoveDt = PosInfo.speed() * DeltaTime;
	FVector2D Dir2D = ToTarget2D.GetSafeNormal();
	FVector2D Step2D = Dir2D * MoveDt;

	// MoveDt가 남은 거리보다 크면, 목표 위치를 그대로 보간
	if (MoveDt >= Dist2D)
	{
		Step2D *= Dist2D / MoveDt; // 속도 유지하면서 마지막 이동량 보정
	}

	// 다음 위치 계산
	NextMoveLocation.X += Step2D.X;
	NextMoveLocation.Y += Step2D.Y;
	NextMoveLocation.Z = CurrentLocation.Z; // Z는 고정

	SetActorLocation(NextMoveLocation);

	TimeSinceLastSend += DeltaTime;
	if (TimeSinceLastSend >= MoveSendInterval)
	{
		ChangeState(Protocol::STATE_MACHINE_MOVING);
		SendMovePacket();
		TimeSinceLastSend = 0.f;
	}

	// 도착 체크
	FVector2D NewToTarget2D(TargetLocation.X - NextMoveLocation.X, TargetLocation.Y - NextMoveLocation.Y);
	if (NewToTarget2D.Size() < KINDA_SMALL_NUMBER)
	{
		NextMoveLocation = FVector(TargetLocation.X, TargetLocation.Y, CurrentLocation.Z);
		SetActorLocation(FVector(TargetLocation.X, TargetLocation.Y, CurrentLocation.Z));
		ChangeState(Protocol::STATE_MACHINE_IDLE);
		SendMovePacket();
		DirtyFlag = false;
		TimeSinceLastSend = 0.f;
	}
}

void AS1MyPlayer::InputRightClickMove(const FInputActionValue& value)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
		return;

	FHitResult Hit;
	if (PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
		if (NavSys)
		{
			FNavLocation ResultLocation; // NavMesh 위에서 가장 가까운 위치 찾기
			if (NavSys->ProjectPointToNavigation(Hit.Location, ResultLocation, FVector(50.f, 50.f, 50.f)))
			{
				SpawnClickMarker(ResultLocation.Location);

				TargetLocation = ResultLocation.Location;
				DirtyFlag = true;
				FVector Direction = (TargetLocation - GetActorLocation());
				Direction.Z = 0;
				// 수직 성분 제거
				if (!Direction.IsNearlyZero())
				{
					FRotator NewRot = Direction.Rotation();
					SetActorRotation(NewRot); // PosInfo에도 Yaw 저장
					PosInfo.set_yaw(NewRot.Yaw);

					if (PosInfo.state() == Protocol::STATE_MACHINE_CASTING)
					{
						HandleLocalCancelCasting();
					}
				}
				// 이동 시작하면 서버로 패킷 전송
				ChangeState(Protocol::STATE_MACHINE_MOVING);
				SendMovePacket();
			}
		}
	}
}

void AS1MyPlayer::OnSkillSlotPressed(int32 SlotIndex)
{
	int32 SkillID = LoadoutComponent->GetSkillIdWithSlot(SlotIndex);
	if (SkillID <= 0)
		return;

	if (SkillComponent->CanUseSkill(SkillID))
	{
		auto SkillIt = S1DataManager::Instance().SkillDict.find(SkillID);
		if (SkillIt == S1DataManager::Instance().SkillDict.end())
			return;

		const Skill& SkillData = SkillIt->second;

		if (SkillData.marker)
			SkillComponent->BeginSkillTargeting(SkillID, SkillData.markerData.distance, SkillData.markerData.range);
		else
			;
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Skill Is Cooldown. ID: %d from Slot %d"), SkillID, SlotIndex);
	}
}

void AS1MyPlayer::OnSkillSlotReleased(int32 SlotIndex)
{
	if (SkillComponent && SkillComponent->IsSkillTargeting())
	{
		int32 SkillID = LoadoutComponent->GetSkillIdWithSlot(SlotIndex);
		SkillComponent->ConfirmSkillTargeting(SkillID);
	}
}

void AS1MyPlayer::OnSkillSlot1Pressed() { OnSkillSlotPressed(0); }
void AS1MyPlayer::OnSkillSlot2Pressed() { OnSkillSlotPressed(1); }
void AS1MyPlayer::OnSkillSlot3Pressed() { OnSkillSlotPressed(2); }
void AS1MyPlayer::OnSkillSlot4Pressed() { OnSkillSlotPressed(3); }

void AS1MyPlayer::OnSkillSlot1Released() { OnSkillSlotReleased(0); }
void AS1MyPlayer::OnSkillSlot2Released() { OnSkillSlotReleased(1); }
void AS1MyPlayer::OnSkillSlot3Released() { OnSkillSlotReleased(2); }
void AS1MyPlayer::OnSkillSlot4Released() { OnSkillSlotReleased(3); }

void AS1MyPlayer::SpawnClickMarker(const FVector& Location)
{
	if (!ClickMarkerMaterial)
		return;

	if (ClickMarker)
	{
		ClickMarker->Destroy();
		ClickMarker = nullptr;
	}

	ClickMarker = GetWorld()->SpawnActor<AS1MarkerActor>(
		AS1MarkerActor::StaticClass(),
		Location + FVector(0, 0, 2.f),
		FRotator::ZeroRotator
	);

	if (ClickMarker)
		ClickMarker->Init(ClickMarkerMaterial, FVector(16.f, 16.f, 16.f), 0.8f);
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
		int32 SkillId = LoadoutComponent->GetSkillIdWithSlot(i);
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

		if (SkillComponent)
		{
			FSkillState State;
			State.SkillID = SkillId;
			State.name = Skill.name.c_str();
			State.CastStartTick = 0;
			State.CastDuration = static_cast<uint64>(Skill.castTime * 1000);
			State.CooldownDuration = Skill.cooldown;
			State.bIsCooldown = false;

			State.ActionInstances.Empty();
			for (const auto& Action : SkillIt->second.actions)
			{
				FClientActionInstance Instance;
				Instance.Action = &Action;
				Instance.Elapsed = 0.f;
				Instance.bTriggered = false;
				State.ActionInstances.Add(Instance);
			}

			SkillComponent->AddSkillState(SkillId, State);
		}
	}
}

void AS1MyPlayer::BindSkillBar(US1SkillBar* InSkillBar)
{
	SkillBar = InSkillBar;
}

void AS1MyPlayer::HandleStartLocalCasting(const FSkillState& State)
{
	SkillBar->StartCastingBar(State, 0);
}

void AS1MyPlayer::HandleStartServerCasting(const FSkillState& State, uint64 ServerCastEndTick)
{
	SkillBar->StartCastingBar(State, ServerCastEndTick);
}

void AS1MyPlayer::HandleLocalCancelCasting()
{
	SkillComponent->LocalCancelCasting();
	SkillBar->HideCastingBar();
}

void AS1MyPlayer::HandleServerCancelCasting(int32 SkillID)
{
	SkillComponent->ServerCancelCasting(SkillID);
	SkillBar->HideCastingBar();
}

void AS1MyPlayer::HandleServerFinishCasting(int32 SkillID, uint64 CooldownRemainTick)
{
	FSkillState* State = SkillComponent->GetSkillState(SkillID);

	{
		State->bIsCasting = false;
		State->CastStartTick = 0;
		State->bIsCooldown = true;
		State->CooldownStartTick = static_cast<uint64>(FPlatformTime::Seconds() * 1000);
		State->CooldownDuration = CooldownRemainTick;
	}

	SkillBar->HideCastingBar();
	SkillBar->StartSlotCooldown(
		LoadoutComponent->GetSkillSlotWithId(SkillID), CooldownRemainTick);
	//State->CooldownDuration = ;
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
	FRotator Rot = GetActorRotation();

	Protocol::C_MOVE MovePkt;
	auto Info = MovePkt.mutable_info();

	PosInfo.set_object_id(PosInfo.object_id()); // 기존 ID 유지
	PosInfo.set_x(NextMoveLocation.X);
	PosInfo.set_y(NextMoveLocation.Y);
	PosInfo.set_z(NextMoveLocation.Z);
	PosInfo.set_yaw(Rot.Yaw);

	Info->CopyFrom(PosInfo);

	SEND_PACKET(MovePkt);
	UE_LOG(LogTemp, Log, TEXT("SendMovePacket Loc: %s"), *NextMoveLocation.ToString());
}
