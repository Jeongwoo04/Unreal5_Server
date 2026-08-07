// Fill out your copyright notice in the Description page of Project Settings.


#include "Network/NetworkWorker.h"
#include "Sockets.h"
#include "Serialization/ArrayWriter.h"
#include "PacketSession.h"

RecvWorker::RecvWorker(FSocket* Socket, TSharedPtr<class PacketSession> Session) : Socket(Socket), SessionRef(Session)
{
	Thread = FRunnableThread::Create(this, TEXT("RecvWorkerThread"));
}

RecvWorker::~RecvWorker()
{

}

bool RecvWorker::Init()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Recv Thread Init")));
	return true;
}

uint32 RecvWorker::Run()
{
	while (Running)
	{
		TArray<uint8> Packet;

		if (ReceivePacket(OUT Packet))
		{
				if (TSharedPtr<PacketSession> Session = SessionRef.Pin())
				{
					Session->RecvPacketQueue.Enqueue(MoveTemp(Packet));
				}
			}
		}

	return 0;
}

void RecvWorker::Exit()
{

}

void RecvWorker::Destroy()
{
	Running = false;
}

bool RecvWorker::ReceivePacket(TArray<uint8>& OutPacket)
{
	// 현재 Socket에 들어온 데이터를 RecvBuffer에 누적
	uint32 PendingSize = 0;

	while (Socket->HasPendingData(PendingSize))
	{
		const int32 OldSize = RecvBuffer.Num();
		RecvBuffer.SetNumUninitialized(OldSize + PendingSize);

		int32 NumRead = 0;

		if (!Socket->Recv(RecvBuffer.GetData() + OldSize, PendingSize, NumRead))
		{
			RecvBuffer.SetNum(OldSize);
			return false;
		}

		if (NumRead <= 0)
		{
			RecvBuffer.SetNum(OldSize);
			return false;
		}

		RecvBuffer.SetNum(OldSize + NumRead);
	}

	const int32 HeaderSize = sizeof(FPacketHeader);

	// Header도 못 읽으면 다음 Recv까지 기다림
	if (RecvBuffer.Num() < HeaderSize)
		return false;

	FPacketHeader Header;
	FMemory::Memcpy(&Header, RecvBuffer.GetData(), HeaderSize);

	// 패킷 전체가 아직 안 들어왔으면 기다림
	if (RecvBuffer.Num() < Header.PacketSize)
		return false;

	UE_LOG(LogTemp,
		Log,
		TEXT("Recv PacketID : %d, PacketSize : %d"),
		Header.PacketID,
		Header.PacketSize);

	// Packet 추출
	OutPacket.Reset();
	OutPacket.Append(RecvBuffer.GetData(), Header.PacketSize);

	// RecvBuffer 앞부분 제거
	RecvBuffer.RemoveAt(0, Header.PacketSize, false);

	return true;
}

//bool RecvWorker::ReceivePacket(TArray<uint8>& OutPacket)
//{
//	// 패킷 헤더 파싱
//	const int32 HeaderSize = sizeof(FPacketHeader);
//	TArray<uint8> HeaderBuffer;
//	HeaderBuffer.AddZeroed(HeaderSize);
//
//	if (ReceiveDesiredBytes(HeaderBuffer.GetData(), HeaderSize) == false)
//		return false;
//
//	// ID, Size 추출
//	FPacketHeader Header;
//	{
//		FMemoryReader Reader(HeaderBuffer);
//		Reader << Header;
//		UE_LOG(LogTemp, Log, TEXT("Recv PacketID : %d, PacketSize : %d"), Header.PacketID, Header.PacketSize);
//	}
//
//	// 패킷 헤더 복사
//	OutPacket = HeaderBuffer;
//
//	// 패킷 내용 파싱
//	TArray<uint8> PayloadBuffer;
//	const int32 PayloadSize = Header.PacketSize - HeaderSize;
//	if (PayloadSize == 0)
//		return false;
//
//	OutPacket.AddZeroed(PayloadSize);
//
//	if (ReceiveDesiredBytes(&OutPacket[HeaderSize], PayloadSize))
//		return true;
//
//	return false;
//}
//
//bool RecvWorker::ReceiveDesiredBytes(uint8* Results, int32 Size)
//{
//	int32 Offset = 0;
//
//	while (Offset < Size)
//	{
//		int32 NumRead = 0;
//
//		if (!Socket->Recv(Results + Offset, Size - Offset, NumRead))
//		{
//			return false;
//		}
//
//		if (NumRead <= 0)
//		{
//			return false;
//		}
//
//		Offset += NumRead;
//	}
//
//	return true;
//}
//{
	//uint32 PendingDataSize;
	//if (Socket->HasPendingData(PendingDataSize) == false || PendingDataSize <= 0)
	//	return false;

	//int32 Offset = 0;

	//while (Size > 0)
	//{
	//	int32 NumRead = 0;
	//	Socket->Recv(Results + Offset, Size, OUT NumRead);
	//	check(NumRead <= Size);

	//	if (NumRead <= 0)
	//		return false;

	//	Offset += NumRead;
	//	Size -= NumRead;
	//}

	//return true;
//}

// SendWorker
SendWorker::SendWorker(FSocket* Socket, TSharedPtr<PacketSession> Session) : Socket(Socket), SessionRef(Session)
{
	Thread = FRunnableThread::Create(this, TEXT("SendWorkerThread"));
}

SendWorker::~SendWorker()
{

}

bool SendWorker::Init()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Send Thread Init")));

	return true;
}

uint32 SendWorker::Run()
{
	while (Running)
	{
		SendBufferRef SendBuffer;

		if (TSharedPtr<PacketSession> Session = SessionRef.Pin())
		{
			if (Session->SendPacketQueue.Dequeue(OUT SendBuffer))
			{
				SendPacket(SendBuffer);
			}
		}

		// Sleep?
	}

	return 0;
}

void SendWorker::Exit()
{

}

bool SendWorker::SendPacket(SendBufferRef SendBuffer)
{
	if (SendDesiredBytes(SendBuffer->Buffer(), SendBuffer->WriteSize()) == false)
		return false;

	return true;
}

void SendWorker::Destroy()
{
	Running = false;
}

bool SendWorker::SendDesiredBytes(const uint8* Buffer, int32 Size)
{
	while (Size > 0)
	{
		int32 BytesSent = 0;
		if (Socket->Send(Buffer, Size, BytesSent) == false)
			return false;

		Size -= BytesSent;
		Buffer += BytesSent;
	}

	return true;
}