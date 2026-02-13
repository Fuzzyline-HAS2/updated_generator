# HAS Generator Game System (ESP32)

이 프로젝트는 ESP32 기반 발전기 게임 장치입니다.  
RFID 태그, 엔코더(레버), 네오픽셀, Nextion, WiFi를 사용해 게임 상태를 제어합니다.

## 프로젝트 핵심
- 게임 상태 관리는 `GameFSM`에서 담당합니다.
- 입력(RFID, 타이머, WiFi, 엔코더 ISR)은 FSM 이벤트로 전달됩니다.
- 상태 전이는 이벤트 큐를 통해 직렬 처리됩니다.

## 하드웨어 구성
- ESP32 Dev Module
- PN532 RFID (SPI)
- Nextion HMI (UART)
- WS2812B Neopixel
- Rotary Encoder
- DC Motor

## 코드 구조
- `updated_generator.ino`: `setup()/loop()` 엔트리 포인트
- `GameFSM.h`, `GameFSM.cpp`: 상태 머신, 이벤트 타입, 상태 전이/액션
- `rfid.ino`: 태그 감지 및 FSM 이벤트 발행
- `encoder.ino`: 엔코더 ISR 처리, 레버 스텝 이벤트 알림
- `timer.ino`: 주기/타임아웃 이벤트 발행
- `Wifi.ino`: 서버 데이터 변경을 FSM 명령 이벤트로 변환
- `nextion.ino`: 디스플레이 관련 입출력
- `neopixel.ino`: LED 제어
- `motor.ino`: 모터 제어
- `GlobalVariables.cpp`, `generator.h`: 전역 리소스/선언

## FSM 이벤트 모델
현재 FSM에서 사용하는 주요 이벤트 타입:
- `EVT_NET_CMD`
- `EVT_TAG_ON`
- `EVT_TAG_OFF`
- `EVT_LEVER_STEP`
- `EVT_TIMEOUT_LOGOUT`
- `EVT_TIMEOUT_GAME`

우선순위 처리:
1. `TAG_OFF`
2. `TAG_ON`
3. `TIMEOUT_*`
4. `NET_CMD`
5. `LEVER_STEP`

## 주요 상태
- `STATE_SETTING`
- `STATE_READY`
- `STATE_ACTIVATE`
- `STATE_BATTERY_MAX`
- `STATE_STARTER_FINISH`
- `STATE_REPAIRED`
- `STATE_REPAIRED_ALL`

## 동작 흐름(요약)
1. 초기화 후 `STATE_SETTING` 또는 서버 상태를 따라 전이
2. 플레이어 태그 인식 시 `player_tag` 이벤트 처리
3. 배터리 최대 시 `STATE_BATTERY_MAX`로 진입, 스타터 모드 시작
4. 레버 누적값 임계치 도달 시 `starter_finish` 전이
5. 이후 태그/서버 이벤트에 따라 `repaired`, `repaired_all`로 진행

## 빌드/업로드
1. Arduino IDE 또는 PlatformIO에서 프로젝트 열기
2. 보드: `ESP32 Dev Module`
3. 필요한 라이브러리 설치
- `Adafruit NeoPixel`
- `Adafruit PN532`
- `ArduinoJson`
- `SimpleTimer` (또는 호환 타이머 라이브러리)
4. 엔트리 파일 `updated_generator.ino` 기준으로 빌드/업로드

## 빠른 확인 포인트
- 메인 루프는 `gameFSM.performStateLogic()` + `TimerRun()` 순서로 동작
- 외부에서 FSM 상태를 직접 바꾸지 않고 이벤트 API를 사용
- 엔코더 ISR은 `notifyLeverStepFromISR()`만 호출

## 문서
- 리팩토링 계획: `docs/FSM_REFACTOR_PLAN.md`
- 변경 이력: `docs/FSM_REFACTOR_CHANGELOG.md`
- 코드 리뷰: `docs/FSM_CODE_REVIEW.md`

## 참고
- 기존 문서/주석 중 일부는 인코딩 이슈가 있을 수 있습니다.
- 현재 구조 기준 설명은 이 README와 `docs/FSM_REFACTOR_CHANGELOG.md`를 우선 참고하세요.
