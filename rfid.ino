#include "GameFSM.h"
#include "generator.h"

const uint8_t PAGE_ADDR = 7;
const uint8_t PACKET_HEADER = 0x00;
const int TAG_DATA_SIZE = 32;
const int CHECK_DELAY = 100;
const int TAG_OFF_TIMEOUT_MS = 250;

bool ReadTagData(uint8_t *dataBuffer);
void CheckingPlayers(uint8_t rfidData[32]);
void ResetLogoutTimer();

void RfidInit() {
  const uint8_t kMaxInitRetry = 5;
  rfid_init_complete[MAINPN532] = false;

  for (uint8_t attempt = 1; attempt <= kMaxInitRetry; ++attempt) {
    nfc[MAINPN532].begin();
    uint32_t versiondata = nfc[MAINPN532].getFirmwareVersion();
    if (versiondata) {
      nfc[MAINPN532].SAMConfig();
      Serial.println("PN532 SUCC : MAINPN532");
      rfid_init_complete[MAINPN532] = true;
      delay(CHECK_DELAY);
      return;
    }

    Serial.printf("PN532 FAIL : MAINPN532 (attempt %u/%u)\r\n", attempt,
                  kMaxInitRetry);
    AllNeoOn(RED);
    delay(200);
    yield();
  }

  Serial.println("PN532 INIT SKIPPED: continue boot without RFID");
}

void RfidLoopMain() {
  if (!rfid_init_complete[MAINPN532]) {
    return;
  }

  uint8_t data[TAG_DATA_SIZE];
  static bool tagDetected = false;
  static unsigned long lastDetectMs = 0;
  bool detectedNow = false;

  for (int i = 0; i < rfid_num; ++i) {
    if (ReadTagData(data)) {
      detectedNow = true;
      lastDetectMs = millis();
      if (!tagDetected) {
        gameFSM.enqueueTagOn();
        tagDetected = true;
      }
      CheckingPlayers(data);
    }
  }

  if (!detectedNow && tagDetected &&
      (millis() - lastDetectMs) > TAG_OFF_TIMEOUT_MS) {
    gameFSM.enqueueTagOff();
    tagDetected = false;
  }
}

bool ReadTagData(uint8_t *dataBuffer) {
  byte packetBuffer[64];
  packetBuffer[0] = PACKET_HEADER;

  if (!nfc[MAINPN532].sendCommandCheckAck(packetBuffer, 1)) {
    return false;
  }
  if (!nfc[MAINPN532].startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A)) {
    return false;
  }
  return nfc[MAINPN532].ntag2xx_ReadPage(PAGE_ADDR, dataBuffer);
}

void CheckingPlayers(uint8_t rfidData[32]) {
  String tagUser = "";
  ResetLogoutTimer();

  for (int i = 0; i < 4; i++) {
    tagUser += (char)rfidData[i];
  }

  if (tagUser == "MMMM") {
    ESP.restart();
    return;
  }

  has2wifi.Receive(tagUser);
  String role = (String)(const char *)tag["role"];

  if (role == "player") {
    gameFSM.enqueueNetCmd("player_tag");
  }
}

void ResetLogoutTimer() { logoutTimerCnt = 0; }
