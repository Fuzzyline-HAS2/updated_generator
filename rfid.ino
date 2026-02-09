#include "GameFSM.h"
#include "generator.h"

// --- Constants ---
const uint8_t PAGE_ADDR = 7;
const uint8_t PACKET_HEADER = 0x00;
const int TAG_DATA_SIZE = 32;
const int UID_SIZE = 7;
const int CHECK_DELAY = 100;
const int LOGIN_DELAY = 3500;

// --- Function Prototypes ---
bool ReadTagData(uint8_t *dataBuffer);
void ProcessTagData(uint8_t *dataBuffer);
void ResetLogoutTimer();
void HandlePlayerTag();
void HandleTaggerTag();
void HandleGhostTag();
void HandleStarterFinishLogin();
void HandleBatteryMaxLogin();
void HandleBatteryChargeLogin();
void HandleBatteryCharging();
void HandleBatteryFull();

// --- Main RFID Functions ---

void RfidInit() {
RestartPn532:
  nfc[MAINPN532].begin();

  uint32_t versiondata = nfc[MAINPN532].getFirmwareVersion();
  if (!versiondata) {
    Serial.println("PN532 FAIL : MAINPN532");
    AllNeoOn(RED);
    delay(1000); // Add delay to prevent rapid looping
    goto RestartPn532;
  } else {
    nfc[MAINPN532].SAMConfig();
    Serial.println("PN532 SUCC : MAINPN532");
    rfid_init_complete[MAINPN532] = true;
  }
  delay(CHECK_DELAY);
}

void RfidLoopMain() {
  uint8_t data[TAG_DATA_SIZE];

  for (int i = 0; i < rfid_num; ++i) {
    // Check if tag is detected and read data
    if (ReadTagData(data)) {
      Serial.println("TAG DETECTED");
      CheckingPlayers(data);
    }
  }
}

// Helper: Check for tag presence and read data page
bool ReadTagData(uint8_t *dataBuffer) {
  byte packetBuffer[64];
  packetBuffer[0] = PACKET_HEADER;

  // 1. Check communication status
  if (!nfc[MAINPN532].sendCommandCheckAck(packetBuffer, 1)) {
    return false;
  }

  // 2. Detect passive target (Tag)
  if (!nfc[MAINPN532].startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A)) {
    return false;
  }

  // 3. Read specific page from NTAG
  return nfc[MAINPN532].ntag2xx_ReadPage(PAGE_ADDR, dataBuffer);
}

void CheckingPlayers(uint8_t rfidData[32]) {
  String tagUser = "";
  ResetLogoutTimer();

  // Extract GxPx data (first 4 bytes)
  for (int i = 0; i < 4; i++) {
    tagUser += (char)rfidData[i];
  }

  Serial.println("Tag User: " + tagUser);

  if (tagUser == "MMMM") { // Staff Card - Reset
    ESP.restart();
    return;
  }

  has2wifi.Receive(tagUser); // Fetch user role from server

  String role = (String)(const char *)tag["role"];

  if (role == "player") {
    Serial.println("Player Tagged");
    if (ptrRfidMode)
      ptrRfidMode();
  } else if (role == "tagger") {
    Serial.println("Tagger Tagged");
    // Handle tagger logic if needed
  } else if (role == "ghost") {
    Serial.println("Ghost Tagged");
    // Handle ghost logic if needed
  } else {
    Serial.println("Unknown Role Tagged");
  }
}

void LoginGenerator() {
  Serial.println("LoginGenerator Executing...");

  BatteryPackSend();
  PageSend();
  SendCmd("page pgLogin");
  LeftGenerator();

  delay(LOGIN_DELAY);

  BatteryPackSend();

  // Restart Logout Timer
  ResetLogoutTimer();
  LogoutTimer.setInterval(logoutTime, LogoutTimerFunc);

  String deviceState = (String)(const char *)my["device_state"];

  if (deviceState == "starter_finish") {
    HandleStarterFinishLogin();
  } else if (deviceState == "battery_max") {
    HandleBatteryMaxLogin();
  } else {
    HandleBatteryChargeLogin();
  }
}

void HandleStarterFinishLogin() {
  AllNeoOn(GREEN);
  LeftGenerator();

  detachInterrupt(encoderPinA);
  detachInterrupt(encoderPinB);

  EngineSpeeed(250);

  ptrRfidMode = StartFinish;
  ptrCurrentMode = RfidLoopMain;

  lightColor(pixels[GAUGE], color[BLUE]);

  BlinkTimer.deleteTimer(blinkTimerId);
  BlinkTimerStart(CIRCUIT, YELLOW);
}

void HandleBatteryMaxLogin() {
  Serial.println("Login: Battery Max State");
  BatteryFinish(); // Transition to next phase
}

void HandleBatteryChargeLogin() {
  Serial.println("Login: Charging Needed");
  ptrRfidMode = BatteryPackCharge;

  // Check if already full (edge case)
  if ((int)my["battery_pack"] >= (int)my["max_battery_pack"]) {
    HandleBatteryFull();
  }
}

void BatteryPackCharge() {
  Serial.println("BatteryPackCharge Executing...");
  ResetLogoutTimer();

  int currentPack = (int)my["battery_pack"];
  int maxPack = (int)my["max_battery_pack"];
  int playerPack = (int)tag["battery_pack"];

  if (currentPack >= maxPack) {
    // Already full -> Move to finish
    BatteryFinish();
    return;
  }

  if (playerPack > 0) {
    // Perform Charge
    HandleBatteryCharging();
  } else {
    Serial.println("Player has no battery packs.");
  }
}

void HandleBatteryCharging() {
  SendCmd("wBatteryCharge.en=1");
  Serial.println("Charging...");

  // Update Server
  has2wifi.Send((String)(const char *)tag["device_name"], "battery_pack", "-1");
  has2wifi.Send((String)(const char *)my["device_name"], "battery_pack", "+1");

  receiveMineOn = true; // Block DataChanged update momentarily
  has2wifi.ReceiveMine();

  delay(10);
  BatteryPackSend();
  delay(2000);

  if ((int)my["battery_pack"] >= (int)my["max_battery_pack"]) {
    HandleBatteryFull();
  }
}

void HandleBatteryFull() {
  AllNeoOn(GREEN);
  Serial.println("Battery Full!");

  SendCmd("page pgBatteryMax");
  delay(100);
  SendCmd("wBatteryFull.en=1");

  BlinkTimer.deleteTimer(blinkTimerId);
  BlinkTimerStart(CIRCUIT, YELLOW);
}

void BatteryFinish() {
  Serial.println("Battery Finish!");

  has2wifi.Send((String)(const char *)my["device_name"], "device_state",
                "battery_max");

  SendCmd("page pgStarter");
  delay(10);
  SendCmd("wStaterOn.en=1");
  LeftGenerator();

  ResetLogoutTimer();
  AllNeoOn(GREEN);

  // Start Game Timer/Encoder logic
  encoderValue = 1;
  GameTimer.deleteTimer(gameTimerId);
  gameTimerCnt = 0;
  gameTimerId = GameTimer.setInterval(gameTime, GameTimerFunc);

  BlinkTimer.deleteTimer(blinkTimerId);
  BlinkTimerStart(STARTER, YELLOW);

  ptrCurrentMode = StarterActivate; // Switch main loop logic

  attachInterrupt(encoderPinA, updateEncoder, CHANGE);
  attachInterrupt(encoderPinB, updateEncoder, CHANGE);
}

void StartFinish() {
  Serial.println("StartFinish Executing...");

  LogoutTimer.deleteTimer(logoutTimerId);
  GameTimer.deleteTimer(gameTimerId);
  BlinkTimer.deleteTimer(blinkTimerId);

  Serial.println("Generator Fixed!");
  SendCmd("page pgFixed");

  has2wifi.Send((String)(const char *)my["device_name"], "device_state",
                "repaired");

  receiveMineOn = true;
  has2wifi.ReceiveMine();

  LeftGenerator();
  AllNeoOn(BLUE);

  ptrCurrentMode = WaitFunc;
}

void ResetLogoutTimer() {
  logoutTimerCnt = 0;
  // If we needed to restart the timer explicitly, we would do it here.
  // Currently, the timer runs continuously and checks logoutTimerCnt.
}
