// Fill out your copyright notice in the Description page of Project Settings.


#include "S1GameInstance.h"
#include "Sockets.h"
#include "Common/TcpSocketBuilder.h"
#include "Serialization/ArrayWriter.h"
#include "SocketSubsystem.h"
#include "PacketSession.h"
#include "Protocol.pb.h"
#include "ClientPacketHandler.h"
#include "S1MyPlayer.h"
#include "S1Monster.h"
#include "S1Projectile.h"
#include "S1MapManager.h"
#include "S1ObjectManager.h"
#include "Data/S1ConfigManager.h"
#include "Data/S1DataManager.h"
#include "S1SkillComponent.h"

void US1GameInstance::Init()
{
	Super::Init();

#if WITH_EDITOR
	if (BP_ObjectManagerClass)
	{
		ObjectManager = NewObject<US1ObjectManager>(this, BP_ObjectManagerClass);
		if (ObjectManager)
		{
			ObjectManager->Init(GetWorld());
		}
	}	

	MapManager = NewObject<US1MapManager>(this);
	if (MapManager)
	{
		MapManager->LoadMap(1, 100.f);
	}

	FString ConfigPath = FPaths::ProjectDir() / TEXT("Data/config.json");
	FString DataPath = FPaths::ProjectDir() / TEXT("Data/");

	UE_LOG(LogTemp, Warning, TEXT("ConfigPath: %s"), *ConfigPath);
	UE_LOG(LogTemp, Warning, TEXT("DataPath: %s"), *DataPath);

	S1ConfigManager::Instance().LoadConfig("C:/Users/jeson/Desktop/unreal/MMO/S1/Source/S1/Data/config.json");
	S1DataManager::Instance().LoadData("C:/Users/jeson/Desktop/unreal/MMO/S1/Source/S1/Data/");

#endif
}

void US1GameInstance::ConnectToGameServer()
{
#if WITH_EDITOR
	// 에디터에서 PIE 실행 시 서버쪽 인스턴스는 제외
	if (IsRunningDedicatedServer() || (GetWorld() && GetWorld()->GetNetMode() == NM_DedicatedServer))
		return;

	Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(TEXT("Stream"), TEXT("Client Socket"));
	
	FIPv4Address Ip;
	FIPv4Address::Parse(IpAddress, Ip);

	TSharedRef<FInternetAddr> InternetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	InternetAddr->SetIp(Ip.Value);
	InternetAddr->SetPort(Port);

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Connecting To Server...")));

	Connected = Socket->Connect(*InternetAddr);

	if (Connected)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Connection Success")));

		// Session
		GameServerSession = MakeShared<PacketSession>(Socket);
		GameServerSession->Run();

		// Lobby
		{
			Protocol::C_LOGIN Pkt;
			SendBufferRef SendBuffer = ClientPacketHandler::MakeSendBuffer(Pkt);
			SendPacket(SendBuffer);
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Connection Failed")));
	}
#endif
}

void US1GameInstance::DisconnectFromGameServer()
{
	if (Connected == false || Socket == nullptr || GameServerSession == nullptr)
		return;

	Protocol::C_LEAVE_GAME LeavePkt;
	SEND_PACKET(LeavePkt);
}

void US1GameInstance::HandleRecvPackets()
{
	if (Connected == false || Socket == nullptr || GameServerSession == nullptr)
		return;

	GameServerSession->HandleRecvPackets();
}

void US1GameInstance::SendPacket(SendBufferRef SendBuffer)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	GameServerSession->SendPacket(SendBuffer);
}

void US1GameInstance::HandleSpawn(const Protocol::ObjectInfo& ObjectInfo, bool IsMine)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	auto* World = GetWorld();
	if (World == nullptr)
		return;

	// 중복 처리 체크
	AActor* NewActor = ObjectManager->SpawnObject(ObjectInfo, IsMine);
	if (!NewActor)
		return;

	if (IsMine)
	{
		MyPlayer = Cast<AS1MyPlayer>(NewActor);

		OnMyPlayerSpawned.Broadcast(MyPlayer);
	}
}

void US1GameInstance::HandleSpawn(const Protocol::S_ENTER_GAME& EnterGamePkt)
{
	HandleSpawn(EnterGamePkt.object(), true);
}

void US1GameInstance::HandleSpawn(const Protocol::S_SPAWN& SpawnPkt)
{
	for (auto& Object : SpawnPkt.objects())
	{
		HandleSpawn(Object, false);
	}
}

void US1GameInstance::HandleDespawn(uint64 ObjectId)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	auto* World = GetWorld();
	if (World == nullptr)
		return;

	ObjectManager->DespawnObject(ObjectId);
}

void US1GameInstance::HandleDespawn(const Protocol::S_DESPAWN& DespawnPkt)
{
	for (auto& ObjectId : DespawnPkt.object_ids())
	{
		HandleDespawn(ObjectId);
	}
}

void US1GameInstance::HandleMove(const Protocol::S_MOVE& MovePkt)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	auto* World = GetWorld();
	if (World == nullptr)
		return;

	const uint64 ObjectId = MovePkt.info().object_id();

	AActor* FindActor = ObjectManager->FindObject(ObjectId);
	if (FindActor == nullptr)
		return;

	AS1Projectile* Projectile = Cast<AS1Projectile>(FindActor);
	if (Projectile)
	{
		Projectile->SetPosInfo(MovePkt.info());
	}

	AS1Creature* Creature = Cast<AS1Creature>(FindActor);
	if (Creature == nullptr)
		return;

	if (Creature == MyPlayer)
	{
		FVector ServerPos = FVector(MovePkt.info().x(), MovePkt.info().y(), MovePkt.info().z());
		
		float DistSq = FVector::DistSquared(ServerPos, MyPlayer->GetActorLocation());
		const float Allow = FMath::Square(5.f);

		if (DistSq > Allow)
		{
			Creature->SetPosInfo(MovePkt.info());
		}
		else
			return;
	}
	else
	{
		Creature->SetPosInfo(MovePkt.info());
	}
}

void US1GameInstance::HandleSkill(const Protocol::S_SKILL& SkillPkt)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	auto* World = GetWorld();
	if (World == nullptr)
		return;

	const uint64 ObjectId = SkillPkt.object_id();

	AActor* FindActor = ObjectManager->FindObject(ObjectId);
	if (FindActor == nullptr)
		return;

	AS1Creature* Creature = Cast<AS1Creature>(FindActor);
	if (Creature == nullptr)
		return;
	
	Creature->ChangeState(Protocol::STATE_MACHINE_SKILL);
	// Creature->UpdateAnim(SkillPkt.skillid());

	// ActionIndex에 맞게 ExecuteAction
	Creature->HandleActionPkt(SkillPkt);
}

void US1GameInstance::HandleSkillCastStart(const Protocol::S_SKILL_CAST_START& CastStartPkt)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	auto* World = GetWorld();
	if (World == nullptr)
		return;

	const uint64 ObjectId = CastStartPkt.object_id();

	AActor* FindActor = ObjectManager->FindObject(ObjectId);
	if (FindActor == nullptr)
		return;

	AS1Creature* Creature = Cast<AS1Creature>(FindActor);

	// CastId 검증
	if (Creature == MyPlayer)
	{
		if (CastStartPkt.castid() < MyPlayer->SkillComponent->GetCurrentSkillState().CastID)
			return; // 이미 더 최신 캐스팅 중이면 무시

		// MyPlayer는 서버 기준 보정 포함
		FSkillState ServerState;
		ServerState.SkillID = CastStartPkt.skillid();
		ServerState.CastTime = CastStartPkt.castendtime() - CastStartPkt.servernow(); // 서버 기준 캐스팅 시간
		ServerState.CastID = CastStartPkt.castid();

		MyPlayer->StartCasting(ServerState, CastStartPkt.castendtime());
	}
	else
	{
		// OtherPlayer / Monster는 단순 State 변경 + 애니메이션
		Creature->ChangeState(Protocol::STATE_MACHINE_CASTING);
		// Creature->UpdateAnim(CastStartPkt.skillid());
	}
}

void US1GameInstance::HandleSkillCastSuccess(const Protocol::S_SKILL_CAST_SUCCESS& CastSuccessPkt)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	auto* World = GetWorld();
	if (World == nullptr)
		return;

	const uint64 ObjectId = CastSuccessPkt.object_id();

	AActor* FindActor = ObjectManager->FindObject(ObjectId);
	if (FindActor == nullptr)
		return;

	AS1Creature* Creature = Cast<AS1Creature>(FindActor);
	if (Creature == nullptr)
		return;

	if (Creature == MyPlayer)
	{
		// CastId 검증
		if (CastSuccessPkt.castid() != MyPlayer->SkillComponent->GetCurrentSkillState().CastID)
			return;

		MyPlayer->FinishCasting(); // 캐스팅바 UI 정리
	}
	else
	{
		Creature->ChangeState(Protocol::STATE_MACHINE_IDLE);
		// Creature->UpdateAnim(CastSuccessPkt.skillid());
	}
}

void US1GameInstance::HandleSkillCastCancel(const Protocol::S_SKILL_CAST_CANCEL& CastCancelPkt)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	auto* World = GetWorld();
	if (World == nullptr)
		return;

	const uint64 ObjectId = CastCancelPkt.object_id();

	AActor* FindActor = ObjectManager->FindObject(ObjectId);
	if (FindActor == nullptr)
		return;

	AS1Creature* Creature = Cast<AS1Creature>(FindActor);
	if (Creature == nullptr)
		return;
	
	if (Creature == MyPlayer)
	{
		// CastId 검증
		if (CastCancelPkt.castid() != MyPlayer->SkillComponent->GetCurrentSkillState().CastID)
			return;

		MyPlayer->CancelCasting(); // 캐스팅바 UI 정리
	}
	else
	{
		// OtherPlayer / Monster
		Creature->ChangeState(Protocol::STATE_MACHINE_IDLE);
		// Creature->UpdateAnim(CastCancelPkt.skillid());
	}
}

void US1GameInstance::HandleChangeHp(const Protocol::S_CHANGE_HP& ChangeHpPkt)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	auto* World = GetWorld();
	if (World == nullptr)
		return;

	// TODO : Hp UI 변경
}

void US1GameInstance::HandleDie(const Protocol::S_DIE& DiePkt)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	auto* World = GetWorld();
	if (World == nullptr)
		return;

	// TODO : Die anim
	HandleDespawn(DiePkt.object_id());
}
