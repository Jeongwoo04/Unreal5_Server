# UE5 + C++ IOCP MMO Server

Unreal Engine 5 클라이언트와 C++ IOCP 기반 멀티스레드 게임 서버를 직접 설계·구현한 실시간 MMO 서버 아키텍처 포트폴리오입니다. 구조 설계, 병목 분석, 대규모 부하 최적화에 초점을 맞추었습니다.

---

## 📌 Quick Links & Demo
- **포트폴리오 PDF:** 제출된 자료 Readme에서 확인 가능
- **시연 영상:** 제출된 자료 Readme에서 확인 가능

---

## 🚀 핵심 성과 (Key Results)
- **Active 1000 환경 SendDelay:** `6594ms → 27ms` (약 **99.6% 개선**)
- **Broadcast N² 병목 완화:** Hybrid Flush Queue (Immediate / Defer) 도입으로 Burst 상황의 SendJob 대기 병목 개선
- **대규모 동시접속 환경 검증:** Active **5000 Session**, **250 Rooms** 환경에서 Room 분산 처리 구조 검증
- **실제 LAN 환경 서버 송신 병목 개선:** Scatter-Gather 적용으로 WSASend System Call을 초당 약 **8~9만 → 최대 5만 회**로 감소 (총 전송 Bytes는 동일)
- **데이터 일관성:** Room + JobQueue 기반 직렬 처리 및 Protobuf Event Ordering 적용
- **확장성:** Data-driven Composition 기반 Skill System 설계
- **캐시 지역성:** TypeChunk 기반 객체 연속 배치로 메모리 접근 지역성 개선

---

## 🛠️ Tech Stack
- **Client:** Unreal Engine 5 (C++)
- **Server:** C++ / IOCP / Multi-thread
- **Network:** Google Protobuf, Custom Packet Pipeline
- **Concurrency:** Custom JobQueue (Room 직렬 처리), Worker Thread Pool (IO/Logic/Send 분리)
- **Profiling:** In-Game HUD Real-time Server Profiler, VTune, Wireshark 등

---

## 🏗️ Architecture & Core Systems
```
[Client]
   ↓
[IOCP Session]
   ↓
[IO Worker]
   ↓
[Room JobQueue]
   ↓
[Scheduled Update]
   ↓
[Hybrid Flush Queue]
   ├─ Immediate → [Session SendQueue]
   └─ Defer     → [Flush]
                     ↓
              [Send Worker]
                     ↓
           [Scatter-Gather WSASend]
```

- **Room + JobQueue:** Room 단위 직렬 처리로 Race Condition 원천 차단 및 락 최소화
- **Hybrid Flush Queue:** 플레이어 입력(Immediate)과 서버 상태 동기화(Defer)를 분리하여 패킷 폭증 상황(Burst) 완화 및 Tick 안정화
- **Data-Driven Skill System:** Action, Projectile, Hit, Damage 등을 모듈화하여 코드 수정 없이 데이터로 스킬 확장

---

## ⚙️ Build & Run Guide

### 1. Requirements
- Visual Studio 2022 (C++20 이상 지원)
- Windows SDK 10.0 이상 (IOCP 사용을 위한 Windows 환경 필수)
- Unreal Engine 5.x

### 2. Build Steps
1. **Source Download**
   - 상단의 **[Code] → [Download ZIP]** 버튼을 눌러 소스 코드를 다운로드 후 압축 해제
2. **Server Build**
   - 솔루션 파일(`.sln`)을 Visual Studio로 오픈
   - Data/ 내부의 Config 와 각종 .json 파일로 서버에 필요한 값들 조정
   - `GameServer` 프로젝트 우클릭 → 속성 → 디버깅 → 작업 디렉터리를 `$(TargetDir)` 설정
   - 빌드 구성을 **Release / x64**로 설정 후 `ServerCore` → `GameServer` → `DummyClient` 순으로 빌드 진행
3. **Client Run**
   - Unreal Engine 5 프로젝트 에디터 실행
   - 서버 IP 및 Port 설정 후 접속 (Dummy Client 연동 테스트 가능)
   - .dll 혹은 .lib 등 각종 라이브러리 필요할 수 있음. 시연영상으로 해당 부분을 대체함
4. **Dummy Client**
   - C++ 더미 클라이언트는 `.sln` 내부에 빌드시 이용 가능
   - C#/.Net 더미는 DotNetDummy 내부의 `.sln` 에서 **Release / x64**로 설정 후 빌드 진행

---

## 🔭 Future Improvements

- 자주 참조되는 데이터를 Component 단위로 분리하여 데이터 접근 패턴을 개선하고, 캐시 효율 개선 검증
- 게임 데이터의 중요도와 저장 주기를 고려한 영속성 전략 및 데이터베이스 설계(MySQL/MSSQL)