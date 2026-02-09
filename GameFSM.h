#ifndef _GAME_FSM_H_
#define _GAME_FSM_H_

#include "Arduino.h"

// Forward declaration of dependencies if needed, or include generator.h
// #include "generator.h"

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

class GameFSM {
private:
  GameState currentState;

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

  // Command Handler
  void executeCommand(String command);

  // Getters
  GameState getCurrentState();
  String getStateName(GameState state);
};

extern GameFSM gameFSM;

#endif
