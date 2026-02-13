#include "GameFSM.h"
#include "generator.h"
#include <cstring>

GameFSM gameFSM;

GameFSM::GameFSM() {
  ctx.current_state = STATE_BOOT;
  ctx.tag_present = false;
  ctx.lever_count = 0;
  ctx.session_id = 0;
  runtimeMode = MODE_IDLE;
  eventHead = 0;
  eventTail = 0;
  eventCount = 0;
  pendingLeverStepsFromIsr = 0;
  gameTickPhase = 0;
}

void GameFSM::begin() {}

bool GameFSM::enqueueEvent(const FsmEvent &event) {
  if (eventCount >= kEventQueueSize) {
    Serial.println("FSM event queue full, event dropped");
    return false;
  }

  eventQueue[eventTail] = event;
  eventTail = (eventTail + 1) % kEventQueueSize;
  eventCount++;
  return true;
}

bool GameFSM::enqueueNetCmd(const String &command) {
  FsmEvent event;
  event.type = EVT_NET_CMD;
  event.ts_ms = millis();
  memset(&event.payload, 0, sizeof(event.payload));
  command.substring(0, sizeof(event.payload.net_cmd) - 1)
      .toCharArray(event.payload.net_cmd, sizeof(event.payload.net_cmd));
  return enqueueEvent(event);
}

bool GameFSM::enqueueTagOn() {
  FsmEvent event;
  event.type = EVT_TAG_ON;
  event.ts_ms = millis();
  memset(&event.payload, 0, sizeof(event.payload));
  return enqueueEvent(event);
}

bool GameFSM::enqueueTagOff() {
  FsmEvent event;
  event.type = EVT_TAG_OFF;
  event.ts_ms = millis();
  memset(&event.payload, 0, sizeof(event.payload));
  return enqueueEvent(event);
}

bool GameFSM::enqueueTimeoutLogout() {
  FsmEvent event;
  event.type = EVT_TIMEOUT_LOGOUT;
  event.ts_ms = millis();
  memset(&event.payload, 0, sizeof(event.payload));
  return enqueueEvent(event);
}

bool GameFSM::enqueueTimeoutGame() {
  FsmEvent event;
  event.type = EVT_TIMEOUT_GAME;
  event.ts_ms = millis();
  memset(&event.payload, 0, sizeof(event.payload));
  return enqueueEvent(event);
}

void GameFSM::notifyLeverStepFromISR(uint16_t steps) {
  pendingLeverStepsFromIsr += steps;
}

void GameFSM::processTagOn() {
  ctx.tag_present = true;
  ctx.session_id++;
}

void GameFSM::processTagOff() {
  ctx.tag_present = false;
  resetStarterGauge();
}

void GameFSM::processLeverSteps(uint16_t steps) {
  if (!ctx.tag_present || !isStarterMode() || steps == 0) {
    return;
  }

  ctx.lever_count += steps;
  encoderValue = ctx.lever_count;

  if (ctx.lever_count >= getLeverThreshold()) {
    enqueueNetCmd("starter_finish");
  }
}

void GameFSM::processTimeoutLogout() {
  if (logoutTimerCnt >= 12) {
    logoutTimerCnt = 0;
    enqueueNetCmd("A");
  }
}

void GameFSM::processTimeoutGame() {
  if (!isStarterMode()) {
    return;
  }

  gameTickPhase++;
  if (gameTickPhase != 5) {
    return;
  }

  int decay = (int)(starterNeoDivider * 0.01f);
  if (ctx.lever_count > decay) {
    ctx.lever_count -= decay;
  } else {
    ctx.lever_count = 0;
  }
  encoderValue = ctx.lever_count;
  gameTickPhase = 3;
}

void GameFSM::processNetCmd(const char *command) {
  if (strcmp(command, "player_tag") == 0) {
    handlePlayerTag();
    return;
  }
  executeCommand(String(command));
}

void GameFSM::handlePlayerTag() {
  if (!ctx.tag_present) {
    return;
  }

  logoutTimerCnt = 0;

  if (ctx.current_state == STATE_STARTER_FINISH) {
    has2wifi.Send((String)(const char *)my["device_name"], "device_state",
                  "repaired");
    receiveMineOn = true;
    has2wifi.ReceiveMine();
    enqueueNetCmd("repaired");
    return;
  }

  if (ctx.current_state != STATE_ACTIVATE) {
    return;
  }

  BatteryPackSend();
  PageSend();
  SendCmd("page pgLogin");
  LeftGenerator();

  LogoutTimer.deleteTimer(logoutTimerId);
  logoutTimerId = LogoutTimer.setInterval(logoutTime, LogoutTimerFunc);

  if ((int)my["battery_pack"] >= (int)my["max_battery_pack"]) {
    enqueueNetCmd("battery_max");
    return;
  }

  int playerPack = (int)tag["battery_pack"];
  if (playerPack <= 0) {
    Serial.println("Player has no battery packs.");
    return;
  }

  SendCmd("wBatteryCharge.en=1");
  has2wifi.Send((String)(const char *)tag["device_name"], "battery_pack", "-1");
  has2wifi.Send((String)(const char *)my["device_name"], "battery_pack", "+1");
  receiveMineOn = true;
  has2wifi.ReceiveMine();
  BatteryPackSend();

  if ((int)my["battery_pack"] >= (int)my["max_battery_pack"]) {
    SendCmd("page pgBatteryMax");
    delay(100);
    SendCmd("wBatteryFull.en=1");
    enqueueNetCmd("battery_max");
  }
}

void GameFSM::processEventQueue() {
  bool hasTagOff = false;
  bool hasTagOn = false;
  uint8_t timeoutLogoutCount = 0;
  uint8_t timeoutGameCount = 0;
  uint16_t leverSteps = pendingLeverStepsFromIsr;
  pendingLeverStepsFromIsr = 0;

  char netCmds[kEventQueueSize][24];
  uint8_t netCmdCount = 0;

  while (eventCount > 0) {
    FsmEvent event = eventQueue[eventHead];
    eventHead = (eventHead + 1) % kEventQueueSize;
    eventCount--;

    switch (event.type) {
    case EVT_TAG_OFF:
      hasTagOff = true;
      break;
    case EVT_TAG_ON:
      hasTagOn = true;
      break;
    case EVT_TIMEOUT_LOGOUT:
      timeoutLogoutCount++;
      break;
    case EVT_TIMEOUT_GAME:
      timeoutGameCount++;
      break;
    case EVT_LEVER_STEP:
      leverSteps += event.payload.lever_steps;
      break;
    case EVT_NET_CMD:
      if (netCmdCount < kEventQueueSize) {
        strncpy(netCmds[netCmdCount], event.payload.net_cmd,
                sizeof(netCmds[netCmdCount]) - 1);
        netCmds[netCmdCount][sizeof(netCmds[netCmdCount]) - 1] = '\0';
        netCmdCount++;
      }
      break;
    default:
      break;
    }
  }

  // Priority: TAG_OFF > TAG_ON > TIMEOUT > NET_CMD > LEVER_STEP
  if (hasTagOff) {
    processTagOff();
  }
  if (hasTagOn) {
    processTagOn();
  }

  for (uint8_t i = 0; i < timeoutLogoutCount; ++i) {
    processTimeoutLogout();
  }
  for (uint8_t i = 0; i < timeoutGameCount; ++i) {
    processTimeoutGame();
  }
  for (uint8_t i = 0; i < netCmdCount; ++i) {
    processNetCmd(netCmds[i]);
  }
  processLeverSteps(leverSteps);
}

void GameFSM::executeCommand(String command) {
  if (command == "watchdog") {
    ESP.restart();
    return;
  }
  if (command == "check") {
    return;
  }
  if (command == "OTA") {
    has2wifi.FirmwareUpdate((String)(const char *)my["device_type"],
                            "172.30.1.44");
    return;
  }

  GameState nextState = ctx.current_state;
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

  if (!stateChanged) {
    return;
  }

  if (nextState != ctx.current_state) {
    exitState(ctx.current_state);
    ctx.current_state = nextState;
    enterState(ctx.current_state);
  } else {
    enterState(ctx.current_state);
  }
}

void GameFSM::enterState(GameState state) {
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

void GameFSM::exitState(GameState state) { (void)state; }

void GameFSM::performStateLogic() {
  processEventQueue();
  runStarterModeLoop();
}

bool GameFSM::isStarterMode() const { return runtimeMode == MODE_STARTER; }

uint16_t GameFSM::getLeverThreshold() const {
  return (uint16_t)(NumPixels[GAUGE] * 4000);
}

void GameFSM::resetStarterGauge() {
  ctx.lever_count = 0;
  encoderValue = 0;
}

void GameFSM::runStarterModeLoop() {
  if (!isStarterMode()) {
    return;
  }

  int gaugeNeoCnt = ctx.lever_count / 4000;
  EncoderNeopixelOn(gaugeNeoCnt);
  EngineSpeeed(gaugeNeoCnt * 8);
}

void GameFSM::onSetting() {
  runtimeMode = MODE_IDLE;
  resetStarterGauge();
  ctx.tag_present = false;

  SendCmd("page pgBeforeTagger");
  LeftGenerator();
  AllNeoOn(WHITE);
  EngineStop();

  detachInterrupt(encoderPinA);
  detachInterrupt(encoderPinB);

  GameTimer.deleteTimer(gameTimerId);
  LogoutTimer.deleteTimer(logoutTimerId);
  BlinkTimer.deleteTimer(blinkTimerId);
  receiveMineOn = false;
}

void GameFSM::onReady() {
  runtimeMode = MODE_IDLE;
  resetStarterGauge();

  AllNeoOn(RED);
  SendCmd("page pgPreTagger");
  LeftGenerator();

  detachInterrupt(encoderPinA);
  detachInterrupt(encoderPinB);

  GameTimer.deleteTimer(gameTimerId);
  LogoutTimer.deleteTimer(logoutTimerId);
  BlinkTimer.deleteTimer(blinkTimerId);
}

void GameFSM::onActivate() {
  runtimeMode = MODE_IDLE;
  resetStarterGauge();

  AllNeoOn(YELLOW);
  SendCmd("page pgLocked");
  LeftGenerator();

  detachInterrupt(encoderPinA);
  detachInterrupt(encoderPinB);

  GameTimer.deleteTimer(gameTimerId);
  LogoutTimer.deleteTimer(logoutTimerId);
  BlinkTimer.deleteTimer(blinkTimerId);
}

void GameFSM::onBatteryMax() {
  runtimeMode = MODE_STARTER;
  ctx.lever_count = 1;
  encoderValue = ctx.lever_count;
  gameTickPhase = 0;

  has2wifi.Send((String)(const char *)my["device_name"], "device_state",
                "battery_max");

  SendCmd("page pgStarter");
  delay(10);
  SendCmd("wStaterOn.en=1");
  LeftGenerator();
  AllNeoOn(GREEN);

  LogoutTimer.deleteTimer(logoutTimerId);
  GameTimer.deleteTimer(gameTimerId);
  BlinkTimer.deleteTimer(blinkTimerId);
  gameTimerCnt = 0;
  gameTimerId = GameTimer.setInterval(gameTime, GameTimerFunc);
  BlinkTimerStart(STARTER, YELLOW);

  attachInterrupt(encoderPinA, updateEncoder, CHANGE);
  attachInterrupt(encoderPinB, updateEncoder, CHANGE);
}

void GameFSM::onStarterFinish() {
  runtimeMode = MODE_IDLE;
  detachInterrupt(encoderPinA);
  detachInterrupt(encoderPinB);

  SendCmd("page pgStarterDone");

  GameTimer.deleteTimer(gameTimerId);
  BlinkTimer.deleteTimer(blinkTimerId);
  lightColor(pixels[STARTER], color[GREEN]);
  BlinkTimerStart(CIRCUIT, YELLOW);
}

void GameFSM::onRepaired() {
  runtimeMode = MODE_IDLE;

  LogoutTimer.deleteTimer(logoutTimerId);
  GameTimer.deleteTimer(gameTimerId);
  BlinkTimer.deleteTimer(blinkTimerId);

  SendCmd("page pgFixed");
  LeftGenerator();
  AllNeoOn(BLUE);
  EngineSpeeed(250);
}

void GameFSM::onRepairedAll() {
  runtimeMode = MODE_IDLE;

  LeftGenerator();
  EngineStop();
  SendCmd("page pgEscapeOpen");
  LeftGenerator();

  GameTimer.deleteTimer(gameTimerId);
  LogoutTimer.deleteTimer(logoutTimerId);
  BlinkTimer.deleteTimer(blinkTimerId);

  AllNeoOn(BLUE);
}

GameState GameFSM::getCurrentState() { return ctx.current_state; }

bool GameFSM::isTagPresent() { return ctx.tag_present; }

uint16_t GameFSM::getLeverCount() { return ctx.lever_count; }

uint32_t GameFSM::getSessionId() { return ctx.session_id; }

String GameFSM::getStateName(GameState state) {
  switch (state) {
  case STATE_BOOT:
    return "boot";
  case STATE_SETTING:
    return "setting";
  case STATE_READY:
    return "ready";
  case STATE_ACTIVATE:
    return "activate";
  case STATE_BATTERY_MAX:
    return "battery_max";
  case STATE_STARTER_FINISH:
    return "starter_finish";
  case STATE_REPAIRED:
    return "repaired";
  case STATE_REPAIRED_ALL:
    return "repaired_all";
  default:
    return "unknown";
  }
}
