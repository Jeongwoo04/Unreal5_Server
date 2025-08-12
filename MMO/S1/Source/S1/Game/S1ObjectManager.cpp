// Fill out your copyright notice in the Description page of Project Settings.


#include "S1ObjectManager.h"
#include "S1MyPlayer.h"
#include "S1Monster.h"

void US1ObjectManager::Init(UWorld* TWorld)
{
	World = TWorld;
	CreateRegistry.Empty();
	DestroyRegistry.Empty();
	TypeConstructors.Empty();
	Objects.Empty();

	TypeConstructors.Add(Protocol::OBJECT_TYPE_CREATURE,
		[](const Protocol::ObjectInfo& Info) { return Info.creature_type(); });
	//TypeConstructors.Add(Protocol::OBJECT_TYPE_PROJECTILE,
	//	[](const Protocol::ObjectInfo& Info) { return Info.projectile_type(); });

	// Player
	AddFactory(FS1ObjectKey{Protocol::OBJECT_TYPE_CREATURE, Protocol::CREATURE_TYPE_PLAYER},
		[this](UWorld* W, const Protocol::ObjectInfo& Info, bool IsMine) -> AActor*
		{
			if (!OtherPlayerClass)
				return nullptr;

			FVector Loc(Info.pos_info().x(), Info.pos_info().y(), Info.pos_info().z());
			FRotator Rot(0.f, Info.pos_info().yaw(), 0.f);

			if (IsMine)
			{
				auto* MyPlayer = W->SpawnActor<AS1MyPlayer>(MyPlayerClass, Loc, Rot);
				if (MyPlayer)
					MyPlayer->SetPosInfo(Info.pos_info());

				if (!Players.Contains(Info.object_id()))
				{
					Players.Add(Info.object_id(), MyPlayer);
				}

				return MyPlayer;
			}
			else
			{
				auto* Player = W->SpawnActor<AS1Player>(OtherPlayerClass, Loc, Rot);
				if (Player)
					Player->SetPosInfo(Info.pos_info());

				if (!Players.Contains(Info.object_id()))
				{
					Players.Add(Info.object_id(), Player);
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

				World->DestroyActor(Actor);
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
			if (Monster)
				Monster->SetPosInfo(Info.pos_info());

			if (!Monsters.Contains(Info.object_id()))
			{
				Monsters.Add(Info.object_id(), Monster);
			}
			
			return Monster;
		},
		[this](AActor* Actor, uint64 Id)
		{
			if (Actor)
			{
				if (Monsters.Contains(Id))
					Monsters.Remove(Id);

				World->DestroyActor(Actor);
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

void US1ObjectManager::MoveObject(const Protocol::S_MOVE& MovePkt)
{
	const uint64 ObjectId = MovePkt.info().object_id();
}

FS1ObjectKey US1ObjectManager::MakeKey(const Protocol::ObjectInfo& Info) const
{
	int32 mainType = Info.object_type();
	int32 subType = 0;

	if (auto sub = TypeConstructors.Find(mainType))
		subType = (*sub)(Info);

	return FS1ObjectKey{ mainType, subType };
}

void US1ObjectManager::SetClasses(TSubclassOf<AS1MyPlayer> InMyPlayerClass, TSubclassOf<AS1Player> InOtherPlayerClass, TSubclassOf<AS1Monster> InMonsterClass)
{
	MyPlayerClass = InMyPlayerClass;
	OtherPlayerClass = InOtherPlayerClass;
	MonsterClass = InMonsterClass;
}
