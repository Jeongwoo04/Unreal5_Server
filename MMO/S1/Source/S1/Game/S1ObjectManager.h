// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Protocol.pb.h"
#include "UObject/NoExportTypes.h"
#include "S1ObjectManager.generated.h"

/**
 * 
 */
class AS1Player;
class AS1Monster;
class AS1MyPlayer;

USTRUCT()
struct FS1ObjectKey
{
	GENERATED_BODY()

	int32 MainType;
	int32 SubType;

	bool operator==(const FS1ObjectKey& Other) const
	{
		return MainType == Other.MainType && SubType == Other.SubType;
	}
};

USTRUCT()
struct FS1ObjectEntry
{
	GENERATED_BODY()

	FS1ObjectKey Key;
	AActor* Actor;
};

FORCEINLINE uint32 GetTypeHash(const FS1ObjectKey& Key)
{
	return HashCombine(::GetTypeHash(Key.MainType), ::GetTypeHash(Key.SubType));
}

UCLASS()
class S1_API US1ObjectManager : public UObject
{
	GENERATED_BODY()

public:
	using CreateFunc = TFunction<AActor*(UWorld*, const Protocol::ObjectInfo&, bool)>;
	using DestroyFunc = TFunction<void(AActor*, uint64)>;
	using TypeConstructor = TFunction<int32(const Protocol::ObjectInfo&)>;
	
public:
	void Init(UWorld* TWorld);
	void AddFactory(const FS1ObjectKey& Key, CreateFunc Create, DestroyFunc Destroy);

	AActor* SpawnObject(const Protocol::ObjectInfo& Info, bool IsMine);
	void DespawnObject(uint64 ObjectId);

	void MoveObject(const Protocol::S_MOVE& MovePkt);

	FS1ObjectKey MakeKey(const Protocol::ObjectInfo& Info) const;

public:
	UPROPERTY()
	UWorld* World;

	TMap<FS1ObjectKey, CreateFunc> CreateRegistry;
	TMap<FS1ObjectKey, DestroyFunc> DestroyRegistry;
	TMap<int32, TypeConstructor>  TypeConstructors;

	TMap<uint64, FS1ObjectEntry> Objects;
	TMap<uint64, AS1Player*> Players;
	TMap<uint64, AS1Monster*> Monsters;

public:
	void SetClasses(TSubclassOf<AS1MyPlayer> InMyPlayerClass, TSubclassOf<AS1Player> InOtherPlayerClass, TSubclassOf<AS1Monster> InMonsterClass);

public:
	UPROPERTY(EditAnywhere, Category = "Classes")
	TSubclassOf<AS1MyPlayer> MyPlayerClass;

	UPROPERTY(EditAnywhere, Category = "Classes")
	TSubclassOf<AS1Player> OtherPlayerClass;

	UPROPERTY(EditAnywhere, Category = "Classes")
	TSubclassOf<AS1Monster> MonsterClass;
};
