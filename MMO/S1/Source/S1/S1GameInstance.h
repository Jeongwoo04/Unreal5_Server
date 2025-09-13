// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "S1.h"
#include "S1GameInstance.generated.h"

class AS1Player;
class AS1Monster;
class AS1MyPlayer;

class AS1Projectile;
class US1MapManager;
class US1ObjectManager;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMyPlayerSpawned, AS1MyPlayer*);

/**
 * 
 */
UCLASS()
class S1_API US1GameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	UFUNCTION(BlueprintCallable)
	void ConnectToGameServer();

	UFUNCTION(BlueprintCallable)
	void DisconnectFromGameServer();

	UFUNCTION(BlueprintCallable)
	void HandleRecvPackets();

	void SendPacket(SendBufferRef SendBuffer);

	FOnMyPlayerSpawned OnMyPlayerSpawned;

public:
	void HandleSpawn(const Protocol::ObjectInfo& ObjectInfo, bool IsMine);
	void HandleSpawn(const Protocol::S_ENTER_GAME& EnterGamePkt);
	void HandleSpawn(const Protocol::S_SPAWN& SpawnPkt);

	void HandleDespawn(uint64 ObjectId);
	void HandleDespawn(const Protocol::S_DESPAWN& DespawnPkt);

	void HandleMove(const Protocol::S_MOVE& MovePkt);

	void HandleSkill(const Protocol::S_SKILL& SkillPkt);

	void HandleChangeHp(const Protocol::S_CHANGE_HP& ChangeHpPkt);

	void HandleDie(const Protocol::S_DIE& DiePkt);

public:
	// GameServer
	class FSocket* Socket;
	FString IpAddress = TEXT("127.0.0.1");
	int16 Port = 7777;
	TSharedPtr<class PacketSession> GameServerSession;

public:
	UPROPERTY(BlueprintReadOnly)
	US1MapManager* MapManager;

	UPROPERTY()
	US1ObjectManager* ObjectManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Managers")
	TSubclassOf<US1ObjectManager> BP_ObjectManagerClass;

public:
	UPROPERTY(BlueprintReadOnly)
	AS1MyPlayer* MyPlayer = nullptr;

	bool Connected = false;
};
