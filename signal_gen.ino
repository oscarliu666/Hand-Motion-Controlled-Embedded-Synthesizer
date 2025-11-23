#include <esp32-hal-dac.h>
#include <Preferences.h>
#include "Adafruit_VL53L0X.h"
#include "signal_gen.h"
#include "ulcd.h"

#define DAC_PIN 25
#define POT_PIN 34
#define MUTE_PIN 27

#define BUTTON_UP   16
#define BUTTON_DOWN 23
#define BUTTON_RIGHT 19
#define BUTTON_LEFT 18


#define C3 130
#define C4 262

#define NUM_SAMPLES 20

Adafruit_VL53L0X lox = Adafruit_VL53L0X();

uint16_t samples[NUM_SAMPLES];
int readIndex = 0;
int total = 0;
int average = 0;

uint32_t freq;
uint32_t phase;
uint32_t tuning_word;
uint16_t volume;
uint16_t pot_val;

uint8_t lut_index= 0;

hw_timer_t *timer;

// void ARDUINO_ISR_ATTR onVolumeChange()
// {
//   ulcd_update_volume(volume);
// }
void ulcd_change_wave() {
    static unsigned long lastChange = 0;
    unsigned long now = millis();
    if (now - lastChange < 150) return;

    if (!digitalRead(BUTTON_RIGHT)) {
        Serial.println("right.");

        lut_index = (lut_index + 1) % 4;
        ulcd_update_wave((Wave)lut_index);
        lastChange = now;

    } else if (!digitalRead(BUTTON_LEFT)) {

        // 向左：减 1 = 加 3 mod 4 （避免 -1）
        lut_index = (lut_index + 3) % 4;
        ulcd_update_wave((Wave)lut_index);
        lastChange = now;
    }
    ulcd_save_settings(lut_index, volume); 

    
}

void ARDUINO_ISR_ATTR incFcw()
{
  phase += tuning_word;
  // uint8_t out = (uint8_t)(amp * lut[getTableIdx(phase)]);
  if (digitalRead(MUTE_PIN)){
    volume = 0;}
  // uint8_t out = (pot_val / 255.0) * lut[getTableIdx(phase)];
  uint8_t out = (volume / 255.0) * lut_list[lut_index][getTableIdx(phase)];
  dacWrite(DAC_PIN, out);
}

void setup() {
  pinMode(BUTTON_UP,   INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
  pinMode(BUTTON_LEFT,  INPUT_PULLUP);

  // UlcdPref.begin("msynth", false);
  // UlcdPref.clear();
  // UlcdPref.end();


  for (int i = 0; i < NUM_SAMPLES; i++)
  {
    samples[i] = 0;
  }

  // put your setup code here, to run once:
  Serial.begin(115200);

  freq = C3;
  phase = 0;
  tuning_word = getFreqCtrlWord(freq);

  ulcd_init();

  ulcd_load_settings(&lut_index, &volume);

  ulcd_update_wave((Wave)lut_index);
  ulcd_update_volume(volume);



  Serial.println("Adafruit VL53L0X test");
  if (!lox.begin()) { 
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  }

  timer = timerBegin(REFCLK);
  timerAttachInterrupt(timer, &incFcw);
  uint64_t prescaler = REFCLK / SAMPLE_RATE;
  timerAlarm(timer, prescaler, true, 0);

  pinMode(MUTE_PIN, INPUT_PULLDOWN);

  // lut = sin_lut;
}

void loop() {

  ulcd_change_wave();

  pot_val = map(analogRead(POT_PIN), 0, 4095, 0, 255);

  // Map sensor value to frequency
  VL53L0X_RangingMeasurementData_t measure;
  total = total - samples[readIndex];
    
  // Serial.print("Reading a measurement... ");
  lox.rangingTest(&measure, false);
  if (measure.RangeStatus != 4) {
    samples[readIndex] = measure.RangeMilliMeter;
  } else {
    // Serial.println(" out of range ");
    samples[readIndex] = 30;
  }

  uint16_t sample = samples[readIndex];
  total += sample;
  readIndex = (readIndex + 1) % NUM_SAMPLES;
  average = total / NUM_SAMPLES;

  uint8_t freq_index = 0;
  if (sample <= 30)
  {
    freq_index = 0;
    // freq = C3;
    volume = 0;
  }
  // else if (sample <= 60)
  // {
  //   // freq = C4;
  //   freq_index = 0;
  //   volume = pot_val;
  // }
  else if (sample >= 300)
  {
    freq_index = SCALE_SIZE - 1;
    volume = pot_val;
  }
  else
  {
    float width = (300 - 30) / SCALE_SIZE;
    freq_index = (uint16_t)((sample - 30) / width);
    // freq = map(sample, 60, 300, C3, C4) << 1;
    volume = pot_val;
  }
  freq = c3_scale[freq_index] << 1;
  tuning_word = getFreqCtrlWord(freq);

  ulcd_update_volume(volume);
  Serial.println(volume);
  ulcd_save_settings(lut_index, volume);
  delay(10);
}
