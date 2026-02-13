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

  NeopixelInit();
  delay(100);
  RfidInit();
  delay(100);
  MotorInit();
  delay(100);
  EncoderInit();
  delay(100);
  NextionInit();
  delay(100);
  TimerInit();
  delay(100);
  // has2wifi.Setup("KT_GiGA_6C64","ed46zx1198");
  has2wifi.Setup("badland_ruins", "Code3824@");
  delay(100);
  //    has2wifi.Setup("city");
  DataChanged();
}
void loop() {
  gameFSM.performStateLogic();
  TimerRun();
  delay(1);
}
