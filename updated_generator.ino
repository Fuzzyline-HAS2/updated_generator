; /**
   * @file Done_Generator_code.ino
   * @author 김병준 (you@domain.com)
   * @brief
   * @version 1.0
   * @date 2022-11-29
   *
   * @copyright Copyright (c) 2022
   *
   */

#include "GameFSM.h"
#include "generator.h"

void setup() {
  Serial.begin(115200);
  Serial.println("[BOOT] setup start");

  Serial.println("[BOOT] NeopixelInit");
  NeopixelInit();
  delay(100);
  Serial.println("[BOOT] RfidInit");
  RfidInit();
  delay(100);
  Serial.println("[BOOT] MotorInit");
  MotorInit();
  delay(100);
  Serial.println("[BOOT] EncoderInit");
  EncoderInit();
  delay(100);
  Serial.println("[BOOT] NextionInit");
  NextionInit();
  delay(100);
  Serial.println("[BOOT] TimerInit");
  TimerInit();
  delay(100);
  // has2wifi.Setup("KT_GiGA_6C64","ed46zx1198");
  Serial.println("[BOOT] Wifi Setup");
  has2wifi.Setup("badland_ruins", "Code3824@");
  delay(100);
  //    has2wifi.Setup("city");
  Serial.println("[BOOT] DataChanged");
  DataChanged();
  Serial.println("[BOOT] setup done");
}
void loop() {
  gameFSM.performStateLogic();
  TimerRun();
  delay(1);
}
