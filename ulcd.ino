#include "Goldelox_Serial_4DLib.h"
#include "Goldelox_Const4D.h"
#include "ulcd.h"

#define TX_PIN      14   // UART2 TX
#define RX_PIN      13   // UART2 RX  2
#define RESETLINE   4

HardwareSerial DisplaySerial(1);
Goldelox_Serial_4DLib Display(&DisplaySerial);

void ulcd_init()
{
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

  Display.txt_MoveCursor(1, 2); Display.putstr("Wave: ");
  Display.txt_MoveCursor(5, 2); Display.putstr("Vol: ");
  Display.txt_MoveCursor(9, 2); Display.putstr("Note: ");
  Serial.println("Initialized ULCD screen");
}

void ulcd_update_wave(Wave wave)
{
  char *buf;
  switch (wave)
  {
    case SIN:
      buf = "Sine";
    case TRI:
      buf = "Triangle";
    case SAW:
      buf = "Sawtooth";
    case SQU:
      buf = "Square";
    default:
      buf = "";
  }

  Display.txt_MoveCursor(1, 7);
  Display.putstr("     ");   // clear old
  Display.txt_MoveCursor(1, 7);
  Display.putstr(buf);
}

void ulcd_update_volume(uint16_t vol)
{
  float display_vol = (vol / 255.0) * 100;
  char buf[16];

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
}