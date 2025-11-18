#include "ulcd.h"

char waveMode[10];
int volume = 0;

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_UP,   INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(POT_PIN,     INPUT);

  Ulcd_init();

  loadSettings(waveMode, sizeof(waveMode), &volume);
  UlcdUI_drawWave(waveMode);
  drawVolume(volume);
}

void loop() {

  NavWave(waveMode);
  updateVolumeFromPot();


  const char* note = "C5";
  int freq = 523;
  updatePitchIfChanged(note, freq);

  // 可以考虑每隔几百毫秒再保存一次，而不是每次 loop 都写 flash
  // saveSettings(waveMode, 当前的g_lastVolume);
  delay(10);
}
