// Fill out your copyright notice in the Description page of Project Settings.


#include "Network/PacketSession.h"
#include "NetworkWorker.h"
#include "Sockets.h"
#include "Common/TcpSocketBuilder.h"
#include "Serialization/ArrayWriter.h"
#include "SocketSubsystem.h"
#include "ClientPacketHandler.h"

PacketSession::PacketSession(class FSocket* Socket) : Socket(Socket)
{
	ClientPacketHandler::Init();
}

PacketSession::~PacketSession()
{
	Disconnect();
}

void PacketSession::Run()
{
	RecvWorkerThread = MakeShared<RecvWorker>(Socket, AsShared());
	SendWorkerThread = MakeShared<SendWorker>(Socket, AsShared());
}

#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"

static IFileHandle* CSVFileHandle = nullptr;
static double LastWriteTime = 0.0;

struct FWindowStats {
	int32 TotalPacketCount = 0;
	double TotalTimeMs = 0.0;
	int32 BurstCount = 0; // 몇 번의 HandleRecvPackets 호출이 있었는지

	void Reset() {
		TotalPacketCount = 0;
		TotalTimeMs = 0.0;
		BurstCount = 0;
	}
} static CurrentWindow;

void PacketSession::HandleRecvPackets()
{
	double StartTime = FPlatformTime::Seconds();
	int32 PacketCount = 0;

	while (true)
	{
		TArray<uint8> Packet;
		if (RecvPacketQueue.Dequeue(OUT Packet) == false)
			break;

		PacketSessionRef ThisPtr = AsShared();
		ClientPacketHandler::HandlePacket(ThisPtr, Packet.GetData(), Packet.Num());

		PacketCount++;
	}

	if (PacketCount > 1)
	{
		const double EndTime = FPlatformTime::Seconds();
		const double TotalMs = (EndTime - StartTime) * 1000.0;

		// 즉시 파일에 쓰지 않고 윈도우 버퍼에 누적
		CurrentWindow.TotalPacketCount += PacketCount;
		CurrentWindow.TotalTimeMs += TotalMs;
		CurrentWindow.BurstCount++;

		// 현재 시간 확인 및 5초 주기 체크
		double CurrentTime = FPlatformTime::Seconds();
		if (LastWriteTime == 0.0) LastWriteTime = CurrentTime;

		if (CurrentTime - LastWriteTime >= 5.0)
		{
			WritePeriodicStats(CurrentTime);
			LastWriteTime = CurrentTime;
		}
	}
}

void PacketSession::WritePeriodicStats(double Timestamp)
{
	if (CurrentWindow.TotalPacketCount == 0) return;

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FString Path = FPaths::ProjectSavedDir() / TEXT("PacketPeriodicStats.csv");

	if (CSVFileHandle == nullptr)
	{
		CSVFileHandle = PlatformFile.OpenWrite(*Path, true);
		if (CSVFileHandle && PlatformFile.FileSize(*Path) == 0)
		{
			// 헤더: 시간, 5초간 처리한 총 패킷, 총 소요시간, 평균 처리시간, 버스트 횟수
			FString Header = TEXT("Timestamp,TotalPackets,SumTotalMs,AvgMsPerPacket,BurstCount\n");
			CSVFileHandle->Write((const uint8*)TCHAR_TO_ANSI(*Header), Header.Len());
		}
	}

	if (CSVFileHandle)
	{
		double AvgMs = CurrentWindow.TotalTimeMs / (double)CurrentWindow.TotalPacketCount;

		// 데이터 기록 (5초간의 요약본)
		FString Line = FString::Printf(TEXT("%.2f,%d,%.4f,%.4f,%d\n"),
			Timestamp,
			CurrentWindow.TotalPacketCount,
			CurrentWindow.TotalTimeMs,
			AvgMs,
			CurrentWindow.BurstCount);

		CSVFileHandle->Write((const uint8*)TCHAR_TO_ANSI(*Line), Line.Len());
		CSVFileHandle->Flush();
	}

	// 다음 5초를 위해 초기화
	CurrentWindow.Reset();
}

void PacketSession::SendPacket(SendBufferRef SendBuffer)
{
	SendPacketQueue.Enqueue(SendBuffer);
}

void PacketSession::Disconnect()
{
	if (RecvWorkerThread)
	{
		RecvWorkerThread->Destroy();
		RecvWorkerThread = nullptr;
	}

	if (SendWorkerThread)
	{
		SendWorkerThread->Destroy();
		SendWorkerThread = nullptr;
	}
}

