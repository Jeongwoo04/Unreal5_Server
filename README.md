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

<div style="page-break-before: always;"></div>

### 25.12.05 추가

# 🧱 확장 설계 요약 (Addendum)

본 프로젝트는 단일 서버(Room 기반) 구조에서 **안정적인 Active 1000 Dummy 환경**을 목표로 설계되었으며,  
아래는 기존 아키텍처 설계 이후 실제 부하 테스트 및 Worker 동작 분석을 통해 추가로 도출된 **확장성 개선 방향과 설계 트레이드오프**를 정리한 확장 보고서입니다.

해당 문서는 향후 **Multi-Room / Multi-Server(Sharding)** 구조까지 확장하기 위한 기술적 고민을 다루며,  
현 구조가 가지는 본질적 한계와 개선 방향을 명확히 설명합니다.

---

## 1. 기존 Worker 구조의 한계 관찰

### 🔹 Worker Thread 분리 구조
- **IO Worker**  
  - IOCP 완료 통지 처리  
  - `OnRecv → PacketHandler → Room JobQueue`
- **GameWorker**  
  - busy-wait 기반  
  - 64ms Tick budget(고정 Tick 기반)
- **SendWorker**  
  - 송신 Job 처리  
  - busy-wait 기반

---

### 🔹 확인된 구조적 문제점

#### 1) PopAll 폭증 시 GameWorker Tick 초과
- burst IO Job 발생 시 `PopAll` 규모가 예측 불가하게 급증
- GameWorker가 Tick Budget(64ms)을 초과하여 **Room Tick 지터 발생**
- 고정 Tick 기반 서버에서는 치명적 문제

#### 2) busy-wait 구조로 인한 CPU 독점
- GameWorker / SendWorker의 busy-wait 루프가 CPU를 독점
- OS 스케줄러가 IO Worker를 깨울 여유가 감소
- IOCP 완료 통지 지연 → 즉시 Room Tick 지연으로 이어짐
- IO bound burst 상황에서 누적 지연이 눈에 띄게 증가

➡ **단일 Room에서는 버티지만, Multi-Room / Multi-Server 환경에서는 명확한 확장 한계가 있음**

---

## 2. Worker Architecture 재설계 방향

### ✅ 2.1 IO Worker 확장 (Core × 2 Idle 보상 모델)

OS-friendly 스케줄링을 위해  
**CPU 코어 수 × 2** 수준까지 IO Worker를 확장하는 모델.

- IOCP Recv 폭주 시에도 Worker가 idle 상태에서 빠르게 복구됨  
- IO Completion 지연이 줄어들어 Room 직렬성 보장에 유리  
- CPU 효율성만 보면 오버스펙처럼 보일 수 있으나,  
  MMO 환경에서 IO 처리 지연은 로직 지연보다 훨씬 치명적

**🧩 Trade-off**
- Context Switching 증가 가능성  
- 그러나 Recv 폭증에 대응하는 데 IO 안정성이 훨씬 중요하여  
  **실제 MMO에서 충분히 감수할 가치가 있는 비용**

---

### ✅ 2.2 GameWorker / SendWorker Hybrid Loop 도입

busy-wait 제거 후 다음 구조로 전환:

- 명시적 Sleep  
- High-resolution Timer 기반 Tick  
- Sleep → Wake 과정에서 OS 스케줄러가 IO Worker를 우선적으로 처리할 여유 확보

**장점**
- CPU 독점 현상 제거  
- Tick 기반 로직의 안정성 향상  
- 부하 폭발 상황에서도 지터 최소화  
- IO/Logic 간 간섭 최소화

**🧩 Trade-off**
- Sleep에서 깨어난 직후 IO Job Burst를 만나면  
  **일시적 Jitter 발생 가능**
- 고정 Tick 기반 서버에서 Room 직렬성은 절대 해칠 수 없으므로  
  이 위험은 감수해야 하는 영역
- 궁극적으로 **Room 단위 스케일 조정**이 병목 조절의 핵심 포인트가 됨

---

### ✅ 2.3 Job Stealing 기반 부하 분산

CPU-bound 작업을 Room 공유 자원과 분리된 Task로 분류하여  
GameWorker idle 시간(64ms Tick 내부 여유)에 **다른 Queue의 Task를 Steal**하는 구조.

적합한 작업 예:
- AI 
- Raycasting
- Pathfinding(지역 단위)
- 충돌 판정의 전처리 계산(결과만 Room에 Echo)

**기대 효과**
- 특정 Room 또는 Queue에서 Burst 발생 시 부하 흡수
- 전체 시스템의 idle 시간 최대한 활용
- Multi-Room / Multi-Zone 기반 서버 확장성 증가

---

### 🧩 Job Stealing 방식별 Trade-off 정리

| 전략 | 장점 | 단점 및 위험 |
|------|-------|----------------|
| **A. 전역 TaskQueue에서 Steal** | - Room 일관성과 완전히 분리<br>- 순수 계산성 작업은 병렬성 극대화 | - Room Context가 필요한 작업은 처리 불가<br>- Sleep 타이밍을 잘못 잡으면 Tick 지연 발생 |
| **B. 결과만 Room에 Echo(Return)** | - Room 로직과 자연스럽게 통합<br>- Race Condition 위험 매우 낮음 | - Echo 작업 증가 → SendWorker 부하 가능성<br>- 결과가 Tick에 반영되는 시점이 뒤로 밀릴 위험 |
| **C. Object 단위 TaskQueue** | - 미세 단위 병렬화로 성능 최고 | - Queue 관리비용 증가<br>- 잘못 설계하면 Room 직렬성과 충돌 가능 |

**핵심 원칙**
> Job Stealing은 반드시 Room 내부 공유 상태를 변경하지 않는  
> **순수 계산형 CPU-bound Job**에만 적용해야 한다.

---

## 3. 향후 확장 고려 (AOI / Multi-Room / Sharding)

본 프로젝트는 단일 Room 스케일에서 안정성 확보를 목표로 했으나,  
부하 테스트를 진행하면서 자연스럽게 “수평 확장”을 위한 방향이 도출되었다.

### 🔹 3.1 AOI 기반 BroadcastGroup 도입
- 공간 분할 기반의 AOI(Area of Interest) 구조  
- Broadcast 영역 축소 → Send 폭주 방지  
- Hybrid Flush Queue와 자연스러운 연동 가능

### 🔹 3.2 Multi-Room 기반 샤딩 구조
- 단일 Room → Multi-Room → Multi-Server(Shard)로 전개  
- Room 단위 직렬 처리 모델을 그대로 유지하며  
  Zone/Shard로 Scale-Out 가능

### 🔹 3.3 Worker Ecosystem 확장
- IO / Logic / Send 간 스케줄링 간섭 최소화  
- Task 기반 분산 구조로 CPU 활용 극대화  
- IO 폭주, Skill 폭주, Monster Spawn 폭주 등  
  부하 패턴별 최적 분산 구조 가능

---

## 🎯 종합 결론

본 프로젝트는 단일 Room 기반 MMO 서버로서 충분히 안정적인 구조를 확보했으며,  
제출 이후 분석을 통해 **확장 가능한 Worker 재설계 방향**을 명확히 정리했다.

핵심은 다음과 같다:

- IO Recv 안정성이 최우선이므로 **IO Worker 확장**은 비용 대비 효과가 매우 크다.  
- GameWorker / SendWorker는 **busy-wait를 제거하고 Hybrid Loop**로 전환해야 한다.  
- Job Stealing은 강력한 확장성 도구지만, **Room 직렬성을 유지하는 선에서만** 제한적으로 사용해야 한다.  
- 향후 AOI · Multi-Room · Sharding 구조로 자연스럽게 확장 가능하다.

이 확장 설계 Addendum은  
“현재 구조의 한계 + 향후 확장 가능성”을 명확히 제시하여  
실제 MMO 서버 엔지니어링 관점의 완성도를 높이는 역할을 수행한다.