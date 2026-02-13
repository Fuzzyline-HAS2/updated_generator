# FSM 리팩토링 변경 정리

이 문서는 이번 FSM 리팩토링에서 실제로 코드가 어떻게 바뀌었는지 파일 단위로 정리한 기록이다.

## 1. 변경 목적
- 이벤트 기반으로 입력 경로를 통일
- 상태 전이 책임을 `GameFSM`으로 집중
- RFID/타이머/엔코더/WiFi가 직접 상태를 바꾸지 않고 이벤트만 발행하도록 변경
- `ptrCurrentMode`, `ptrRfidMode` 전역 포인터 의존 제거

## 2. 핵심 아키텍처 변경

### 2.1 강타입 이벤트 모델 도입
- 파일: `GameFSM.h`, `GameFSM.cpp`
- 추가된 이벤트 타입:
  - `EVT_NET_CMD`
  - `EVT_TAG_ON`
  - `EVT_TAG_OFF`
  - `EVT_LEVER_STEP`
  - `EVT_TIMEOUT_LOGOUT`
  - `EVT_TIMEOUT_GAME`
- 추가된 구조체:
  - `FsmEventPayload` (`net_cmd`, `lever_steps`)
  - `FsmEvent` (`type`, `ts_ms`, `payload`)
  - `FsmContext` (`current_state`, `tag_present`, `lever_count`, `session_id`)

### 2.2 FSM 컨텍스트 캡슐화
- 파일: `GameFSM.h`, `GameFSM.cpp`
- 기존의 단일 상태 변수 중심에서 `ctx` 구조체 기반으로 변경
- 외부 조회용 getter 추가:
  - `getCurrentState()`
  - `isTagPresent()`
  - `getLeverCount()`
  - `getSessionId()`

### 2.3 이벤트 소비/우선순위 처리
- 파일: `GameFSM.cpp`
- `processEventQueue()`에서 이벤트를 모아 우선순위대로 처리
- 현재 우선순위:
  1. `TAG_OFF`
  2. `TAG_ON`
  3. `TIMEOUT`
  4. `NET_CMD`
  5. `LEVER_STEP`

## 3. 파일별 상세 변경

### 3.1 `GameFSM.h`
- 이벤트 enum/페이로드/컨텍스트 타입 추가
- API 변경:
  - 추가: `enqueueNetCmd`, `enqueueTagOn`, `enqueueTagOff`
  - 추가: `enqueueTimeoutLogout`, `enqueueTimeoutGame`
  - 변경: `notifyLeverStepFromISR(uint16_t steps=1)` 시그니처 확장
- 내부 상태/런타임 모드 필드 추가

### 3.2 `GameFSM.cpp`
- 거의 전면 재구성
- 변경 내용:
  - 이벤트 enqueue API 구현
  - `processEventQueue()` 우선순위 처리 구현
  - ISR 누적 레버 스텝(`pendingLeverStepsFromIsr`) 처리 추가
  - `handlePlayerTag()`로 RFID player 태그 처리 로직을 FSM 내부로 이동
  - `runStarterModeLoop()`에서 스타터 게이지/모터 동작 관리
  - 상태 진입 함수(`onSetting ~ onRepairedAll`)를 FSM 내부 동작 중심으로 정리
- 결과:
  - 상태 전이 엔트리는 `executeCommand()` + FSM 이벤트 처리 경로로 집중됨

### 3.3 `rfid.ino`
- 이전: 로그인/충전/수리 완료 처리와 모드 포인터 전환까지 직접 수행
- 이후: RFID 판독 + 역할 판별 + FSM 이벤트 발행만 수행
  - 태그 감지 시작: `enqueueTagOn()`
  - 태그 해제 타임아웃: `enqueueTagOff()`
  - player 태그 인식: `enqueueNetCmd("player_tag")`
- 삭제된 직접 상태 제어:
  - `ptrRfidMode()` 직접 호출
  - `ptrCurrentMode/ptrRfidMode` 직접 대입
  - `LoginGenerator`, `StartFinish`, `BatteryPackCharge` 등 상태성 함수

### 3.4 `timer.ino`
- `GameTimerFunc()`:
  - 이전: `encoderValue` 직접 감소
  - 이후: `enqueueTimeoutGame()` 이벤트 발행
- `LogoutTimerFunc()`:
  - 이전: 직접 `enqueueCommand("A")` + 카운터 리셋
  - 이후: `enqueueTimeoutLogout()` 이벤트 발행

### 3.5 `Wifi.ino`
- 이전: `enqueueCommand(...)` 사용
- 이후: `enqueueNetCmd(...)`로 전환
- `game_state`, `device_state` 변경 모두 net command 이벤트로 통일

### 3.6 `encoder.ino`
- 유지된 정책:
  - ISR에서 상태 전이/화면/네트워크 처리 없음
  - 유효 엔코더 전이 시 `notifyLeverStepFromISR()`만 호출

### 3.7 `Game_system.ino`
- `StarterActivate()`에서 `starter_finish` 직접 명령 발행 제거
- 스타터 완료 전이는 FSM 내부 레버 누적 임계치 기반으로 처리되도록 정리

### 3.8 `updated_generator.ino`
- `loop()` 변경:
  - 이전: `ptrCurrentMode()` 호출
  - 이후: `gameFSM.performStateLogic()` 호출
- 메인 루프 진입점을 FSM 중심으로 고정

### 3.9 `generator.h`, `GlobalVariables.cpp`
- 제거:
  - `ptrCurrentMode`, `ptrRfidMode` 선언/정의
  - 구식 RFID 모드 함수 선언(`LoginGenerator`, `StartFinish`)
- 결과:
  - 모드 함수 포인터 기반 전역 상태 제어 경로 제거

## 4. 확인된 조건
- 코드 기준 `gameFSM.executeCommand(...)` 직접 호출: FSM 외부 0건
- 코드 기준 `ptrCurrentMode|ptrRfidMode` 참조: 문서 제외 0건
- 입력 소스 이벤트화:
  - WiFi -> `enqueueNetCmd`
  - Timer -> `enqueueTimeout*`
  - RFID -> `enqueueTagOn/Off`, `enqueueNetCmd("player_tag")`
  - Encoder ISR -> `notifyLeverStepFromISR`

## 5. 주의사항
- 이번 변경은 구조 리팩토링 중심이며, 실장비 동작 검증(시나리오 반복 100회)은 별도 수행 필요
- `Game_system.ino`의 `StarterActivate()`는 현재 사실상 빈 루프 성격이며, 향후 제거 또는 호환용 유지 여부 결정 필요
