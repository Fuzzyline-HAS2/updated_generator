#include "GameFSM.h"
#include "generator.h"

void WaitFunc() {
  // Empty function for idle state
}

void StarterActivate() {
  int gaugeNeoCnt = encoderValue / 4000;
  // Serial.println(gaugeNeoCnt);
  EncoderNeopixelOn(gaugeNeoCnt);
  EngineSpeeed(gaugeNeoCnt * 8);
}
