// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "S1PlayerController.generated.h"

class AS1MyPlayer;
/**
 * 
 */
UCLASS()
class S1_API AS1PlayerController : public APlayerController
{
	GENERATED_BODY()

	virtual void BeginPlay() override;

	void TryConnectAfterLocalReady();
	void HandleMyPlayerSpawned(AS1MyPlayer* SpawnedMyPlayer);
};
