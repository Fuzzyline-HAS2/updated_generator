void TimerInit() {
  wifiTimerId = WifiTimer.setInterval(wifiTime, WifiIntervalFunc);
  gameTimerId = GameTimer.setInterval(gameTime, GameTimerFunc);
  logoutTimerId = LogoutTimer.setInterval(logoutTime, LogoutTimerFunc);
  blinkTimerId = BlinkTimer.setInterval(blinkTime, BlinkTimerFunc);

  GameTimer.deleteTimer(gameTimerId);
  LogoutTimer.deleteTimer(logoutTimerId);
  BlinkTimer.deleteTimer(blinkTimerId);
}

void WifiIntervalFunc() { has2wifi.Loop(DataChanged); }

void GameTimerFunc() { gameFSM.enqueueTimeoutGame(); }
void LogoutTimerFunc() {
  logoutTimerCnt++;
  // Serial.println("LogoutTimerCnt:" + (String)logoutTimerCnt);
  if (logoutTimerCnt >= 12) {
    Serial.println("LogOutTimer TimeOUT");
    LogoutTimer.deleteTimer(logoutTimerId); // 로그아웃 타이머 종료
    GameTimer.deleteTimer(gameTimerId);
    BlinkTimer.deleteTimer(blinkTimerId);
    EngineStop();
    gameFSM.enqueueTimeoutLogout();
  }
}

void BlinkTimerFunc() {
  // Serial.println("Blink!");
  if (blinkOn == true) {
    lightColor(pixels[blinkNeo], color[blinkColor]);
    blinkOn = false;
  } else {
    lightColor(pixels[blinkNeo], color[BLACK]);
    blinkOn = true;
  }
}
void BlinkTimerStart(int Neo, int NeoColor) {
  blinkNeo = Neo;
  blinkColor = NeoColor;
  blinkTimerId = BlinkTimer.setInterval(blinkTime, BlinkTimerFunc);
}

void TimerRun() {
  WifiTimer.run();
  GameTimer.run();
  LogoutTimer.run();
  BlinkTimer.run();
}
