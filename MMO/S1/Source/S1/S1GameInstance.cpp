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
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("GameInstance Init"));

	UWorld* World = GetWorld();
	if (!World)
		return;

	// ------------------------------------------------------
	// 1. NetMode 체크: 독립 서버 연결용 클라이언트만 초기화
	ENetMode NetMode = World->GetNetMode();
	if (NetMode == NM_DedicatedServer)
	{
		UE_LOG(LogTemp, Warning, TEXT("GameInstance Init skipped: NetMode=%d"), (int32)NetMode);
		return;
	}

	// ------------------------------------------------------
	// 2. PIE(Play In Editor) 체크: Editor에서 여러 인스턴스 생성 방지
#if WITH_EDITOR
	if (IsRunningDedicatedServer())
	{
		UE_LOG(LogTemp, Warning, TEXT("GameInstance Init skipped: Dedicated Server in Editor"));
		return;
	}
#endif

	MapManager = NewObject<US1MapManager>(this);
	if (MapManager)
	{
		MapManager->LoadMap(1, 100.f);
	}

	FString ConfigPath;
	FString DataPath;

#if WITH_EDITOR
	// Editor, PIE
	ConfigPath = FPaths::ProjectContentDir() / TEXT("Data/config.json");
	DataPath = FPaths::ProjectContentDir() / TEXT("Data/");
#else
	// Standalone / 패키징 exe
	ConfigPath = FPaths::ProjectContentDir() / TEXT("Data/config.json");
	DataPath = FPaths::ProjectContentDir() / TEXT("Data/");
#endif

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("ConfigPath: %s"), *ConfigPath));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("DataPath: %s"), *DataPath));

	S1ConfigManager::Instance().LoadConfig(TCHAR_TO_UTF8(*ConfigPath));
	S1DataManager::Instance().LoadData(TCHAR_TO_UTF8(*DataPath));

	FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &US1GameInstance::OnWorldReady);
}

void US1GameInstance::OnWorldReady(UWorld* World, const UWorld::InitializationValues IVS)
{
	if (!World || Connected) // 이미 연결되어있으면 무시
		return;

	if (BP_ObjectManagerClass)
	{
		ObjectManager = NewObject<US1ObjectManager>(this, BP_ObjectManagerClass);
		if (ObjectManager)
		{
			ObjectManager->Init(GetWorld());
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("World is ready, connecting to server..."));
	ConnectToGameServer();

	// 한 번만 실행되도록 delegate 제거
	FWorldDelegates::OnPostWorldInitialization.RemoveAll(this);
}

void US1GameInstance::Shutdown()
{
	DisconnectFromGameServer();
	Super::Shutdown();
}

void US1GameInstance::ConnectToGameServer()
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Connected To Server"));
	UWorld* World = GetWorld();
	if (!World)
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
}

void US1GameInstance::DisconnectFromGameServer()
{
	if (Connected && Socket && GameServerSession.IsValid())
	{
		Protocol::C_LEAVE_GAME LeavePkt;
		SEND_PACKET(LeavePkt);
	}
}

void US1GameInstance::HandleRecvPackets()
{
	if (Connected && Socket && GameServerSession.IsValid())
	{
		GameServerSession->HandleRecvPackets();
	}
}

void US1GameInstance::SendPacket(SendBufferRef SendBuffer)
{
	if (Connected && Socket && GameServerSession.IsValid())
	{
		GameServerSession->SendPacket(SendBuffer);
	}
}

void US1GameInstance::HandleSpawn(const Protocol::ObjectInfo& ObjectInfo, bool IsMine)
{
	if (Socket == nullptr || GameServerSession == nullptr)
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("HandleSpawn: Socket or GameServerSession is nullptr"));
		return;
	}

	auto* World = GetWorld();
	if (World == nullptr)
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("HandleSpawn: World is nullptr"));
		return;
	}

	// 중복 처리 체크
	AActor* NewActor = ObjectManager->SpawnObject(ObjectInfo, IsMine);
	if (!NewActor)
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
				FString::Printf(TEXT("HandleSpawn: SpawnObject returned nullptr for ObjectID %llu"), ObjectInfo.object_id()));
		return;
	}

	if (IsMine)
	{
		MyPlayer = Cast<AS1MyPlayer>(NewActor);
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue,
				FString::Printf(TEXT("HandleSpawn: Set MyPlayer = %s"), MyPlayer ? *MyPlayer->GetName() : TEXT("nullptr")));

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
		
		float DistSq = FVector::DistSquared2D(ServerPos, MyPlayer->GetActorLocation());
		const float Allow = FMath::Square(25.f);

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
	
	//Creature->ChangeState(Protocol::STATE_MACHINE_SKILL);
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
	if (Creature == nullptr)
		return;

	// CastId 검증
	if (Creature == MyPlayer)
	{
		if (CastStartPkt.castid() < MyPlayer->SkillComponent->GetSkillState(CastStartPkt.skillid())->CastID)
			return; // 이미 더 최신 캐스팅 중이면 무시

		FSkillState* LocalState = MyPlayer->SkillComponent->GetSkillState(CastStartPkt.skillid());

		uint64 ClientRecvTick = static_cast<uint64>(FPlatformTime::Seconds() * 1000);
		OneWayDelay = (ClientRecvTick - CastStartPkt.clientsend()) / 2;

		// 서버 캐스팅 종료를 클라 시계로 변환
		uint64 CastDuration = CastStartPkt.castendtime() - CastStartPkt.servernow();
		uint64 ClientCastEnd = ClientRecvTick + CastDuration - OneWayDelay;

		MyPlayer->HandleStartServerCasting(*LocalState, ClientCastEnd);
	}
	else
	{
		// OtherPlayer / Monster는 단순 State 변경 + 애니메이션
		Creature->ChangeState(Protocol::STATE_MACHINE_CASTING);
		// Creature->UpdateAnim(CastStartPkt.skillid()); // 필요 시
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

	Vec3 Vec = CastSuccessPkt.skill_info().targetpos();
	FVector TargetLoc = {Vec.x(), Vec.y(), Vec.z()};
	FVector Direction = (TargetLoc - Creature->GetActorLocation());
	Direction.Z = 0;
	Direction.Normalize();
	
	float Rad = FMath::Atan2(Direction.Y, Direction.X);
	float Degree = FMath::RadiansToDegrees(Rad);

	FRotator NewRot(0.f, Degree, 0.f);

	if (Creature == MyPlayer)
	{
		FSkillState* LocalState = MyPlayer->SkillComponent->GetSkillState(CastSuccessPkt.skillid());

		if (LocalState->CastID != CastSuccessPkt.castid())
			return;

		uint64 ClientRecvTick = static_cast<uint64>(FPlatformTime::Seconds() * 1000);

		// 서버 캐스팅 종료를 클라 시계로 변환
		uint64 CooldownTick = CastSuccessPkt.cooldownendtime() - CastSuccessPkt.servernow();
		uint64 CooldownRemainTick = CooldownTick - OneWayDelay;

		MyPlayer->HandleServerFinishCasting(CastSuccessPkt.skillid(), CooldownRemainTick); // 캐스팅바 UI 정리

		const float CurrentYaw = MyPlayer->GetActorRotation().Yaw;
		const float TargetYaw = NewRot.Yaw;

		float DeltaYaw = FMath::Abs(FRotator::NormalizeAxis(TargetYaw - CurrentYaw));

		if (DeltaYaw > 1.f)
		{
			MyPlayer->SetActorRotation(NewRot);
			MyPlayer->SetYaw(NewRot.Yaw);
		}
	}
	else
	{
		Creature->SetActorRotation(NewRot);
		Creature->SetYaw(NewRot.Yaw);
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
		if (CastCancelPkt.castid() != MyPlayer->GetCurrentCastId())
			return;

		MyPlayer->HandleServerCancelCasting(CastCancelPkt.skillid()); // 캐스팅바 UI 정리
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

void US1GameInstance::HandleHeartbeat(const Protocol::S_HEARTBEAT& pkt)
{
	if (Socket == nullptr || GameServerSession == nullptr)
		return;

	auto* World = GetWorld();
	if (World == nullptr)
		return;

	uint64 clientTime = static_cast<uint64>(FPlatformTime::Cycles64() * 1000);

	Protocol::C_HEARTBEAT responsePkt;
	responsePkt.set_clienttime(clientTime);
	auto sendBuffer = ClientPacketHandler::MakeSendBuffer(responsePkt);
	SendPacket(sendBuffer);
}
