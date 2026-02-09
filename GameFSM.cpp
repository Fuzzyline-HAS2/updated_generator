#include "GameFSM.h"
#include "generator.h" // Include main header to access globals like has2wifi, Neopixel, etc.

GameFSM gameFSM;

GameFSM::GameFSM() { currentState = STATE_BOOT; }

void GameFSM::begin() {
  // Initial setup if needed
}

void GameFSM::executeCommand(String command) {
  Serial.println("FSM Command: " + command);

  // Global commands (actions that don't necessarily change state or force
  // change)
  if (command == "watchdog") {
    ESP.restart();
    return;
  } else if (command == "check") {
    // Report status logic here if needed
    return;
  } else if (command == "OTA") {
    has2wifi.FirmwareUpdate((String)(const char *)my["device_type"],
                            "172.30.1.44");
    return;
  }

  // State Transitions
  // Mapped from user request: S, R, A, battery_max, starter_finish, repaired,
  // repaired_all Plus manual overrides: mbm, msf, mr, mra

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
      // Re-trigger enter logic if command explicitly sent? usually yes for
      // S/R/A
      enterState(currentState);
    }
  }
}

void GameFSM::enterState(GameState state) {
  // Logic moved from SettingFunc, ReadyFunc, etc.
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
  // Cleanup logic if needed when leaving a state
  // For now, most cleanup is handled by the 'enter' of the next state (e.g.,
  // deleting timers) But ideally, we should stop things here.

  // Example: verify timers are stopped if they shouldn't persist
}

void GameFSM::performStateLogic() {
  // Continuous logic (loop)
  // This replaces 'ptrCurrentMode'

  if (ptrCurrentMode) {
    ptrCurrentMode(); // Keep using the function pointer for now if detailed
                      // sub-logic (like RfidLoopMain) relies on it
  }
}

// --- Implementation of Logic (Moved from .ino files) ---

void GameFSM::onSetting() {
  // Logic from SettingFunc
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
  // Logic from ReadyFunc
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
  // Logic from ActivateFunc
  Serial.println("STATE: ACTIVATE");
  AllNeoOn(YELLOW);
  SendCmd("page pgLocked");
  LeftGenerator();

  detachInterrupt(encoderPinA);
  detachInterrupt(encoderPinB);

  GameTimer.deleteTimer(gameTimerId);
  LogoutTimer.deleteTimer(logoutTimerId);
  BlinkTimer.deleteTimer(blinkTimerId);

  ptrRfidMode = LoginGenerator;  // Defined in rfid.ino (needs to be accessible)
  ptrCurrentMode = RfidLoopMain; // Defined in rfid.ino
}

void GameFSM::onBatteryMax() {
  // Logic from battery_max cases
  Serial.println("STATE: BATTERY_MAX");

  // Calculate max battery
  int maxBattery = (int)my["max_battery_pack"] - (int)my["battery_pack"];
  // Serial.println((String)maxBattery);

  // Send status
  has2wifi.Send((String)(const char *)my["device_name"], "battery_pack",
                ((String)maxBattery));

  LogoutTimer.deleteTimer(logoutTimerId);
  GameTimer.deleteTimer(gameTimerId);

  // Re-trigger activate logic? Or specific Battery Max logic?
  // Original code called ActivateFunc() after battery_max logic in DataChanged
  // But also had specific battery checks.
  // Logic for 'battery_max' in DataChanged:
  // 1. Send battery_pack update
  // 2. Stop timers
  // 3. ActivateFunc()

  // Let's call onActivate() at the end or just set state to ACTIVATE?
  // If state is BATTERY_MAX, we perform logic then switch to ACTIVATE?
  // Or BATTERY_MAX is a transient state provided by server?

  // For now, implementing the specific actions:
  // ActivateFunc(); // This calls the function (which we are moving/replacing)
  // Since onActivate duplicates ActivateFunc, we can call onActivate() here
  onActivate();
}

void GameFSM::onStarterFinish() {
  // Logic for starter_finish
  // Original code in DataChanged was empty for "starter_finish"?
  // "else if((String)(const char*)my["device_state"] == "starter_finish") { }"

  // Usage in Game_system.ino (StarterActivate function):
  // SendCmd("page pgStarterDone");
  // has2wifi.Send(..., "starter_finish");
  // ptrRfidMode = StartFinish;
  // ptrCurrentMode = RfidLoopMain;
  // ...

  Serial.println("STATE: STARTER_FINISH");
  // If this is triggered by command from server (DataChanged), it might be an
  // ack? Or if we need to force this state:

  SendCmd("page pgStarterDone");
  ptrRfidMode = StartFinish; // StartFinish is in rfid.ino
  ptrCurrentMode = RfidLoopMain;

  GameTimer.deleteTimer(gameTimerId);
  BlinkTimer.deleteTimer(blinkTimerId);
  lightColor(pixels[STARTER], color[GREEN]);
  BlinkTimerStart(CIRCUIT, YELLOW);
}

void GameFSM::onRepaired() {
  // Logic from DataChanged "repaired"
  Serial.println("STATE: REPAIRED");

  LogoutTimer.deleteTimer(logoutTimerId);
  GameTimer.deleteTimer(gameTimerId);
  BlinkTimer.deleteTimer(blinkTimerId);

  Serial.println("Generator Fixed!");
  SendCmd("page pgFixed");
  LeftGenerator();
  AllNeoOn(BLUE);

  // ledcWrite(MotorLedChannel, 250); // Engine logic
  EngineSpeeed(250);

  ptrCurrentMode = WaitFunc;
}

void GameFSM::onRepairedAll() {
  // Logic from DataChanged "repaired_all"
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
