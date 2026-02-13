# FSM 리팩토링 실행계획 (Audio 제외)

## 1. 범위
- 포함: 발전기 FSM 상태 일관성, 이벤트 직렬화, 상태 전이 결정론 확보
- 제외: mp3/DFPlayer 관련 변경 일체

## 2. 목표
- 상태 변경 경로를 단일화해 race condition 제거
- TAG_ON/OFF와 LEVER_STEP 동시 입력 시 결과를 항상 동일하게 고정
- 기존 플로우(Setting/Ready/Activate/StarterFinish/Repaired 계열) 동작 유지

## 3. 현재 구조 기준 리스크
- 상태 전이 트리거가 여러 파일에 분산되어 있어 순서 경쟁 가능
- 모드 함수 포인터(`ptrCurrentMode`, `ptrRfidMode`) 직접 변경 지점이 다수
- 엔코더 ISR과 RFID 로직이 공유 상태에 동시에 영향

참고 코드:
- `GameFSM.cpp:12` (`executeCommand`)
- `GameFSM.cpp:65` (상태 변경)
- `GameFSM.cpp:184` (모드 포인터 변경)
- `rfid.ino:171` (`BatteryPackCharge`)
- `rfid.ino:225` (`BatteryFinish`)
- `rfid.ino:248` (`ptrCurrentMode = StarterActivate`)
- `encoder.ino:17` (`updateEncoder` ISR 경로)

## 4. 아키텍처 원칙
- FSM 단일 소유자(싱글 라이터): 상태 변경은 한 실행 컨텍스트에서만 수행
- 입력 소스(RFID/엔코더/타이머/WiFi)는 상태를 직접 변경하지 않고 이벤트만 전달
- 상태 전이 우선순위를 명시해 동시 입력에도 결정론 보장

## 5. 구현 단계

### 5.1 이벤트 모델 정의
- `FsmEventType` 추가:
  - `EVT_TAG_ON`
  - `EVT_TAG_OFF`
  - `EVT_LEVER_STEP`
  - `EVT_TIMEOUT`
  - `EVT_NET_CMD`
- 필요 시 payload 포함 (`cmd`, `timestamp`, `source`)

### 5.2 단일 FSM 처리 루프 구성
- FSM 이벤트를 소비하는 단일 루프(또는 FreeRTOS 단일 Task) 구성
- `gameFSM.executeCommand()`는 이 루프 내부에서만 호출
- 외부 파일에서 `executeCommand()` 직접 호출 금지

### 5.3 상태 컨텍스트 캡슐화
- FSM 내부 컨텍스트 구조체 도입:
  - `currentState`
  - `tag_present`
  - `lever_count`
  - `session_id` (선택)
- 외부는 getter로만 읽고 직접 쓰기 금지

### 5.4 결정론 전이 규칙 고정
- Activate 단계 규칙:
  - `TAG_ON` 시 `tag_present=true`, `lever_count=0`
  - `LEVER_STEP`는 `tag_present==true`일 때만 반영
  - `TAG_OFF` 시 `tag_present=false`, `lever_count=0`, state1 유지
  - `lever_count >= N`이면 state2 전이
- 동시 이벤트 우선순위(고정): `TAG_OFF > LEVER_STEP`

### 5.5 파일별 리팩토링 포인트
- `GameFSM.h`:
  - 이벤트 타입/컨텍스트 인터페이스 추가
- `GameFSM.cpp`:
  - 이벤트 핸들러 중심으로 전이 로직 통합
  - 상태 전이/모드 포인터 변경을 FSM 내부로 집중
- `rfid.ino`:
  - `BatteryFinish`, `StartFinish`, `LoginGenerator`에서 직접 전이/모드 설정 제거
  - FSM 이벤트 enqueue로 치환
- `encoder.ino`:
  - ISR에서 공유 상태 직접 갱신 최소화
  - 레버 스텝 이벤트 전달로 변경

## 6. 검증 계획
- 시나리오 1: 태그 유지 + 레버 N회 -> 항상 state2
- 시나리오 2: 레버 도중 태그 해제 -> 항상 state1 유지
- 시나리오 3: 태그 온/오프 빠른 반복 + 레버 입력 -> 결과 일관
- 시나리오 4: 네트워크 명령(S/R/A/...)과 로컬 입력 동시 -> 우선순위 규칙 준수
- 각 시나리오 100회 반복, 전이 불일치 0건 목표

## 7. 완료 기준
- 상태 변경 코드 경로 단일화 완료
- 전이 우선순위/규칙 문서화 완료
- 동시성 테스트 반복에서 비결정적 전이 0건
- 기존 게임 플로우 회귀 없음

## 8. 비범위(명시)
- 이 단계에서 오디오/DFPlayer 큐잉, mutex, 재생 정책은 다루지 않음
