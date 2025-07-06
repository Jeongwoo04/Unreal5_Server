# Unreal Engine 5 + C++ IOCP Game Server Project

본 프로젝트는 **Unreal Engine 5 클라이언트**와 **C++ IOCP 기반 멀티스레드 게임 서버**를 연동하여 기본적인 MMO 게임 구조를 구현한 개인 포트폴리오입니다.

## 🧩 기술 스택

- **Unreal Engine 5** (Client)
- **C++ IOCP (I/O Completion Port)** 기반 서버 구조
- **Custom JobQueue / WorkerThread Pool** 기반 멀티스레딩
- **Google Protobuf** (패킷 직렬화)
- **Custom Room / Object / Actor 구조**

## 🚀 구현 기능 요약

### ✔️ Actor 입장 / 퇴장 관리
- 클라이언트 접속 시 Server에 Player Actor 생성
- 클라이언트 종료 시 Server의 Actor 소멸 처리
- `Room` 기반으로 Actor를 관리하며, 하나의 Room은 다수의 Actor를 포함

### ✔️ Actor 이동 동기화
- 클라이언트가 이동 시 이동 패킷 전송
- 서버에서 수신 후, 다른 클라이언트들에게 이동 브로드캐스트
- 단순 브로드캐스트 기반 구현

### ✔️ Room + JobQueue 구조
- Room 클래스는 JobQueue를 상속
- 클라이언트에서 요청된 작업은 해당 Room에 `DoAsync()`로 등록
- JobQueue의 작업은 등록된 Worker Thread 중 하나가 처리
- 멀티 클라이언트 환경에서도 Race Condition을 피하기 위한 구조

## 📌 구조 요약 다이어그램 (간략화)

[Client UE5]  
   ↕ (Protobuf Packet)  
[C++ IOCP Server]  
   └── Session  
         ↕  
       Room (JobQueue 상속)  
         ├── Player List
         └── JobQueue  
               ↳ Worker Thread가 처리  

## 📋 TODO & 고민 정리

### 1. 이동 동기화 정확도 개선

- 현재 구조: 클라이언트가 먼저 이동 → 서버로 이동 패킷 전송 → 서버는 그대로 다른 클라에 브로드캐스트
- 문제점: 클라 이동과 서버 이동 결과 간 Collision 불일치 발생 가능
- 고려 중인 개선안:
  - 서버에서 Collision 감지 후 판정 보정 (ex. 움직임 Rollback or 수정된 위치 전송)
  - Client Prediction + Server Reconciliation 구조로 전환

> 예시: Overwatch, Valorant 등은 서버 권한이 강한 authoritative 서버 구조를 사용하며,  
> 클라 예측과 서버 보정을 함께 사용함.

### 2. JobQueue 실행 스레드 구조 설계

- 현재: Room은 JobQueue를 상속 → 내부 작업은 WorkerThread가 실행
- 고민 중인 방향:
  - Room당 1개 Thread 전담 (싱글 컨슈머): 가장 안정적이지만 스레드 낭비 우려
  - Worker Thread Pool에서 JobQueue 락 기반 처리: 효율적이지만 병목 위험
  - Hybrid 구조 고려

> 방향 결정은 유저 수 / Room 수 / 타임크리티컬한 작업 양에 따라 결정 필요

## 📂 프로젝트 구조 예시

/Server  
├── GameSession.h / .cpp  
├── Room.h / .cpp  
├── JobQueue.h / .cpp  
├── Object / Player ...  
└── PacketHandler / Protocol / Protobufs  

/Client (Unreal Engine 5)  
├── GameMode  
├── NetworkManager  
└── ActorController  

## ✍️ Note

개인 프로젝트지만 실제 MMO 서버 아키텍처와 유사한 구조를 구성하고자 했습니다.  
멀티스레드 기반 IOCP 처리, Actor 관리, 이동 동기화, Room/Session 설계로 구성하고 있습니다.
앞으로도 Lock-free 구조, DB 연동, Skill 처리 FSM, 캐릭터 상태 보정 등을 계속 확장할 예정입니다.

## 🔧 빌드 및 실행

1. Visual Studio 2022 이상에서 Server.sln 빌드  
2. Unreal 프로젝트 S1.sln 빌드
3. 서버 실행 → 클라이언트 연결 테스트
