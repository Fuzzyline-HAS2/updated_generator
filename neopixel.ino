#include "generator.h"
#include <Adafruit_NeoPixel.h>

// Helper to get uint32_t color from global color array
uint32_t GetColor(int colorIdx) {
  return pixels[0].Color(color[colorIdx][0], color[colorIdx][1],
                         color[colorIdx][2]);
}

void NeopixelInit() {
  uint32_t white = GetColor(WHITE);
  for (int i = 0; i < NeopixelNum; ++i) {
    pixels[i].begin();
    pixels[i].fill(white);
    pixels[i].show();
  }
}

void EncoderNeopixelOn(int neoNum) {
  uint32_t blue = GetColor(BLUE);
  uint32_t green = GetColor(GREEN);

  pixels[GAUGE].fill(blue, 0, neoNum);
  pixels[GAUGE].fill(green, neoNum); // Fills from neoNum to end
  pixels[GAUGE].show();
}

void NeoBlink(int neo, int neoColor, int cnt, int blinkTime) {
  uint32_t targetColor = GetColor(neoColor);
  uint32_t black = GetColor(BLACK);

  for (int i = 0; i < cnt; i++) {
    pixels[neo].fill(black);
    pixels[neo].show();
    delay(blinkTime);

    pixels[neo].fill(targetColor);
    pixels[neo].show();
    delay(blinkTime);
  }
}

void AllNeoOn(int neoColor) {
  uint32_t targetColor = GetColor(neoColor);
  for (int i = 0; i < NeopixelNum; ++i) {
    pixels[i].fill(targetColor);
    pixels[i].show();
  }
}
