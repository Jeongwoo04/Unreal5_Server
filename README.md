# UE5 + C++ IOCP 기반 대규모 MMO 서버 포트폴리오

Unreal Engine 5 클라이언트와 C++ IOCP 기반 멀티스레드 게임 서버를 직접 설계·구현한 
대규모 실시간 MMO 서버 아키텍처 포트폴리오 프로젝트입니다. 
단순 기능 구현이 아닌, 구조 설계, 병목 분석, 대규모 부하 최적화에 초점을 맞추었습니다.

---

## ✅ 핵심 성과 (Key Results)

- **Active 1000 환경 SendDelay 6594ms → 27ms (약 99.6% 개선)**
- Broadcast 구조의 N² 병목 문제, Hybrid Flush Queue 도입으로 구조적 해결
- Protobuf 기반 Batch Packet 및 Event Ordering 정책 적용
- Data-driven Action 기반 Skill System 설계 (확장성 극대화)
- Worker Thread 재배치로 IO 경합 및 처리 지연 최소화
- HUD 기반 실시간 서버 프로파일링 시스템 구축

---

## 🧩 Tech Stack

- Client: Unreal Engine 5 (C++)
- Server: C++ / IOCP / Multi-thread
- Network: Google Protobuf, Custom Packet Pipeline
- Concurrency: Custom JobQueue (Room 직렬 처리 보장), Worker Thread Pool (IO/Logic 분리)
- Architecture: Room-Based Architecture (Scale-up 지향), Data-driven Skill System
- Profiling: In-Game HUD Real-time Server Profiler

---

## 🏗️ Server Architecture Overview

```scss
        [Client] 
            ↓ 
    [Server TCP Stack] 
            ↓ 
[IOCP registered Session] 
            ↓  OnRecv() 
    [IO Worker Thread] 
            ↓ PacketHandler (Packet → Job) 
     [Room JobQueue] → Single-thread 
            ↓ 
       Execute Jobs 
            ↓ 
  [t0. Scheduled Update Loop] 
    ↓                 ↓ 
  Immediate         Defer  
    ↓                 ↓ 
    [Merge & Serialize]  →  [SendQueue (SendJob)]  →  [Send Worker Thread] 
             ↓ 
        Execute Jobs 
            ↓ 
  [t1. Scheduled Update Loop] 
          . . .  
```

<div style="page-break-before: always;"></div>

---

본 프로젝트는 Room 기반 아키텍처를 중심으로 IOCP 네트워크 계층과 멀티 Worker Thread 구조를 통해 
대규모 동시 접속 환경에서도 안정적으로 동작하도록 설계되었습니다.

네트워크 이벤트는 IOCP → Session → Room → Object → SkillSystem 흐름으로 전달되며, 
Room 내부 로직은 JobQueue 기반 직렬 처리로 데이터 충돌을 원천 차단하며,
락 사용을 최소화한 구조를 통해 멀티스레드 성능 하락을 방지했습니다.

---

## ⚙️ 주요 시스템 구성

✅ **Room + JobQueue 기반 직렬 처리**
- Room 단위 JobQueue 기반 처리로 Race Condition 원천 최소화
- Worker Thread Pool 기반 병렬 처리 구조 설계 (IOCP 기반)

✅ **Data-driven Skill System**
- Skill을 Action 단위의 조합(Composition) 구조로 설계
- Skill Instance 기반 이벤트 처리 및 FSM 분리로 구조적 결합도 제거

✅ **Hybrid Flush Queue (Immediate / Defer)**
- Player 입력은 Immediate Queue, Server 상태 동기화는 Defer Queue 처리
- 대량 Broadcast Burst 완화 및 Tick 안정화 기여

✅ **Protobuf Batch Packet + Event Ordering**
- Spawn → Move → Skill → Hit → Die 순서 보장 (클라이언트 일관성)
- 패킷 타입 통합 및 네트워크 비용 절감

---

## 🚀 성능 최적화 핵심

### 1. Broadcast 병목 개선 (Hybrid Flush Queue)

기존 구조에서 Room 내 모든 플레이어에게 개별 Send가 발생하며 N² 급 성능 저하가 발생했습니다. 
이를 **Hybrid Flush Queue (Immediate / Defer)** 구조로 재설계하여 개선하였습니다.

- **Immediate Queue:** 중요 이벤트(플레이어 입력 등) 즉시 전송
- **Defer Queue:** 일반 이동 데이터 Tick 단위로 묶어서 전송
- **결과:** Active 1000 환경 기준 평균 SendDelay **6594ms → 27ms 수준 (약 99.6% 개선)**

### 2. Event Ordering 기반 Batching

**Protobuf**를 활용, S_IMMEDIATE_FLUSH_PKT 같은 **Type별 패킷을 통합한 Batch Packet**을 구성하고, 
Event Ordering 정책을 적용하여 **클라이언트 연출의 일관성**을 확보했습니다.

---

<div style="page-break-before: always;"></div>

## 🎯 Skill System (Data-driven Composition)

- 모든 스킬은 **Data-driven Composition** 기반으로 정의
- Action, Projectile, Hit, Damage, Buff 등 모든 요소를 모듈화
- 스킬 발동/판정/효과 처리 시 **Event Ordering**을 고려한 이벤트 기반 처리 구조
- 서버 FSM 기반 상태 처리와 완전 분리된 스킬 처리 구조
- 신규 스킬 추가 시 코드 수정 없이 데이터 추가만으로 확장 가능

---

## 📊 실시간 서버 프로파일링

- In-Game HUD 기반 **Room/Worker/Object/Network 상태** 실시간 시각화
- Tick 처리 시간, JobQueue 적체, Send 큐 적재량 등 핵심 지표 실시간 출력
- 부하 테스트 중 병목 위치 및 Worker 스레드 부하 즉시 확인 가능

---

## 🧪 부하 테스트 환경

- Dummy Client 1000 동시 접속
- Monster, Skill, Projectile, Broadcast 동시 처리 환경
- 패킷 폭증 상황에서도 서버 다운 없이 안정적 유지

---

## 📁 프로젝트 목적

본 프로젝트는 단순 게임 기능 구현이 아닌,

- 실무 수준의 서버 아키텍처 설계 역량
- 대규모 동시 처리 최적화 경험
- 병목 분석 및 구조적 개선 능력

을 검증하며 **'데이터 흐름이 막히는 곳을 집요하게 파고드는'** 저의 개발 철학을 담은 **실전형 서버 포트폴리오**입니다.

---

## 🔚 마무리

본 프로젝트를 통해 IOCP 기반 C++ 서버 구조 설계, 멀티스레드 동기화 문제 해결, 
대규모 Broadcast 최적화, 실시간 서버 프로파일링까지 
실무 서버 엔지니어에게 요구되는 전반적인 역량을 직접 설계하고 구현하였습니다.