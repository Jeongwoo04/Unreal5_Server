// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/S1PlayerController.h"
#include "S1GameInstance.h"
#include "S1MyPlayer.h"
#include "S1SkillBar.h"

void AS1PlayerController::BeginPlay() {
	Super::BeginPlay();

	// 1) GameInstance 델리게이트 구독
	if (US1GameInstance* GI = GetGameInstance<US1GameInstance>())
	{
		GI->OnMyPlayerSpawned.AddUObject(this, &AS1PlayerController::HandleMyPlayerSpawned);
		// 이미 스폰돼 있었다면 즉시 처리
		if (GI->MyPlayer)
		{
			HandleMyPlayerSpawned(GI->MyPlayer);
		}
	}

	// 2) LocalPlayer가 완전히 붙은 뒤 서버 연결 (다음 틱부터 체크)
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AS1PlayerController::TryConnectAfterLocalReady);
}

void AS1PlayerController::TryConnectAfterLocalReady()
{
	// LocalPlayer가 준비될 때까지 다음 틱에 재시도
	if (!GetLocalPlayer())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AS1PlayerController::TryConnectAfterLocalReady);
		return;
	}
}

void AS1PlayerController::HandleMyPlayerSpawned(AS1MyPlayer* SpawnedMyPlayer)
{
	if (!SpawnedMyPlayer)
		return;

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
			FString::Printf(TEXT("HandleMyPlayerSpawned called for %s"), *SpawnedMyPlayer->GetName()));
	// 로컬 컨트롤러만 Possess
	if (!IsLocalController())
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, TEXT("HandleMyPlayerSpawned: Not local controller"));
		return;
	}

	// 이미 다른 컨트롤러가 잡고 있지 않다면 Possess
	if (!SpawnedMyPlayer->GetController())
	{
		Possess(SpawnedMyPlayer);
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Possessed %s"), *SpawnedMyPlayer->GetName()));

		SetViewTarget(SpawnedMyPlayer);
		CreateSkillBarWidget();
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, TEXT("SkillBarWidget created"));
		SpawnedMyPlayer->BindSkillBar(Cast<US1SkillBar>(SkillBarWidget));
		SpawnedMyPlayer->InitSkillBar();

		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, TEXT("SkillBar bound and initialized"));

		bShowMouseCursor = true;
	}
	else
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
				FString::Printf(TEXT("Already possessed by %s"), *SpawnedMyPlayer->GetController()->GetName()));
	}
}

void AS1PlayerController::CreateSkillBarWidget()
{
	if (WBP_SkillBarClass) // TSubclassOf<UUserWidget>
	{
		SkillBarWidget = CreateWidget(this, WBP_SkillBarClass);
		if (SkillBarWidget)
		{
			SkillBarWidget->AddToViewport();
		}
	}
}