#include "GameFSM.h"
#include "generator.h" // 메인 헤더 포함 (has2wifi, Neopixel 등 전역 변수 접근)

GameFSM gameFSM;

GameFSM::GameFSM() { currentState = STATE_BOOT; }

void GameFSM::begin() {
  // 초기 설정이 필요하다면 여기에 작성
}

void GameFSM::executeCommand(String command) {
  Serial.println("FSM Command: " + command);

  // 전역 명령어 (상태를 변경하지 않거나 독립적으로 수행되는 동작)
  if (command == "watchdog") {
    ESP.restart();
    return;
  } else if (command == "check") {
    // 상태 보고 로직이 필요하다면 여기에 작성
    return;
  } else if (command == "OTA") {
    has2wifi.FirmwareUpdate((String)(const char *)my["device_type"],
                            "172.30.1.44");
    return;
  }

  // 상태 전이 로직
  // 사용자 요청 매핑: S, R, A, battery_max, starter_finish, repaired,
  // repaired_all 수동 오버라이드: mbm, msf, mr, mra

  GameState nextState = currentState;
  bool stateChanged = false;

  if (command == "S") {
    nextState = STATE_SETTING;
    stateChanged = true;
  } else if (command == "R") {
    nextState = STATE_READY;
    stateChanged = true;
  } else if (command == "A") {
    nextState = STATE_ACTIVATE;
    stateChanged = true;
  } else if (command == "battery_max" || command == "mbm") {
    nextState = STATE_BATTERY_MAX;
    stateChanged = true;
  } else if (command == "starter_finish" || command == "msf") {
    nextState = STATE_STARTER_FINISH;
    stateChanged = true;
  } else if (command == "repaired" || command == "mr") {
    nextState = STATE_REPAIRED;
    stateChanged = true;
  } else if (command == "repaired_all" || command == "mra") {
    nextState = STATE_REPAIRED_ALL;
    stateChanged = true;
  }

  if (stateChanged) {
    if (nextState != currentState) {
      exitState(currentState);
      currentState = nextState;
      enterState(currentState);
    } else {
      // 같은 상태라도 명령어가 들어오면 진입 로직(enterState)을 재실행 (S/R/A
      // 등 초기화 목적)
      enterState(currentState);
    }
  }
}

void GameFSM::enterState(GameState state) {
  // 각 상태별 진입 로직 실행
  switch (state) {
  case STATE_SETTING:
    onSetting();
    break;
  case STATE_READY:
    onReady();
    break;
  case STATE_ACTIVATE:
    onActivate();
    break;
  case STATE_BATTERY_MAX:
    onBatteryMax();
    break;
  case STATE_STARTER_FINISH:
    onStarterFinish();
    break;
  case STATE_REPAIRED:
    onRepaired();
    break;
  case STATE_REPAIRED_ALL:
    onRepairedAll();
    break;
  default:
    break;
  }
}

void GameFSM::exitState(GameState state) {
  // 상태를 벗어날 때 필요한 정리 로직
  // 현재는 다음 상태의 enterState에서 타이머 등을 정리하고 있음
}

void GameFSM::performStateLogic() {
  // 지속적으로 수행해야 하는 로직 (loop 내에서 호출)
  // 기존의 ptrCurrentMode 함수 포인터 대체
  if (ptrCurrentMode) {
    ptrCurrentMode();
  }
}

// --- 상태별 상세 구현 (기존 .ino 파일에서 이동됨) ---

void GameFSM::onSetting() {
  // 초기 설정 모드
  Serial.println("STATE: SETTING");
  SendCmd("page pgBeforeTagger");
  LeftGenerator();
  AllNeoOn(WHITE);
  EngineStop();
  encoderValue = 100;

  detachInterrupt(encoderPinA);
  detachInterrupt(encoderPinB);

  GameTimer.deleteTimer(gameTimerId);
  LogoutTimer.deleteTimer(logoutTimerId);
  BlinkTimer.deleteTimer(blinkTimerId);

  ptrRfidMode = WaitFunc;
  ptrCurrentMode = WaitFunc;
  receiveMineOn = false;
}

void GameFSM::onReady() {
  // 게임 준비 모드
  Serial.println("STATE: READY");
  AllNeoOn(RED);
  SendCmd("page pgPreTagger");
  LeftGenerator();

  detachInterrupt(encoderPinA);
  detachInterrupt(encoderPinB);

  GameTimer.deleteTimer(gameTimerId);
  LogoutTimer.deleteTimer(logoutTimerId);
  BlinkTimer.deleteTimer(blinkTimerId);

  ptrRfidMode = WaitFunc;
  ptrCurrentMode = WaitFunc;
}

void GameFSM::onActivate() {
  // 게임 활성화 (로그인 성공)
  Serial.println("STATE: ACTIVATE");
  AllNeoOn(YELLOW);
  SendCmd("page pgLocked");
  LeftGenerator();

  detachInterrupt(encoderPinA);
  detachInterrupt(encoderPinB);

  GameTimer.deleteTimer(gameTimerId);
  LogoutTimer.deleteTimer(logoutTimerId);
  BlinkTimer.deleteTimer(blinkTimerId);

  ptrRfidMode = LoginGenerator;  // rfid.ino에 정의됨
  ptrCurrentMode = RfidLoopMain; // rfid.ino에 정의됨
}

void GameFSM::onBatteryMax() {
  // 배터리 완충 상태 처리
  Serial.println("STATE: BATTERY_MAX");

  // 배터리 잔량 계산 및 전송
  int maxBattery = (int)my["max_battery_pack"] - (int)my["battery_pack"];
  has2wifi.Send((String)(const char *)my["device_name"], "battery_pack",
                ((String)maxBattery));

  LogoutTimer.deleteTimer(logoutTimerId);
  GameTimer.deleteTimer(gameTimerId);

  // Activate 동작 수행
  onActivate();
}

void GameFSM::onStarterFinish() {
  // 발전(스타터) 완료 상태
  Serial.println("STATE: STARTER_FINISH");

  SendCmd("page pgStarterDone");
  ptrRfidMode = StartFinish; // rfid.ino에 정의됨
  ptrCurrentMode = RfidLoopMain;

  GameTimer.deleteTimer(gameTimerId);
  BlinkTimer.deleteTimer(blinkTimerId);
  lightColor(pixels[STARTER], color[GREEN]);
  BlinkTimerStart(CIRCUIT, YELLOW);
}

void GameFSM::onRepaired() {
  // 수리 완료 상태
  Serial.println("STATE: REPAIRED");

  LogoutTimer.deleteTimer(logoutTimerId);
  GameTimer.deleteTimer(gameTimerId);
  BlinkTimer.deleteTimer(blinkTimerId);

  Serial.println("Generator Fixed!");
  SendCmd("page pgFixed");
  LeftGenerator();
  AllNeoOn(BLUE);

  EngineSpeeed(250); // 모터 가동

  ptrCurrentMode = WaitFunc;
}

void GameFSM::onRepairedAll() {
  // 전체 수리 완료 (탈출)
  Serial.println("STATE: REPAIRED_ALL");

  ptrRfidMode = WaitFunc;
  ptrCurrentMode = WaitFunc;

  LeftGenerator();
  EngineStop();

  SendCmd("page pgEscapeOpen");
  LeftGenerator();

  GameTimer.deleteTimer(gameTimerId);
  LogoutTimer.deleteTimer(logoutTimerId);
  BlinkTimer.deleteTimer(blinkTimerId);

  AllNeoOn(BLUE);
}

GameState GameFSM::getCurrentState() { return currentState; }
