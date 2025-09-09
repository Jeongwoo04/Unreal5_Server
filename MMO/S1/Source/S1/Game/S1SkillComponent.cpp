// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/S1SkillComponent.h"
#include "S1MyPlayer.h"
#include "S1MarkerActor.h"
#include "S1PlayerController.h"
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
	CachedPlayer = Cast<AS1MyPlayer>(GetOwner());
	if (!CachedPlayer)
		return;
}


// Called every frame
void US1SkillComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void US1SkillComponent::BeginSkillTargeting(int32 SkillID, float Distance, float Range)
{
	ClearSkillMarkers();

	if (!CachedPlayer)
		return;

	CurrentTargetingSkillID = SkillID;
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

void US1SkillComponent::ConfirmSkillTargeting()
{
	if (!bIsSkillTargeting)
		return;

	bIsSkillTargeting = false;

	// TODO : 스킬 사용 -> 캐스팅 State 추가 후 적용 -> 패킷 전송
	C_SKILL SkillPkt;
	SkillPkt.mutable_info()->set_skillid(CurrentTargetingSkillID);

	SEND_PACKET(SkillPkt);

	ClearSkillMarkers();
}

bool US1SkillComponent::IsSkillTargeting()
{
	return bIsSkillTargeting;
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
	CurrentTargetingSkillID = -1;
	CurrentSkillDistance = 0.f;
}

