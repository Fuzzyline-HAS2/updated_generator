void NeopixelInit() {
  for (int i = 0; i < NeopixelNum; ++i) {
    pixels[i].begin();
  }
  for (int i = 0; i < NeopixelNum; ++i) {
    for (int j = 0; j < pixels[i].numPixels(); j++) {
      pixels[i].setPixelColor(
          j,
          pixels[i].Color(color[WHITE][0], color[WHITE][1], color[WHITE][2]));
    }
    pixels[i].show();
  }
}

void EncoderNeopixelOn(int neoNum) {
  for (int i = 0; i < neoNum; i++)
    pixels[GAUGE].setPixelColor(
        i, pixels[GAUGE].Color(color[BLUE][0], color[BLUE][1], color[BLUE][2]));
  for (int i = neoNum; i < NumPixels[GAUGE]; i++)
    pixels[GAUGE].setPixelColor(
        i,
        pixels[GAUGE].Color(color[GREEN][0], color[GREEN][1], color[GREEN][2]));
  pixels[GAUGE].show();
}
void NeoBlink(int neo, int neoColor, int cnt, int blinkTime) {
  for (int i = 0; i < cnt; i++) {          // 0.5*10=5초동안 점멸
    lightColor(pixels[neo], color[BLACK]); // 전체 off
    delay(blinkTime);
    lightColor(pixels[neo], color[neoColor]); // 전체 적색on
    delay(blinkTime);                         // 전체 적색on
  }
}

void AllNeoOn(int neoColor) {
  for (int i = 0; i < NeopixelNum; ++i)
    lightColor(pixels[i], color[neoColor]);
}
