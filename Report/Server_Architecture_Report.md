# 서버 아키텍처 설계 보고서  
**Server Architecture Design Document**  

---

# 문서 개요
본 문서는 MMO 서버 프로젝트의 전체 아키텍처와 각 설계 의사결정의 근거를 정리한 기술 보고서입니다.
아키텍처 포트폴리오(PPT)와 최적화 보고서(PDF)는 이미 별도로 존재하며, 본 문서는 다음을 중점적으로 다룹니다.

- **왜 이 구조를 선택했는가?**  
- **여러 구조 중 어떤 기준으로 선택했는가?**  
- **문제 → 설계의도 → 선택구조 → 선택이유 → 기대효과** 의 형태로 핵심 설계를 설명  
- 프로젝트 전반에서 배운 점과 확장 방향 제시

# 프로젝트 개요
본 프로젝트는 Unreal 기반 MMO 서버를 아키텍처 레벨에서부터 구현하며,  
**대규모 동시 처리 환경을 안정적으로 운영할 수 있는 서버 엔지니어링 역량을 검증하기 위한 프로젝트입니다.**
엔진·네트워크·Flush Queue·Game Logic·Data-driven 설계까지 전체 파이프라인을 직접 설계/구현하였고,
최종적으로 Active Dummy 1000 명 기준 Room Tick 안정화(0.85ms)와 WSASend 병목 제거까지 달성하였습니다.

- 개발 기간: 2025.07 ~ 2025.11 (약 4개월)
- 역할: 서버 시스템 전반 개발 (맵/Room/AI/Collision/Network I/O)
- 기여도: 100% (개인 프로젝트)
- 개발 환경: C++20, Unreal Engine 5, C++14 IOCP ServerCore, Protobuf, Windows 11

### 담당 기능 요약
- IOCP 서버코어 구축
- JobQueue·JobTimer 기반의 스케줄링 및 직렬화 구조 구현
- Single-thread Room Logic + Worker Thread 분리 모델 최적화
- CollisionMap 기반 지형 충돌 시스템 구축
- Monster/AI 및 Raycasting 시스템 개발
- SkillSystem 설계 및 개발
- Flush Queue를 활용한 대규모 서버 안정화 설계
- 네트워크 패킷 구조 및 Protobuf 설계
- Tick 기반 서버 아키텍처 구성

---
<div style="page-break-before: always;"></div>

# 0. Summary

본 프로젝트는 Unreal 기반 MMO 서버를 아키텍처부터 네트워크, AI, Skill, Broadcast까지  
**전체 파이프라인을 단독 개발하여 하나의 동작 가능한 서버 엔진을 완성한 프로젝트**입니다.

핵심 성과는 다음과 같습니다:

- **Room 단일 스레드 구조 구축** → 로직 Race Condition 0  
- **FlushQueue 기반 전송 파이프라인** → WSASend 병목 제거  
- **Broadcast 최적화** → Active Dummy 1000명 기준 안정 처리  
- **Room Tick 평균 0.85ms 유지**  
- **IOCP 네트워크 모델 직접 구현**  
- **Skill/Monster/AI/Map/Stat 전체 Data-driven 재설계**  
- **이벤트 정렬 기반 Serialize 구조 도입(Event Ordering Policy)**  

이 문서는 단순한 기능 설명을 넘어  
**문제 정의 → 설계 의도 → 선택한 구조 → 선택 이유 → 기대 효과**  
의 체계로 기술적 의사결정의 근거를 명확히 정리합니다.

---
<div style="page-break-before: always;"></div>

# 1. 서버 전체 구조 개요 (Overview)

본 서버는 **Room 단위 Single-thread Game Loop**를 기반으로 한다.  
모든 클라이언트 입력은 IOCP 기반 네트워크 스레드에서 수신되어  
**Job 단위로 Room(JobQueue)에 전달**되며, Room은 이를 순차 처리해 게임 로직을 실행한다.

- **Room Update 주기:** 100ms  
- **Job 분배 방식:** IO 스레드 → Room JobQueue  
- **전송 구조:** Immediate / Defer FlushQueue → SendQueue(SendJob) → Send Worker 스레드

### 다이어그램
```
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

### 섹션 요약
- Room은 독립된 단일 스레드로 실행 → 동시성 비용 0  
- Packet 송신은 IO Thread로 완전 분리  
- Immediate/Defer Flush 설계로 반응성과 부하 제어 모두 달성

---
<div style="page-break-before: always;"></div>

# 2. **Single-thread Room 구조 (JobQueue) 선택 이유**  

## 2.1 문제
멀티스레드 기반 Room 구조는 다음과 같은 대규모 비용을 가진다.

- 지속적인 Lock 경쟁  
- Race Condition·Deadlock 위험  
- 디버깅 난이도 증가  
- 메모리 캐시 비효율  
- Multi-Room 구조를 적용할 경우 Room 간 책임 분리 및 로드 밸런싱 기준이 복잡해짐

## 2.2 설계 의도
- **게임 로직의 순차성 보장**  
- **논리적 일관성 유지**  
- **장애 재현이 가능한 디버깅 친화적 구조** 
- **Room 단위 병렬 확장이 가능한 수평 확장 구조**

## 2.3 선택 구조
- Room 단위 **Single-thread 실행 모델**
- IO 스레드는 Room 로직에 직접 개입하지 않고 **Job만 Push**
- Room 간 데이터 공유 완전 차단 → **구조적 Lock-Free**

## 2.4 선택 이유
- MMO 서버의 로직은 대부분 **가벼운 연산의 고빈도 처리**
- 로직 내부 병렬화보다 **실행 순서 보장이 훨씬 중요**
- 병렬성은 로직 내부가 아닌 **Room 단위 분할로 확보**
- 멀티스레드 동기화 비용과 복잡도를 구조적으로 제거 가능

## 2.5 기대 효과
- Room 내부 Race Condition 구조적으로 불가능
- Room Tick 안정화(1ms 이하)  
- 디버깅 및 분석 난이도 대폭 감소
- 운영 중 장애 재현 가능성 극대화

---
<div style="page-break-before: always;"></div>

# 3. JobQueue 실행 모델  

## 3.1 역할 정의
JobQueue는 단순한 작업 큐가 아니라,  
**내부의 모든 로직 실행 순서를 보장하는 단일 실행 컨테이너이다.**

Room과 SendQueue는 JobQueue를 직접 상속받으며,  
**Single-thread Room을 구성하기 위한 필요충분조건이 바로 JobQueue이다.**

## 3.2 설계 의도
- 내부 로직의 **완전한 직렬 실행 보장**
- IO 처리와 게임 로직 실행의 **구조적 분리**
- 모든 상태 변화가 **절대적인 실행 순서를 갖도록 설계**

## 3.3 선택 구조
- 모든 Room과 내부 SendQueue는 **JobQueue를 직접 상속**
- IO 스레드는 패킷을 Job으로 변환하여 Room(JobQueue)에 PushOnly
- GlobalQueue에 등록된 Room(JobQueue)을 Worker가 Pop
- Worker는 해당 Room의 `_jobs`를 PopAll 하여 **단일 스레드에서 Execute**
- 전송 전용 **SendQueue(JobQueue)** 를 별도로 두어 병렬 처리

## 3.4 선택 이유
- Room 내부 로직은 항상 **단 하나의 실행 흐름만 허용**
- IOCP, Worker, Room 간 직접적인 Lock 경합 구조 제거
- SendQueue를 통해 로직 실행과 네트워크 전송의 책임을 명확히 분리
- 전체 시스템은 병렬로 동작하되, **JobQueue 내부는 항상 직렬 실행 유지**

## 3.5 기대 효과
- Room 내부 Race Condition 완전 제거
- 모든 게임 상태 변화의 실행 순서 보장
- Room 단위 병렬 처리 구조 확립
- 대규모 동접 환경에서도 예측 가능한 서버 동작 확보

---
<div style="page-break-before: always;"></div>

```
void JobQueue::Push(JobRef job, bool pushOnly)
{
	const int32 prevCount = _jobCount.fetch_add(1);
	_jobs.Push(job); // LOCK

	if (prevCount == 0)
	{
		// 실행중인 JobQueue가 없으면 실행
		if (LCurrentJobQueue == nullptr && pushOnly == false)
			Execute();
		else
		{
			// GlobalQueue로 넘겨 여유있는 Worker가 실행
			GGlobalQueue->Push(shared_from_this());
		}
	}
}
```

---
<div style="page-break-before: always;"></div>

# 4. Flush Queue & Hybrid Flush Queue 설계 (최적화 결과 반영)  

## 4.1 문제
Scenario 2 벤치마크에서  
- BroadCast 상황에서 **SendQueueDelay가 급증**  
- N명의 Player, NearByPlayer K 인경우 SendCount = N x K (N^2에 비례)
- Client Tick이 ServerTick보다 짧아 서버는 받은 모든 패킷을 즉시 Broadcast할 경우 IO 병목이 발생
- Player 입력에 대한 반응이 Flush 단계에서 1회 보내질 경우 최대 2Tick의 지연을 체감할 수 있음
- WSASend 등록 지연 발생

## 4.2 설계 의도
- Client에서 보내는 모든 패킷을 판정을 하되 결과는 모아 한 번만 전송
- 반응성이 중요한 Packet의 우선순위를 2단계로 설정 후 Immediate / Defer로 구분
- Broadcast 전송 병목 제거  
- 전송 파이프라인을 완전 병렬화
- 이벤트 정렬 정책(Event Ordering Policy)에 따라 Serialize


## Event Ordering Policy
- 이벤트는 Event Ordering Policy(Spawn → Move → Skill → Hit → Die → Despawn)에 따라
- 정렬되어 Serialize되므로 동일 Tick 내 모든 클라이언트는 동일한 재생 순서를 보장받는다.
- Cast Start 단계에서 PosInfo를 포함하여, 클라이언트에서의 재생 위치·방향 정보가 오차 없이 표현되도록 한다.
- Immediate 처리 이벤트: CastStart → CastCancel → Move  
- Defer 처리 이벤트: Spawn → Move → SkillEvent → Hit → Die → Despawn  

**동일 Tick 내 이벤트 재생 순서를 통일하여 클라이언트 화면의 일관성을 확보합니다.**

## 4.3 선택 구조
1. FlushQueue
2. Immediate Flush  
3. Defer Flush  
4. **FlushPkt & Merge & Serialize**  
5. **SendQueue로 Push**  
6. IO Thread가 WSASend 수행

## 4.4 선택 이유
- IO Thread가 전송을 전담하여 Room Tick에서 전송 비용 완전히 제거  
- Immediate/Defer의 장점은 유지하며 병목만 해결  
- Room은 로직만 수행 → 낮은 변동성

<div style="page-break-before: always;"></div>

## 4.5 기대 효과 (측정값 포함)
- Room Tick 평균: **0.85ms**  
- SendQueueDelay: **2~3ms로 안정화**  
- Broadcast 환경에서도 일정한 Frame 유지

```
enum class Type
{
	SPAWN, MOVE, HIT, DIE, DESPAWN,
	CAST_START, CAST_CANCEL,
	CAST_SUCCESS, SKILL_ACTION
};

struct FlushQueue
{
	ObjectRef object;
	Type type;
	optional<Protocol::S_SKILL_EVENT> eventInfo;
};

public:
	vector<FlushQueue> _immediateFlushQueue; // C_Packet
	vector<FlushQueue> _deferFlushQueue; // Room Object Update
	shared_ptr<JobQueue> _sendQueue;

message S_IMMEDIATE_FLUSH
{
	optional S_SKILL skill_pkt = 1;
	optional S_MOVE move_pkt = 2;
}

message S_DEFER_FLUSH
{
	optional S_SPAWN spawn_pkt = 1;
	optional S_MOVE move_pkt = 2;
	optional S_SKILL skill_pkt = 3;
	optional S_HIT hit_pkt = 4;
	optional S_DIE die_pkt = 5;
	optional S_DESPAWN despawn_pkt = 6;
}
```

---

<div style="page-break-before: always;"></div>

# 5. SpatialGrid 선택 이유  
**(SpatialHash(map) vs SpatialGrid(vector) 비교 및 Quadtree의 확장 고려)**

## 5.1 문제
Room 내부에서 충돌 탐색을 위해 공간 분할 구조가 필요했으며  
다음과 같은 문제가 확인되었다:

- **SpatialHash(map 기반)**  
  - 해시 충돌 및 map 구조 특성으로 인해 캐시 효율이 낮음  
  - 삽입/삭제 비용이 일정하지 않아 Tick 변동성 발생  

- **Quadtree**  
  - 동적 분할/병합 비용이 큼  
  - Collision처럼 “작고 고정된 범위 탐색”에는 과도한 구조  
  - 현재 Room 스케일에서는 장점을 활용하기 어려움  

즉, Room 내부 충돌 판정은 **간단하고 비용이 예측 가능한 구조**가 필요했다.

---

## 5.2 설계 의도
- **항상 일정한 탐색 비용 (O(1))**  
- **연속 메모리 기반의 높은 캐시 효율 확보**  
- **Room 단일 스레드 구조와 자연스럽게 결합**  
- **삽입/삭제가 빈번해도 비용 흔들림이 없는 구조**

---

## 5.3 선택 구조
- 2D 공간을 고정 크기 셀로 나눈 **SpatialGrid (vector 기반)**  
- 내부는 1D Flat Vector로 구성  
- Actor 이동 시: 이전 셀 pop → 새 셀 push  
- 충돌 탐색 시: 현재 셀과 주변 인접 셀만 조회

---


## 5.4 SpatialGrid 선택 이유 (핵심)

### (1) 현재 스케일에서는 Quadtree가 과한 구조
Quadtree는 Actor 분포가 비균일하고, 넓은 AOI 탐색이 필요한 경우 유리하다.  
하지만 **Room 충돌 검사처럼 좁고 밀도가 일정한 환경**에서는 오히려 비용이 더 든다.

| 항목 | Quadtree | SpatialGrid(vector) |
|------|----------|---------------------|
| Actor 분포 | 불균일일수록 유리 | 균일한 분포에서 최적 |
| 삽입/삭제 | 분할/병합 비용 큼 | O(1) pure push/pop |
| 메모리·캐시 | 노드 기반 → 캐시 효율 낮음 | 완전 연속 메모리 |
| 구현 난이도 | 높음 | 매우 낮음 |
| Room 충돌 검사 | 과도한 구조 | ✔ 최적 |

현재 Room 구조는  
- 고정 크기  
- Actor 밀집도 안정적  
- 탐색 범위 일정  

즉, Quadtree의 장점(동적 분할·비균일 처리)이 필요한 조건이 현재 Room 구조에는 존재하지 않는다.

---

### (2) SpatialHash(map) 대비 압도적인 캐시 효율  
직접 벤치마크 결과:

| 구조 | 특징 | 20×20 영역 탐색 |
|------|-------|----------------|
| SpatialHash(map) | map 기반, 캐시 미스 많음 | **0.9652ms** |
| **SpatialGrid(vector)** | 완전 연속 메모리 | **0.0870ms** |

약 **11배** 가까운 차이가 났으며,  
Room Tick마다 충돌 검사가 반복되는 구조에서는 누적 비용이 매우 커진다.

---

### (3) 향후 AOI 확장 시에도 재활용 가능  
현재는 충돌 중심 Grid지만, 나중에는 다음처럼 확장할 수 있다:

- Grid 단위를 조정해 AOI용 BroadcastGroup으로 활용  
- Grid 상위 레벨에 Quadtree 삽입해 넓은 AOI 탐색 구조로 확장  

---

## 5.5 기대 효과
- **Collision 탐색 성능 대폭 향상**  
- Tick 단위 충돌 비용이 약 11배 감소  
- SpatialGrid 기반 Move Logic의 비용이 완전 O(1)로 고정  
- 캐시 일관성 향상으로 Room Tick 변동성 감소
- 구조가 단순해 AOI / BroadcastGroup 구조 전환 시 단위 셀만 변경하면 됨

---

<div style="page-break-before: always;"></div>

# 6. SkillSystem 설계

Skill 처리의 중복을 제거하기 위해  
Object 기반이 아닌 **System 기반 처리**를 도입하였다.
## 6.1 문제
- Object 별 Skill 로직의 중복
- 기본적인 동작에 대한 중복
- Object Update 시 스킬 사용 순서가 깨짐

## 6.2 설계 의도
- **공통 행동(Action): Move Attack Spawn Buff 등**을 조합한 Skill을 Data 기반으로 사용
- **높은 재사용성 및 Data-driven 친화적**
- **스킬 사용 순서 보장 및 Action Delay로 클라이언트 연출 동기화 최적화**

## 6.3 선택 구조
- 공통 Action을 갖는 Skill 조합으로 SkillInstance 활용
- Action 단위 조합 + JSON 기반 Data-driven  
- Skill 사용시 Execute 로직으로 Casting / Non-Casting 구분
- Update시 Cast Elapsed / Action Delay Elapsed 판정 후 적용
- HandleAction을 통한 공통 Action 처리
- 특수 Monster, 플레이어 클래스 별 Custom Action 등록 후 사용 가능

## 6.4 선택 이유
Object 단위로 Skill 로직을 구현할 경우, 로직이 중복되고 객체 순회 순서에 따라 Skill Update 순서가 달라지는 문제가 있다.

반면, SkillSystem 과 SkillInstance를 통한 Skill, Action 처리
- SkillSystem의 SkillInstance로 Room 내 사용된 Skill의 순서가 보전됨
- 동일한 Tick의 처리로 일관성 있음
- Flush Packet Event Ordering Policy와 연계 가능
- Delay 시스템으로 Client 연출 동기화

## 6.5 기대 효과
- Data-driven 설계와 매우 궁합이 좋음
- ECS에서 System이 하는 역할을 OOP 환경에 맞춰 구현
- Monster AI 도 System으로 분리해 Data-driven 친화적으로 확장 가능
- 클라이언트 연출과 Timing 동기화 품질 향상

---

<div style="page-break-before: always;"></div>

# 7. Data-driven 설계 도입 배경 및 효과

Object, Map, Skill, Stat 등 핵심 콘텐츠는 JSON 기반으로 구성되며  
서버는 “코드가 아닌 데이터”에 의존하도록 설계했다.

### 문제
- Stat/Skill/Object 설정이 코드에 박혀 있어 수정 비용이 컸다.
- 객체 간 로직 중복이 발생하고 유지보수가 어려웠다.

### 설계 의도
- 파라미터 기반 조정이 가능하도록 설계하였다.
- Hotfix 및 밸런스 패치 속도 향상을 도모하였다.
- 콘텐츠 팀과의 협업을 위해 구조를 개선하였다.

### 기대 효과
- 재사용성이 크게 향상됨.
- Skill, AI, Monster 등 주요 시스템을 Data-driven 방식으로 확장하기 용이함.

---

<div style="page-break-before: always;"></div>

# 8. 네트워크 모델 개선 방향(AOI 확장 계획)
**All Broadcast → AOI BroadcastGroup 확장 설계**
Room 단위의 논리적 공간을 Zone 단위(AOI) 형태로 분리해 최종 단계의 부하 최적화 방식으로 확장

AOI BroadcastGroup 모델은 초기 설계에서부터 목표로 두었던 방식이 아니라, 개발 과정에서 실제 병목 지점을 파악하며 자연스럽게 도달한 구조이다.
FlushQueue 기반의 직렬화 파이프라인은 이러한 AOI 확장과 구조적으로 잘 결합되며, 기존 설계를 유지한 채 최종 확장 단계로 적용할 수 있다.

### 현재 구조
- 모든 플레이어 위치/State 패킷은 All Broadcast  
- 간단하고 안정적  
- Room 스케일 작을 때는 매우 효율적

### AOI 시도와 문제
- CollisionMap과 동일한 셀 크기를 사용해 과도한 탐색 발생  
- Broadcast Range 기반 캐싱을 고려하지 못함  

### 향후 구조
- Room 내부를 AOI Cell로 분할  
- Cell마다 BroadcastQueue를 보유 (현재 FlushQueue 구조 확장)  
- SpatialGrid 기반 주변 탐색으로 최소 단위 패킷 전송  
- delta shift, base offset 등 활용한 연산 최적화를 겸한 설계 확장

# 9. 느낀 점 & 개선 방향

### 배운 점
- Single-thread Room은 단순하지만 강력한 구조  
- SpatialGrid가 실제 환경에서 가장 안정적인 공간 분할 방식임  
- FlushQueue 활용이 Broadcast 병목 해결의 핵심  
- MMO 서버에서 가장 중요한 기준은 “Room Tick 안정성”

### 향후 개선
- Room 간 자동 부하 분산  
- Skill/AI 완전 Data-driven(ECS적 관점으로 개선)  
- Monster BehaviorTree 적용  
- Packet Priority 구조 확장 (정교한 우선순위 기준 확립)
- Multi-Server 구조 설계 (Inter-Server)

---

<div style="page-break-before: always;"></div>

# Epilogue

본 프로젝트를 통해 Unreal 기반 MMO 서버의 핵심 구조와 설계 선택을 검증했습니다.  
IOCP, 단일 스레드 Room, SpatialGrid, FlushQueue, SkillSystem, Data-driven 설계 등  
주요 기술적 판단은 모두 성능과 안정성 측정 결과를 기반으로 이루어졌습니다.

**주요 성과**
- Room Tick 0.85ms 달성  
- WSASend 병목 제거  
- Active Dummy 1000명 환경에서 안정적인 Broadcast 처리  

이러한 성과는 설계 선택의 타당성을 명확히 보여주며,  
향후 Multi-Server 확장, AOI Broadcast, Data-driven 생태계 구축 등  
더 큰 규모의 MMO 서버 아키텍처로 확장할 수 있는 기반을 마련합니다.

**핵심 원칙**
- 구조는 단순해야 한다  
- 문제는 데이터로 증명해야 한다  
- 성능은 의도가 아닌 결과로 말해야 한다  
- 아키텍처는 기능이 아니라 판단의 근거가 핵심이다

본 보고서는 서버 설계와 구현의 기술적 근거를 체계적으로 기록한 문서이며,  
향후 확장성과 안정성을 위한 기준점으로 활용될 수 있습니다.

