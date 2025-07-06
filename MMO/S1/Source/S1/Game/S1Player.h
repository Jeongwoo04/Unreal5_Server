// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Protocol.pb.h"
#include "S1Player.generated.h"

UCLASS()
class S1_API AS1Player : public ACharacter
{
	GENERATED_BODY()

public:
	AS1Player();
	virtual ~AS1Player();

protected:
	virtual void BeginPlay();
	virtual void Tick(float DeltaSeconds) override;

public:
	bool IsMyPlayer();

	Protocol::MoveState GetMoveState() { return PlayerInfo.state(); }
	void SetMoveState(Protocol::MoveState State);

public:
	void SetPlayerInfo(const Protocol::PlayerInfo& Info);
	void SetDestInfo(const Protocol::PlayerInfo& Info);
	Protocol::PlayerInfo GetPlayerInfo() { return PlayerInfo; }

protected:
	class Protocol::PlayerInfo PlayerInfo; // 현재 위치
	class Protocol::PlayerInfo DestInfo; // 목적지
};
