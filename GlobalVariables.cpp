#include "generator.h"
#include <Adafruit_NeoPixel.h>
#include <Adafruit_PN532.h>

//****************************************WIFI****************************************************************
HAS2_Wifi has2wifi("http://172.30.1.43");

//****************************************Game
// System****************************************************************
bool receiveMineOn = false; // Added missing definition
const unsigned long starterNeoDivider = 15000 * 7.5;
bool blinkOn = false;

//****************************************Timer
// System****************************************************************
SimpleTimer GameTimer;
SimpleTimer WifiTimer;
SimpleTimer LogoutTimer;
SimpleTimer BlinkTimer;

int blinkNeo = 0;
int blinkColor = 0;

int wifiTimerId;
int gameTimerId;
int logoutTimerId;
int blinkTimerId;

unsigned long wifiTime = 2000;   // 1sec
unsigned long logoutTime = 5000; // 5ses 로그
unsigned long gameTime = 400;    // 3sec
unsigned long blinkTime = 1800;  // 1sec

volatile unsigned int logoutTimerCnt;
volatile unsigned int gameTimerCnt;

//****************************************Nextion
// SETUP****************************************************************
HardwareSerial nexHwSerial(2); // Added missing definition

String nextion_str = "";
int currner_page_num;

//****************************************Neopixel
// SETUP****************************************************************
// Helper function implementation (moved from generator.h)
void lightColor(Adafruit_NeoPixel &p, int *c) {
  uint32_t colorVal = p.Color(c[0], c[1], c[2]);
  p.fill(colorVal);
  p.show();
}

const int NumPixels[4] = {28, 4, 16, 10};
// const int NeopixelNum = 4; // Int literals are often fine in headers if
// static, but let's be consistent or assume "extern const" if needed. const in
// C++ defaults to internal linkage, so "const int NeopixelNum = 4;" in header
// is fine (each TU gets its own copy). Arrays like NumPixels might be an issue
// if address is taken, but usually fine. Using 'extern' for them to be
// text-book correct in preventing dupes if linker complains, but const usually
// ok. However, the linker complained about "NumPixels" too? Let's check linker
// output. Linker error: "multiple definition of `pixels'" Linker error:
// "multiple definition of `color'" Linker error: "multiple definition of
// `lightColor'" Linker error: "multiple definition of `blinkNeo'" It did NOT
// complain about "NumPixels" or "NeopixelNum" in the error list provided (maybe
// truncated or maybe because they are const). I will move non-consts and
// objects.

int color[11][3] = {{20, 20, 20}, // WHITE
                    {40, 0, 0},   // RED
                    {40, 40, 0},  // YELLOW
                    {0, 40, 0},   // GREEN
                    {0, 0, 40},   // BLUE
                    {40, 0, 40},  // PURPLE
                    {0, 0, 0},    // BLACK
                    {0, 0, 20},   // ENCODERBLUE0
                    {0, 0, 40},   // ENCODERBLUE1
                    {0, 0, 60},   // ENCODERBLUE2
                    {0, 0, 80}};  // ENCODERBLUE3

Adafruit_NeoPixel pixels[NeopixelNum] = {
    Adafruit_NeoPixel(NumPixels[GAUGE], PN532_NEOPIXEL_PIN,
                      NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(NumPixels[STARTER], ENCODER_NEOPIXEL_PIN,
                      NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(NumPixels[DEVICESTATE], INNER_NEOPIXEL_PIN,
                      NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(NumPixels[CIRCUIT], CIRCUIT_NEOPIXEL_PIN,
                      NEO_GRB + NEO_KHZ800)};

//****************************************RFID
// SETUP****************************************************************
Adafruit_PN532 nfc[rfid_num] = {
    Adafruit_PN532(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS1)};

bool rfid_tag;
bool rfid_timer_assess;
bool rfid_init_complete[rfid_num];

//****************************************ENCODER
// SETUP****************************************************************
volatile int lastEncoded = 0;   // 직전 엔코더 값
volatile long encoderValue = 0; // 현재 엔코더 값
long lastencoderValue = 0;
int lastMSB = 0;
int lastLSB = 0;
