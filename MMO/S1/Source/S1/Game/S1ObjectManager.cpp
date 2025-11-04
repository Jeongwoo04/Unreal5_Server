// Fill out your copyright notice in the Description page of Project Settings.


#include "S1ObjectManager.h"
#include "S1MyPlayer.h"
#include "S1Monster.h"
#include "S1Projectile.h"
#include "S1Field.h"

void US1ObjectManager::Init(UWorld* TWorld)
{
	World = TWorld;
	CreateRegistry.Empty();
	DestroyRegistry.Empty();
	TypeConstructors.Empty();
	Objects.Empty();

	TypeConstructors.Add(Protocol::OBJECT_TYPE_CREATURE,
		[](const Protocol::ObjectInfo& Info) { return Info.creature_type(); });
	TypeConstructors.Add(Protocol::OBJECT_TYPE_PROJECTILE,
		[](const Protocol::ObjectInfo& Info) { return 0; });
	TypeConstructors.Add(Protocol::OBJECT_TYPE_ENV,
		[](const Protocol::ObjectInfo& Info) { return 1; });

	// Player
	AddFactory(FS1ObjectKey{ Protocol::OBJECT_TYPE_CREATURE, Protocol::CREATURE_TYPE_PLAYER },
		[this](UWorld* W, const Protocol::ObjectInfo& Info, bool IsMine) -> AActor*
		{
			if (!OtherPlayerClass)
				return nullptr;

			FVector Loc(Info.pos_info().x(), Info.pos_info().y(), Info.pos_info().z());
			FRotator Rot(0.f, Info.pos_info().yaw(), 0.f);

			if (IsMine)
			{
				auto* MyPlayer = W->SpawnActor<AS1MyPlayer>(MyPlayerClass, Loc, Rot);
				if (MyPlayer && !Players.Contains(Info.object_id()))
				{
					Objects.Add(Info.object_id(), { FS1ObjectKey{Protocol::OBJECT_TYPE_CREATURE, Protocol::CREATURE_TYPE_PLAYER}, MyPlayer });
					Players.Add(Info.object_id(), MyPlayer);
					MyPlayer->SetPosInfo(Info.pos_info());
				}

				return MyPlayer;
			}
			else
			{
				auto* Player = W->SpawnActor<AS1Player>(OtherPlayerClass, Loc, Rot);
				if (Player && !Players.Contains(Info.object_id()))
				{
					Objects.Add(Info.object_id(), { FS1ObjectKey{Protocol::OBJECT_TYPE_CREATURE, Protocol::CREATURE_TYPE_PLAYER}, Player });
					Players.Add(Info.object_id(), Player);
					Player->SetPosInfo(Info.pos_info());
				}

				return Player;
			}
		},
		[this](AActor* Actor, uint64 Id)
		{
			if (Actor)
			{
				if (Players.Contains(Id))
					Players.Remove(Id);
				if (Objects.Contains(Id))
					Objects.Remove(Id);

				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Player Is Despawn. ID = %llu"), Id));

				Actor->Destroy();
			}
			else
			{
				// Log
			}
		}
	);

	// Monster
	AddFactory(FS1ObjectKey{Protocol::OBJECT_TYPE_CREATURE, Protocol::CREATURE_TYPE_MONSTER},
		[this](UWorld* W, const Protocol::ObjectInfo& Info, bool IsMine) -> AActor*
		{
			if (!MonsterClass)
				return nullptr;

			FVector Loc(Info.pos_info().x(), Info.pos_info().y(), Info.pos_info().z());
			FRotator Rot(0.f, Info.pos_info().yaw(), 0.f);

			auto* Monster = W->SpawnActor<AS1Monster>(MonsterClass, Loc, Rot);
			if (Monster && !Monsters.Contains(Info.object_id()))
			{
				Objects.Add(Info.object_id(), { FS1ObjectKey{Protocol::OBJECT_TYPE_CREATURE, Protocol::CREATURE_TYPE_MONSTER}, Monster });
				Monsters.Add(Info.object_id(), Monster);
				Monster->SetPosInfo(Info.pos_info());
			}			
			
			return Monster;
		},
		[this](AActor* Actor, uint64 Id)
		{
			if (Actor)
			{
				if (Monsters.Contains(Id))
					Monsters.Remove(Id);
				if (Objects.Contains(Id))
					Objects.Remove(Id);

				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Monster Is Despawn. ID = %llu"), Id));

				Actor->Destroy();
			}
			else
			{
				// Log
			}
		}
	);

	// Projectile
	AddFactory(FS1ObjectKey{ Protocol::OBJECT_TYPE_PROJECTILE, 0 },
		[this](UWorld* W, const Protocol::ObjectInfo& Info, bool IsMine) -> AActor*
		{
			if (!ProjectileClass)
				return nullptr;

			FVector Loc(Info.pos_info().x(), Info.pos_info().y(), Info.pos_info().z());
			FRotator Rot(0.f, Info.pos_info().yaw(), 0.f);

			auto* Projectile = W->SpawnActor<AS1Projectile>(ProjectileClass, Loc, Rot);
			if (Projectile && !Projectiles.Contains(Info.object_id()))
			{
				Objects.Add(Info.object_id(), { FS1ObjectKey{Protocol::OBJECT_TYPE_PROJECTILE, 0}, Projectile });
				Projectiles.Add(Info.object_id(), Projectile);
				Projectile->SetPosInfo(Info.pos_info());
			}

			return Projectile;
		},
		[this](AActor* Actor, uint64 Id)
		{
			if (Actor)
			{
				if (Projectiles.Contains(Id))
					Projectiles.Remove(Id);
				if (Objects.Contains(Id))
					Objects.Remove(Id);

				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Projectile Is Despawn. ID = %llu"), Id));

				Actor->Destroy();
			}
			else
			{
				// Log
			}
		}
	);
	// Field
	AddFactory(FS1ObjectKey{ Protocol::OBJECT_TYPE_ENV, 1 },
		[this](UWorld* W, const Protocol::ObjectInfo& Info, bool IsMine) -> AActor*
		{
			if (!FieldClass)
				return nullptr;

			FVector Loc(Info.pos_info().x(), Info.pos_info().y(), Info.pos_info().z());
			FRotator Rot(0.f, Info.pos_info().yaw(), 0.f);

			auto* Field = W->SpawnActor<AS1Field>(FieldClass, Loc, Rot);
			if (Field && !Fields.Contains(Info.object_id()))
			{
				Objects.Add(Info.object_id(), { FS1ObjectKey{Protocol::OBJECT_TYPE_ENV, 1}, Field });
				Fields.Add(Info.object_id(), Field);
				Field->SetPosInfo(Info.pos_info());
			}

			return Field;
		},
		[this](AActor* Actor, uint64 Id)
		{
			if (Actor)
			{
				if (Fields.Contains(Id))
					Fields.Remove(Id);
				if (Objects.Contains(Id))
					Objects.Remove(Id);

				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Field Is Despawn. ID = %llu"), Id));

				Actor->Destroy();
			}
			else
			{
				// Log
			}
		}
	);
}

void US1ObjectManager::AddFactory(const FS1ObjectKey& Key, CreateFunc Create, DestroyFunc Destroy)
{
	CreateRegistry.Add(Key, Create);
	DestroyRegistry.Add(Key, Destroy);
}

AActor* US1ObjectManager::FindObject(uint64 ObjectId)
{
	if (FS1ObjectEntry* Found = Objects.Find(ObjectId))
	{
		return Found->Actor;
	}
	return nullptr;
}

AActor* US1ObjectManager::SpawnObject(const Protocol::ObjectInfo& Info, bool IsMine)
{
	if (!World)
		return nullptr;

	FS1ObjectKey Key = MakeKey(Info);

	if (auto* FuncPtr = CreateRegistry.Find(Key))
	{
		AActor* NewActor = (*FuncPtr)(World, Info, IsMine);
		if (NewActor)
		{
			Objects.Add(Info.object_id(), { Key, NewActor });
		}

		return NewActor;
	}

	return nullptr;
}

void US1ObjectManager::DespawnObject(uint64 ObjectId)
{
	if (auto* Entry = Objects.Find(ObjectId))
	{
		if (auto* FuncPtr = DestroyRegistry.Find(Entry->Key))
		{
			(*FuncPtr)(Entry->Actor, ObjectId);
		}

		Objects.Remove(ObjectId);
	}
}

FS1ObjectKey US1ObjectManager::MakeKey(const Protocol::ObjectInfo& Info) const
{
	int32 mainType = Info.object_type();
	int32 subType = 0;

	if (auto sub = TypeConstructors.Find(mainType))
		subType = (*sub)(Info);

	return FS1ObjectKey{ mainType, subType };
}