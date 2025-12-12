#include "Goldelox_Serial_4DLib.h"
#include "Goldelox_Const4D.h"
#include "ulcd.h"
#include <Preferences.h>

//pins settings for ulcd
#define TX_PIN      14  
#define RX_PIN      13   
#define RESETLINE   4

//object for lib
HardwareSerial DisplaySerial(1);
Goldelox_Serial_4DLib Display(&DisplaySerial);
static Preferences UlcdPref;

//initialization for ulcd
void ulcd_init()
{
  //reboot ulcd
  pinMode(RESETLINE, OUTPUT);
  digitalWrite(RESETLINE, LOW);
  delay(100);
  digitalWrite(RESETLINE, HIGH);
  delay(500);

  DisplaySerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  Display.TimeLimit4D = 5000;

//UI initialization
  Display.gfx_Cls();
  Display.txt_FontID(5);
  Display.txt_BGcolour(BLACK);
  Display.txt_Set(TEXT_COLOUR, WHITE);
  Display.txt_Set(TEXT_HEIGHT, 1.7);
  Display.txt_Set(TEXT_WIDTH, 1.7);

//defult display elements
  Display.txt_MoveCursor(1, 2); Display.putstr("Wave: ");
  Display.txt_MoveCursor(5, 2); Display.putstr("Vol: ");
  Display.txt_MoveCursor(9, 2); Display.putstr("Note: ");
  Serial.println("Initialized ULCD screen");
}

//display waveform
void ulcd_update_wave(Wave wave)
{
  char buf[16];
  switch (wave)
  {
    case 0:
      snprintf(buf, sizeof(buf), "Sine");break;
    case 1:
      snprintf(buf, sizeof(buf), "Triangle");break;
    case 2:
      snprintf(buf, sizeof(buf), "Sawtooth");break;
    case 3:
      snprintf(buf, sizeof(buf), "Square");break;
    default:
      snprintf(buf, sizeof(buf), "");break;
  }
  //Serial.println("updated");

  Display.txt_MoveCursor(1, 7);
  Display.putstr("        ");   // clear old
  Display.txt_MoveCursor(1, 7);
  Display.putstr(buf);
}

//display volume and save the setting in NVM
void ulcd_update_volume(uint16_t vol)
{
  //naje volume percentage
  float display_vol = (vol / 255.0) * 100;
  char buf[16];
  //alignment for display
  if (display_vol < 10)
    snprintf(buf, sizeof(buf), "%.2f%%  ", display_vol);
  else if (display_vol < 100)
    snprintf(buf, sizeof(buf), "%.2f%% ", display_vol);
  else
    snprintf(buf, sizeof(buf), "%.2f%%", display_vol);

  Display.txt_MoveCursor(5, 7);
  // Display.putstr("      ");
  Display.txt_MoveCursor(5, 7);
  Display.putstr(buf);
  ulcd_save_settings(lut_index, vol);


}
//display note
void ulcd_update_note(uint16_t freq_index)
{
  char* notes[] = { "C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B ", "C ", "  "};

  Display.txt_MoveCursor(9, 7);
  // Display.putstr("      ");
  Display.txt_MoveCursor(9, 7);
  Display.putstr(notes[freq_index]);
}
//function for saving settings into NVM
void ulcd_save_settings(uint8_t wave_index, uint16_t vol)
{
  // Previous saved values.
  // Initialized with invalid numbers to force a save the first time.
  static uint8_t lastWave = 255;
  static uint16_t lastVol = 65535;
  static unsigned long lastWrite = 0;
  unsigned long now = millis();

  //ensures we only save meaningful changes and avoid spamming NVS
  if (now - lastWrite < 1000 &&
      wave_index == lastWave &&
      abs((int)vol - (int)lastVol) < 5)
    return;

  UlcdPref.begin("msynth", false);   // write mode
  UlcdPref.putInt("wave", (int)wave_index);
  UlcdPref.putInt("vol",  (int)vol);
  UlcdPref.end();
  // Update values.
  lastWave = wave_index;
  lastVol = vol;
  lastWrite = now;
}

//functions for loading settings from NVM
void ulcd_load_settings(uint8_t *wave_index, uint16_t *vol)
{
  UlcdPref.begin("msynth", true);   // read-only mode

  int w = UlcdPref.getInt("wave", 0);      // defult 0 = Sine

  UlcdPref.end();

  if (w < 0 || w > 3) w = 0;

  *wave_index = (uint8_t)w;
}
