// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/S1PlayerController.h"
#include "S1GameInstance.h"
#include "Blueprint/UserWidget.h"
#include "S1MyPlayer.h"

void AS1PlayerController::BeginPlay() {
	Super::BeginPlay();

	// 1) GameInstance 델리게이트 구독
	if (US1GameInstance* GI = GetGameInstance<US1GameInstance>())
	{
		GI->OnMyPlayerSpawned.AddUObject(this, &AS1PlayerController::HandleMyPlayerSpawned);
		// 이미 스폰돼 있었다면 즉시 처리
		if (GI->MyPlayer) HandleMyPlayerSpawned(GI->MyPlayer);
	}

	if (WBP_SkillBarClass) // TSubclassOf<UUserWidget>
	{
		UUserWidget* SkillBar = CreateWidget(this, WBP_SkillBarClass);
		if (SkillBar)
		{
			SkillBar->AddToViewport();
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
	// 로컬 컨트롤러만 Possess
	if (!IsLocalController())
		return;
	// 이미 다른 컨트롤러가 잡고 있지 않다면 Possess
	if (!SpawnedMyPlayer->GetController())
	{
		Possess(SpawnedMyPlayer);
		// (선택) 카메라 초기 회전 맞추고 싶으면 여기서 SetControlRotation 등 처리
		// SetControlRotation(...)
	}
}