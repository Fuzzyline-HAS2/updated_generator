# FSM 코드 리뷰 (현재 구현 기준)

이 문서는 현재 브랜치의 실제 코드(`GameFSM.h`, `GameFSM.cpp`, `rfid.ino`, `timer.ino`, `Wifi.ino`, `encoder.ino`)를 기준으로 작성한 리뷰다.

## 1. 리뷰 요약
- 기존 포인터 기반 모드 분기(`ptrCurrentMode`, `ptrRfidMode`)는 제거되었고, 이벤트 큐 기반 FSM 진입점으로 통합됐다.
- 입력 소스(RFID/Timer/WiFi/Encoder ISR)는 상태를 직접 바꾸지 않고 이벤트를 발행한다.
- `FsmContext` 도입으로 상태 관련 값(`current_state`, `tag_present`, `lever_count`, `session_id`)이 FSM 내부로 캡슐화됐다.
- `TAG_OFF > TAG_ON > TIMEOUT > NET_CMD > LEVER_STEP` 우선순위가 `processEventQueue()`에 반영됐다.

## 2. 현재 구조에서 확인된 개선점

### A. 상태 전이 단일화
- 외부 파일에서 `gameFSM.executeCommand(...)` 직접 호출이 제거됨
- 전이 호출은 `enqueueNetCmd(...)` 또는 기타 이벤트 API를 통해 간접 호출
- FSM 최종 처리 지점은 `GameFSM::processEventQueue()`

관련 코드:
- `GameFSM.cpp`
- `Wifi.ino`
- `timer.ino`
- `rfid.ino`

### B. 이벤트 모델 강타입화
- `FsmEventType`에 `EVT_NET_CMD`, `EVT_TAG_ON`, `EVT_TAG_OFF`, `EVT_LEVER_STEP`, `EVT_TIMEOUT_LOGOUT`, `EVT_TIMEOUT_GAME` 정의
- `FsmEvent`에 `type`, `ts_ms`, `payload` 포함

관련 코드:
- `GameFSM.h`

### C. 컨텍스트 캡슐화
- `FsmContext` 도입
- 외부 조회 API 제공:
  - `getCurrentState()`
  - `isTagPresent()`
  - `getLeverCount()`
  - `getSessionId()`

관련 코드:
- `GameFSM.h`
- `GameFSM.cpp`

### D. 입력 경로 정리
- RFID: `enqueueTagOn()`, `enqueueTagOff()`, `enqueueNetCmd("player_tag")`
- Timer: `enqueueTimeoutGame()`, `enqueueTimeoutLogout()`
- WiFi: `enqueueNetCmd(...)`
- Encoder ISR: `notifyLeverStepFromISR()`

관련 코드:
- `rfid.ino`
- `timer.ino`
- `Wifi.ino`
- `encoder.ino`

## 3. 현재 코드에서 남아 있는 리스크/주의점

### A. 큐 동시성 보호 미적용
- 현재 큐는 링버퍼 + 일반 변수 기반 구현
- ISR/loop 동시 접근에 대한 임계구역 보호(`noInterrupts`, FreeRTOS queue 등)가 없음
- 이벤트 유실/경합 가능성이 남아 있음

권장:
- 다음 단계에서 FreeRTOS Queue 또는 임계구역 보호로 교체

### B. `EVT_LEVER_STEP` 타입은 정의됐지만 외부 enqueue API 미사용
- 실제 레버 입력은 `pendingLeverStepsFromIsr` 누적 방식으로 처리
- 타입 정의와 실제 경로가 혼합되어 있어 모델 일관성이 약함

권장:
- `enqueueLeverStep(uint16_t)` API 추가 후 단일 이벤트 경로로 통일
- 또는 문서에 "ISR 누적은 예외 경로"를 명시

### C. `executeCommand` 문자열 매핑 의존
- `EVT_NET_CMD`가 결국 문자열 기반 상태 명령으로 매핑됨
- 오타/미정의 명령 검증이 약함

권장:
- 상태 명령 enum 또는 command id 테이블 도입

### D. `Game_system.ino`의 `StarterActivate()`는 사실상 호환 잔재
- 루프는 `gameFSM.performStateLogic()` 중심으로 동작
- `StarterActivate()`가 더 이상 상태 전이 책임을 갖지 않음

권장:
- 사용처 없는 경우 제거하거나 레거시 호환용으로 주석 명시

## 4. 회귀 관점 체크리스트
- `setting -> ready -> activate -> battery_max -> starter_finish -> repaired` 흐름 확인
- 태그 해제 직후 레버 입력 무시(`TAG_OFF` 우선순위) 확인
- 타이머 timeout 시 상태 복귀 흐름 확인
- WiFi `game_state/device_state` 변경 수신 시 전이 확인

## 5. 결론
- 이번 리팩토링으로 FSM 구조는 기존 대비 명확해졌고, 상태 전이 책임이 실질적으로 집중됐다.
- 다만 큐 동시성 보호와 이벤트 모델 일관성(`EVT_LEVER_STEP` 경로 통일)은 다음 단계에서 보강이 필요하다.
