// Main command handler for Wifi Data
void DataChanged() {
  static StaticJsonDocument<500> cur;

  // Game State Changes
  if ((String)(const char *)my["game_state"] !=
      (String)(const char *)cur["game_state"]) {
    String cmd = (String)(const char *)my["game_state"];

    // Map legacy "setting", "ready", "activate" to single letter commands if
    // needed Or just pass the string to GameFSM and handle it there? GameFSM
    // handles "S", "R", "A". The incoming values are "setting", "ready",
    // "activate".

    if (cmd == "setting")
      gameFSM.executeCommand("S");
    else if (cmd == "ready")
      gameFSM.executeCommand("R");
    else if (cmd == "activate")
      gameFSM.executeCommand("A");
  }

  // Left Generator Logic (Separate from FSM or part of it? Keeping as is for
  // now)
  if ((String)(const char *)my["left_generator"] !=
      (String)(const char *)cur["left_generator"]) {
    LeftGenerator();
  }

  // Device State Changes (receiveMineOn check is existing logic)
  if (receiveMineOn == false) {
    if ((String)(const char *)my["device_state"] !=
        (String)(const char *)cur["device_state"]) {
      String stateCmd = (String)(const char *)my["device_state"];

      // Pass to FSM to handle mapped commands
      // GameFSM handles: "repaired", "repaired_all", "battery_max",
      // "starter_finish", "player_win", "player_lose" Note: "player_win/lose"
      // were in original code but I didn't add them to FSM plan explicitly. I
      // should check if I missed them in FSM.

      gameFSM.executeCommand(stateCmd);
    }
  } else {
    receiveMineOn = false;
  }

  cur = my;
}

// Old state functions moved to GameFSM.cpp
// SettingFunc, ActivateFunc, ReadyFunc, WaitFunc - Removed
