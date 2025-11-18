#ifndef ULCD_UI_H
#define ULCD_UI_H

#include <Preferences.h>
#include "Goldelox_Serial_4DLib.h"
#include "Goldelox_Const4D.h"
#include <Arduino.h>
#include <string.h>

// #define TX_PIN      6
// #define RX_PIN      7
// #define RESETLINE   4
// #define BUTTON_UP   20
// #define BUTTON_DOWN 19
// #define POT_PIN     0
#define TX_PIN      17   // UART2 TX
#define RX_PIN      16   // UART2 RX
#define RESETLINE   4

#define BUTTON_UP   18
#define BUTTON_DOWN 19

#define POT_PIN     34


// 显示相关的全局对象（header-only 用 static）
static HardwareSerial DisplaySerial(1);
static Goldelox_Serial_4DLib Display(&DisplaySerial);

// NVM 存储
static Preferences UlcdPref;

//
static int  g_lastVolume = -1;
static unsigned long g_lastVolumeUpdate = 0;

static char g_lastNote[8] = "";
static int  g_lastFreq = -1;
static unsigned long g_lastPitchUpdate = 0;



// ============ 初始化 UI ============
static inline void Ulcd_init() {
  pinMode(RESETLINE, OUTPUT);
  digitalWrite(RESETLINE, LOW);
  delay(100);
  digitalWrite(RESETLINE, HIGH);
  delay(500);

  DisplaySerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  Display.TimeLimit4D = 5000;

  Display.gfx_Cls();
  Display.txt_FontID(5);
  Display.txt_BGcolour(BLACK);
  Display.txt_Set(TEXT_COLOUR, WHITE);
  Display.txt_Set(TEXT_HEIGHT, 1.7);
  Display.txt_Set(TEXT_WIDTH, 1.7);

  Display.txt_MoveCursor(1, 2); Display.putstr("Wave:");
  Display.txt_MoveCursor(5, 2); Display.putstr("Vol:");
  Display.txt_MoveCursor(9, 2); Display.putstr("Pit:");
}


// ============ 画 Wave ============
static inline void UlcdUI_drawWave(const char* wave) {
  char buf[10];
  strcpy(buf, wave);

  Display.txt_MoveCursor(1, 7);
  Display.putstr("     ");   // clear old
  Display.txt_MoveCursor(1, 7);
  Display.putstr(buf);
}

// ============ 画 Volume ============

static inline void drawVolume(int vol) {
  if (vol < 0)   vol = 0;
  if (vol > 100) vol = 100;

  char buf[10];
  sprintf(buf, "%d%%", vol);

  Display.txt_MoveCursor(5, 7);
  Display.putstr("      ");
  Display.txt_MoveCursor(5, 7);
  Display.putstr(buf);
}

// ============ 读 POT 映射到 0–100 ============
static inline int readPot() {
  uint16_t raw = analogRead(POT_PIN);
  int v = map(raw, 256, 3300, 0, 100);
  if (v < 0)   v = 0;
  if (v > 100) v = 100;
  return v;
}

// ============ 处理按钮，改变 waveMode 字符串 + 更新屏幕 ============
static inline void NavWave(char* waveMode) {
  if (!digitalRead(BUTTON_UP)) {
    UlcdUI_drawWave("SQUA");
    strcpy(waveMode, "SQUA");
    delay(150);
  } else if (!digitalRead(BUTTON_DOWN)) {
    UlcdUI_drawWave("SINE");
    strcpy(waveMode, "SINE");
    delay(150);
  }
}

// ============ 画 Pitch（note + freq） ============
static inline void drawPitch(const char* note, int freq) {
  char buf[20];
  sprintf(buf, "%s (%dHz)", note, freq);

  Display.txt_MoveCursor(9, 7);
  Display.putstr("                ");
  Display.txt_MoveCursor(9, 7);
  Display.putstr(buf);
}

// 读电位器 + 只在改变时刷新 Volume
static inline void updateVolumeFromPot() {
  uint16_t raw = analogRead(POT_PIN);
  int vol = map(raw, 256, 3300, 0, 100);
  if (vol < 0)   vol = 0;
  if (vol > 100) vol = 100;

  unsigned long now = millis();

  // 1. 如果没变，就不要画
  if (vol == g_lastVolume) return;

  // 2. 限制刷新频率（比如至少间隔 80ms）
  if (now - g_lastVolumeUpdate < 100) return;

  drawVolume(vol);
  g_lastVolume = vol;
  g_lastVolumeUpdate = now;
}

// 只在 note/freq 变化时刷新 Pitch
static inline void updatePitchIfChanged(const char* note, int freq) {
  unsigned long now = millis();

  // note 或 freq 任意一个变了，才更新
  if ( (freq == g_lastFreq) && (strcmp(note, g_lastNote) == 0) ) {
    return;
  }

  // 限制刷新频率，比如 100ms
  if (now - g_lastPitchUpdate < 100) return;

  drawPitch(note, freq);

  g_lastFreq = freq;
  strncpy(g_lastNote, note, sizeof(g_lastNote));
  g_lastNote[sizeof(g_lastNote)-1] = '\0';

  g_lastPitchUpdate = now;
}


// ============ 保存设置到 NVM ============
static inline void saveSettings(const char* waveMode, int volume) {
  UlcdPref.begin("ulcd", false);
  UlcdPref.putString("wave", waveMode);
  UlcdPref.putInt("vol", volume);
  UlcdPref.end();
}

// ============ 从 NVM 读取设置 ============
static inline void loadSettings(char* waveOut, size_t maxLen, int* volOut) {
  UlcdPref.begin("ulcd", true);

  String w = UlcdPref.getString("wave", "SINE"); //如果曾经存过值，比如 "SQUARE"，就读取出来赋给 w//如果没存过就用默认值 "SINE"
  int v    = UlcdPref.getInt("vol", 50);

  strncpy(waveOut, w.c_str(), maxLen);
  waveOut[maxLen - 1] = '\0';   // 防止没结束符
  *volOut = v;

  UlcdPref.end();
}

#endif // ULCD_UI_H
