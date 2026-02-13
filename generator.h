#ifndef _DONE_ITEMBOX_CODE_
#define _DONE_ITEMBOX_CODE_

#include "Library_and_pin.h"
const int rfid_num = 1; // 설치된 pn532의 개수

//****************************************WIFI****************************************************************
//****************************************WIFI****************************************************************
extern HAS2_Wifi has2wifi;
void DataChanged();

//****************************************Game
// System****************************************************************
#include "GameFSM.h" // Include the new FSM header
extern bool receiveMineOn;


void WaitFunc();
void StarterActivate(); // Keep this as it is used as a loop function
extern const unsigned long starterNeoDivider;
extern bool blinkOn;

//****************************************Timer
// System****************************************************************
extern SimpleTimer GameTimer;
extern SimpleTimer WifiTimer;
extern SimpleTimer LogoutTimer;
extern SimpleTimer BlinkTimer;

void TimerInit();
void WifiIntervalFunc();
void GameTimerFunc();
void LogoutTimerFunc();

void BlinkTimerFunc();
void BlinkTimerStart(int Neo, int NeoColor);
extern int blinkNeo;
extern int blinkColor;

extern int wifiTimerId;
extern int gameTimerId;
extern int logoutTimerId;
extern int blinkTimerId;

extern unsigned long wifiTime;
extern unsigned long logoutTime;
extern unsigned long gameTime;
extern unsigned long blinkTime;

extern volatile unsigned int logoutTimerCnt;
extern volatile unsigned int gameTimerCnt;
//****************************************Nextion
// SETUP****************************************************************
extern HardwareSerial nexHwSerial;
extern String nextion_str;
extern int currner_page_num;
void NextionInit();
void DisplayCheck();
void NextionReceived(String *nextion_string);
void LeftGenerator();
void BatteryPackSend();
void PageSend();
void SendCmd(String command);

//****************************************Neopixel
// SETUP****************************************************************
void NeopixelInit();
void EncoderNeopixelOn(int neoNum);
void NeoBlink(int neo, int neoColor, int cnt, int blinkTime);
void AllNeoOn(int neoColor); // Added declaration
// Helper function declaration only
void lightColor(Adafruit_NeoPixel &p, int *c);

extern const int NumPixels[4];
const int NeopixelNum = 4; // Keep const in header (internal linkage)
enum { GAUGE = 0, STARTER, DEVICESTATE, CIRCUIT };
enum {
  WHITE = 0,
  RED,
  YELLOW,
  GREEN,
  BLUE,
  PURPLE,
  BLACK,
  BLUE0,
  BLUE1,
  BLUE2,
  BLUE3
};
// Neopixel 색상정보
extern int color[11][3];

const int neopixel_num = 4; // 설치된 네오픽셀의 개수

extern Adafruit_NeoPixel pixels[NeopixelNum];

//****************************************RFID
// SETUP****************************************************************
enum { MAINPN532 = 0 };

extern Adafruit_PN532 nfc[rfid_num];

extern bool rfid_tag;
extern bool rfid_timer_assess;

extern bool rfid_init_complete[rfid_num];
void RfidInit(void);
void RfidLoopMain(void); // Renamed/Added from RfidLoop
void WaitFunc(); // Ensure this is declared

//****************************************MOTOR
// SETUP****************************************************************
const int MotorFreq = 5000;
const int MotorResolution = 8;
const int MotorLedChannel = 3;
const int MotorMAX_DUTY_CYCLE = (int)(pow(2, MotorResolution) - 1);
void MotorInit();
void EngineSpeeed(int enginePwm);
void EngineStop();

//****************************************ENCODER
// SETUP****************************************************************
void EncoderInit();
long readEncoderValue(void);
void updateEncoder();

extern volatile int lastEncoded;   // 직전 엔코더 값
extern volatile long encoderValue; // 현재 엔코더 값

extern long lastencoderValue;

extern int lastMSB;
extern int lastLSB;

#endif
