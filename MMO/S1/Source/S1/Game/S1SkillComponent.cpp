// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/S1SkillComponent.h"
#include "S1Creature.h"
#include "S1MyPlayer.h"
#include "S1MarkerActor.h"
#include "S1PlayerController.h"
#include "Data/S1DataManager.h"
#include "S1SkillBar.h"
#include "S1CastingBar.h"
#include "TimerManager.h"
#include "Protocol.pb.h"
#include "S1.h"

using namespace Protocol;

// Sets default values for this component's properties
US1SkillComponent::US1SkillComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void US1SkillComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	OwnerCreature = Cast<AS1Creature>(GetOwner());
	CachedPlayer = Cast<AS1MyPlayer>(GetOwner());
}


// Called every frame
void US1SkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CachedPlayer && CurrentSkillState.SkillID > 0)
	{
		TickSkillState(DeltaTime);
	}
}

void US1SkillComponent::BeginSkillTargeting(int32 SkillID, float Distance, float Range)
{
	ClearSkillMarkers();

	if (!CachedPlayer)
		return;

	CurrentSkillID = SkillID;
	bIsSkillTargeting = true;
	CurrentSkillDistance = Distance;
	CurrentSkillRange = Range;

	// 캐릭터 중심 사거리 마커
	if (SkillRangeMaterial)
	{
		SkillRangeMarker = GetWorld()->SpawnActor<AS1MarkerActor>(
			AS1MarkerActor::StaticClass(),
			CachedPlayer->GetActorLocation(),
			FRotator::ZeroRotator
		);

		if (SkillRangeMarker)
		{
			SkillRangeMarker->Init(SkillRangeMaterial, FVector(Distance, Distance, Distance), 0.f);
			SkillRangeMarker->SetFollowActor(CachedPlayer);
		}
	}

	// 마우스 따라다니는 범위 마커
	if (SkillAreaMaterial)
	{
		FVector SpawnLoc = CachedPlayer->GetActorLocation();

		APlayerController* PC = Cast<AS1PlayerController>(CachedPlayer->GetController());
		if (PC)
		{
			FVector MouseWorld, MouseDir;
			if (PC->DeprojectMousePositionToWorld(MouseWorld, MouseDir))
			{
				FHitResult Hit;
				FVector Start = MouseWorld;
				FVector End = MouseWorld + MouseDir * 10000.f;

				// WorldStatic만 충돌
				if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic))
				{
					FVector Target = Hit.Location;
					FVector OwnerPos = CachedPlayer->GetActorLocation();

					FVector Dir = (Target - OwnerPos).GetSafeNormal();
					float Dist = FVector::Dist(OwnerPos, Target);

					if (Dist > CurrentSkillDistance)
						Target = OwnerPos + Dir * CurrentSkillDistance;

					SpawnLoc = Target + FVector(0, 0, 2.f);
				}
			}
		}

		SkillAreaMarker = GetWorld()->SpawnActor<AS1MarkerActor>(
			AS1MarkerActor::StaticClass(),
			SpawnLoc,
			FRotator::ZeroRotator
		);

		if (SkillAreaMarker)
		{
			SkillAreaMarker->Init(SkillAreaMaterial, FVector(Range, Range, Range), 0.f);
		}
	}

	// 타이머 시작
	GetWorld()->GetTimerManager().SetTimer(
		SkillAreaUpdateTimer,
		this,
		&US1SkillComponent::UpdateSkillMarkers,
		SkillAreaUpdateInterval,
		true
	);
}

void US1SkillComponent::CancelSkillTargeting()
{
	bIsSkillTargeting = false;
	ClearSkillMarkers();
}

void US1SkillComponent::ConfirmSkillTargeting(int32 SlotIndex)
{
	if (!bIsSkillTargeting || !CachedPlayer)
		return;

	bIsSkillTargeting = false;

	CurrentSkillState = FSkillState();
	CurrentSkillState.SkillID = CurrentSkillID;
	CurrentSkillState.SlotIndex = SlotIndex;
	CurrentSkillState.TargetPos = SkillAreaMarker ? SkillAreaMarker->GetActorLocation() : CachedPlayer->GetActorLocation();
	CurrentSkillState.CastID = CachedPlayer->GetNextCastId(); // 클라이언트 고유 CastId

	// TODO: CastTime 설정 필요
	auto SkillIt = S1DataManager::Instance().SkillDict.find(CurrentSkillID);
	if (SkillIt != S1DataManager::Instance().SkillDict.end())
	{
		CurrentSkillState.name = SkillIt->second.name.c_str();
		CurrentSkillState.CastTime = SkillIt->second.castTime;
		CurrentSkillState.bIsCasting = CurrentSkillState.CastTime > 0.f;
		CurrentSkillState.CooldownDuration = SkillIt->second.cooldown;

		// ActionInstances 초기화
		CurrentSkillState.ActionInstances.Empty();
		for (const auto& Action : SkillIt->second.actions)
		{
			FClientActionInstance Instance;
			Instance.Action = &Action;
			Instance.Elapsed = 0.f;
			Instance.bTriggered = false;
			CurrentSkillState.ActionInstances.Add(Instance);
		}
	}

	if (CurrentSkillState.bIsCasting)
	{
		// 캐스팅 시작
		CachedPlayer->StartCasting(CurrentSkillState);
		CachedPlayer->ChangeState(Protocol::STATE_MACHINE_CASTING);
		// TODO : 액션 시작
	}
	else
	{
		// 즉발이면 바로 액션 Tick 수행
		CurrentSkillState.bIsCasting = false;
		CurrentSkillState.CurrentActionIndex = 0;
		// TODO : 액션 시작
	}

	// TODO : 서버 패킷 전송 -> 액션 핸들링쪽으로 이동
	uint64 ClientNowTick = static_cast<uint64>(FPlatformTime::Seconds() * 1000);
	CurrentSkillState.ClientSendTick = ClientNowTick;

	C_SKILL SkillPkt;
	SkillPkt.mutable_skill_info()->set_skillid(CurrentSkillState.SkillID);
	SkillPkt.mutable_skill_info()->mutable_targetpos()->set_x(CurrentSkillState.TargetPos.X);
	SkillPkt.mutable_skill_info()->mutable_targetpos()->set_y(CurrentSkillState.TargetPos.Y);
	SkillPkt.mutable_skill_info()->mutable_targetpos()->set_z(CurrentSkillState.TargetPos.Z);
	SkillPkt.set_castid(CurrentSkillState.CastID);
	SkillPkt.set_clientsend(ClientNowTick);
	
	SEND_PACKET(SkillPkt);

	ClearSkillMarkers();
}

bool US1SkillComponent::IsSkillTargeting()
{
	return bIsSkillTargeting;
}

void US1SkillComponent::StartCasting(const FSkillState& SkillState)
{
	if (OwnerCreature)
	{
		CurrentSkillState = SkillState;
		OwnerCreature->ChangeState(Protocol::STATE_MACHINE_CASTING);

		// 캐스팅 애니메이션/이펙트 처리
		//OwnerCreature->PlayCastingAnimation(SkillState.SkillID);
	}

	// MyPlayer일 경우 캐스팅바 표시
	if (CachedPlayer)
	{
		CachedPlayer->StartCasting(SkillState);
	}
}

void US1SkillComponent::CancelCasting()
{
	CurrentSkillState = FSkillState();
}

void US1SkillComponent::FinishCasting()
{

}

void US1SkillComponent::HandleActionPkt(const Protocol::S_SKILL& Pkt)
{
	auto it = S1DataManager::Instance().SkillDict.find(Pkt.skill_info().skillid());
	if (it == S1DataManager::Instance().SkillDict.end())
		return;

	const ClientAction& Action = it->second.actions[Pkt.skill_info().actionindex()];
	
	const FVector& TargetPos = { Pkt.skill_info().targetpos().x(),Pkt.skill_info().targetpos().y(), Pkt.skill_info().targetpos().z() };
	ExecuteAction(Action, TargetPos);
}

void US1SkillComponent::UpdateSkillMarkers()
{
	UpdateSkillAreaMarker();
	UpdateSkillRangeMarker();
}

void US1SkillComponent::UpdateSkillAreaMarker()
{
	if (!SkillAreaMarker || !bIsSkillTargeting || !CachedPlayer)
		return;

	AS1PlayerController* PC = Cast<AS1PlayerController>(CachedPlayer->GetController());
	if (!PC) return;

	FVector MouseWorld, MouseDir;
	if (PC->DeprojectMousePositionToWorld(MouseWorld, MouseDir))
	{
		FHitResult Hit;
		FVector Start = MouseWorld;
		FVector End = MouseWorld + MouseDir * 10000.f;

		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic))
		{
			FVector Target = Hit.Location;
			FVector OwnerPos = CachedPlayer->GetActorLocation();

			// 사거리 제한
			FVector Dir = (Target - OwnerPos).GetSafeNormal();
			float Dist = FVector::Dist(OwnerPos, Target);

			if (Dist > CurrentSkillDistance)
				Target = OwnerPos + Dir * CurrentSkillDistance;

			SkillAreaMarker->SetActorLocation(Target + FVector(0, 0, 2.f));
		}
	}
}

void US1SkillComponent::UpdateSkillRangeMarker()
{
	if (!SkillRangeMarker || !bIsSkillTargeting || !CachedPlayer)
		return;

	// 캐릭터 위치 따라가기
	FVector PlayerLocation = CachedPlayer->GetActorLocation();
	SkillRangeMarker->SetActorLocation(PlayerLocation);
}

void US1SkillComponent::ClearSkillMarkers()
{
	GetWorld()->GetTimerManager().ClearTimer(SkillAreaUpdateTimer);

	if (SkillRangeMarker)
	{
		SkillRangeMarker->Destroy();
		SkillRangeMarker = nullptr;
	}

	if (SkillAreaMarker)
	{
		SkillAreaMarker->Destroy();
		SkillAreaMarker = nullptr;
	}

	bIsSkillTargeting = false;
	CurrentSkillID = -1;
	CurrentSkillDistance = 0.f;
}

void US1SkillComponent::TickSkillState(float DeltaTime)
{
	if (CurrentSkillState.SkillID <= 0 || !CachedPlayer)
		return;

	// 캐스팅 진행
	if (CurrentSkillState.bIsCasting)
	{
		CurrentSkillState.CastElapsed += DeltaTime;
		if (CurrentSkillState.CastElapsed >= CurrentSkillState.CastTime)
		{
			CurrentSkillState.bIsCasting = false;
			CurrentSkillState.CurrentActionIndex = 0;
		}
	}
	else
	{
		// Action 순차 실행
		if (CurrentSkillState.CurrentActionIndex < CurrentSkillState.ActionInstances.Num())
		{
			FClientActionInstance& ActionInst = CurrentSkillState.ActionInstances[CurrentSkillState.CurrentActionIndex];
			ActionInst.Elapsed += DeltaTime;

			if (!ActionInst.bTriggered && ActionInst.Elapsed >= ActionInst.Action->actionDelay)
			{
				HandleExecuteAction(ActionInst);
				ActionInst.bTriggered = true;
			}

			// 다음 액션으로 이동
			if (ActionInst.bTriggered)
			{
				CurrentSkillState.CurrentActionIndex++;
			}
		}
		else
		{
			// 스킬 종료
			CurrentSkillState = FSkillState();
		}
	}
}

void US1SkillComponent::HandleExecuteAction(FClientActionInstance& ActionInstance)
{
	if (!ActionInstance.Action || !CachedPlayer)
		return;

	// 이미 실행된 액션이면 스킵
	if (ActionInstance.bTriggered)
		return;

	const ClientAction& Action = *ActionInstance.Action;
	FVector TargetPos = CurrentSkillState.TargetPos;

	ExecuteAction(Action, TargetPos);

	// 액션 완료 처리
	ActionInstance.bTriggered = true;
	CurrentSkillState.CurrentActionIndex++;
}

void US1SkillComponent::ExecuteAction(const ClientAction& Action, const FVector& TargetPos)
{
	// 실제 실행
	switch (Action.actionType)
	{
	case ClientActionType::PlayAnimation:
		//CachedPlayer->PlayAnimMontage(Action.animName);
		break;

	case ClientActionType::PlayEffect:
		//CachedPlayer->SpawnEffect(Action.effectName, Action.attachBone);
		break;

	case ClientActionType::SpawnProjectile:
		// 클라에서는 미리보기용 Spawn만 처리
		//CachedPlayer->SpawnPreviewProjectile(Action.dataId, TargetPos);
		break;

	case ClientActionType::SpawnField:
		//CachedPlayer->SpawnPreviewField(Action.dataId, TargetPos);
		break;

	case ClientActionType::Move:
		//CachedPlayer->MoveSkill(Action.moveDistance, TargetPos);
		break;

	default:
		UE_LOG(LogTemp, Warning, TEXT("Unknown ClientActionType in ExecuteAction"));
		break;
	}
}