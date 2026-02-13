#ifndef _GAME_FSM_H_
#define _GAME_FSM_H_

#include "Arduino.h"

enum GameState {
  STATE_BOOT,
  STATE_SETTING,
  STATE_READY,
  STATE_ACTIVATE,
  STATE_BATTERY_MAX,
  STATE_STARTER_FINISH,
  STATE_REPAIRED,
  STATE_REPAIRED_ALL
};

enum FsmEventType {
  EVT_NET_CMD = 0,
  EVT_TAG_ON,
  EVT_TAG_OFF,
  EVT_LEVER_STEP,
  EVT_TIMEOUT_LOGOUT,
  EVT_TIMEOUT_GAME
};

struct FsmEventPayload {
  char net_cmd[24];
  uint16_t lever_steps;
};

struct FsmEvent {
  FsmEventType type;
  uint32_t ts_ms;
  FsmEventPayload payload;
};

struct FsmContext {
  GameState current_state;
  bool tag_present;
  uint16_t lever_count;
  uint32_t session_id;
};

class GameFSM {
private:
  enum RuntimeMode { MODE_IDLE = 0, MODE_STARTER };

  FsmContext ctx;
  RuntimeMode runtimeMode;
  static const uint8_t kEventQueueSize = 16;
  FsmEvent eventQueue[kEventQueueSize];
  uint8_t eventHead;
  uint8_t eventTail;
  uint8_t eventCount;
  volatile uint16_t pendingLeverStepsFromIsr;
  uint8_t gameTickPhase;
  bool encoderIsrAttached;

  bool isStarterMode() const;
  uint16_t getLeverThreshold() const;
  void resetStarterGauge();
  void runStarterModeLoop();
  void attachEncoderInterrupts();
  void detachEncoderInterrupts();

  void processTagOn();
  void processTagOff();
  void processLeverSteps(uint16_t steps);
  void processTimeoutLogout();
  void processTimeoutGame();
  void processNetCmd(const char *command);
  void handlePlayerTag();
  const char *eventTypeName(FsmEventType type) const;
  void logEvent(FsmEventType type) const;
  void logState(const char *prefix) const;

  void enterState(GameState state);
  void exitState(GameState state);

  // Action methods (logic moved from .ino files)
  void onSetting();
  void onReady();
  void onActivate();
  void onBatteryMax();
  void onStarterFinish();
  void onRepaired();
  void onRepairedAll();

public:
  GameFSM();
  void begin();
  void performStateLogic(); // Loop function

  bool enqueueEvent(const FsmEvent &event);
  bool enqueueNetCmd(const String &command);
  bool enqueueTagOn();
  bool enqueueTagOff();
  bool enqueueTimeoutLogout();
  bool enqueueTimeoutGame();
  void processEventQueue();
  void notifyLeverStepFromISR(uint16_t steps = 1);
  void executeCommand(String command);

  // Getters for context
  GameState getCurrentState();
  bool isTagPresent();
  uint16_t getLeverCount();
  uint32_t getSessionId();
  String getStateName(GameState state) const;
};

extern GameFSM gameFSM;

#endif
