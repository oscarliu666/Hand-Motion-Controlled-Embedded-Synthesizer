#include <esp32-hal-dac.h>
#include <Preferences.h>
#include "Adafruit_VL53L0X.h"
#include "signal_gen.h"
#include "ulcd.h"

#define DAC_PIN 25
#define POT_PIN 34
#define MUTE_PIN 27

/* Nav switch buttons */
#define BUTTON_UP   17
#define BUTTON_DOWN 23
#define BUTTON_RIGHT 19
#define BUTTON_LEFT 18

Adafruit_VL53L0X lox = Adafruit_VL53L0X();

/* Variables for DDS */
uint32_t freq;
uint32_t phase;
uint32_t tuning_word;

uint16_t volume;
uint16_t pot_val;
uint8_t octave = O4;

/**
  Originally we tried smoothing the sensor input by averaging, but we decided to use
  interpolation instead, so these variables are unused. Used code from Arduino as a 
  reference for the unused code on smoothing.

  https://docs.arduino.cc/built-in-examples/analog/Smoothing/
 */
#define NUM_TO_AVG 10
uint32_t freq_buf[NUM_TO_AVG];
uint8_t read_idx = 0;
uint32_t total = 0;;
uint32_t avg = 0;

/* Index to select from list of LUTs (sine, square, saw, triangle)*/
uint8_t lut_index= 0;

/* HW timer to trigger the interrupt to update the DAC */
hw_timer_t *timer;

/**
  Change the selected sound with the down switch and save the settings to non-volatile
  memory.
 */
void ulcd_change_wave() {
    // Debouncing
    static unsigned long lastChange = 0;
    unsigned long now = millis();
    if (now - lastChange < 150) return;

    if (!digitalRead(BUTTON_DOWN)) {
        lut_index = (lut_index + 3) % 4;
        ulcd_update_wave((Wave)lut_index);
        lastChange = now;
    }
    ulcd_save_settings(lut_index, volume); 
}

/**
  Change the selected octave with the up and down switches and save the settings to memory.
 */
void ulcd_change_octave() {
    static unsigned long lastChange = 0;
    unsigned long now = millis();
    if (now - lastChange < 150) return;

    if (!digitalRead(BUTTON_LEFT)) {
        octave = (octave + 1) % 3;
        lastChange = now;
    } else if (!digitalRead(BUTTON_RIGHT)) {
        octave = (octave + 2) % 3;
        lastChange = now;
    }
}

void ARDUINO_ISR_ATTR incFcw()
{
  phase += tuning_word;
  uint8_t out = (volume / 255.0) * lut_list[lut_index][get_table_idx(phase)];
  dacWrite(DAC_PIN, out);
}

void setup() {
  pinMode(BUTTON_UP,   INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
  pinMode(BUTTON_LEFT,  INPUT_PULLUP);
  pinMode(MUTE_PIN,  INPUT_PULLUP);

  /**
    Code to initialize unused buffer for smoothing.
   */
  for (int i = 0; i < NUM_TO_AVG; i++)
    freq_buf[i] = 0;

  // put your setup code here, to run once:
  Serial.begin(115200);

  // Initialize default frequency and phase
  freq = 130;
  phase = 0;

  ulcd_init();

  ulcd_load_settings(&lut_index, &volume);

  ulcd_update_wave((Wave)lut_index);
  ulcd_update_volume(volume);

  /**
    Used sample code from https://github.com/adafruit/Adafruit_VL53L0X 
    for initializing sensor. 
   */
  Serial.println("Adafruit VL53L0X test");
  if (!lox.begin()) { 
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  }

  /**
    Create the 10 MHz timer. The interrupt triggers at 44100 KHz (22.7 us).
   */
  timer = timerBegin(REFCLK);
  timerAttachInterrupt(timer, &incFcw);
  uint64_t prescaler = REFCLK / SAMPLE_RATE;
  timerAlarm(timer, prescaler, true, 0);

  // Set the volume on the screen
  ulcd_update_volume(map(analogRead(POT_PIN), 0, 4095, 0, 255));
}

void loop() {
  ulcd_change_wave();
  ulcd_change_octave();

  pot_val = map(analogRead(POT_PIN), 0, 4095, 0, 255);

  /**
    Referred to https://github.com/adafruit/Adafruit_VL53L0X as starting point
    for code to read from sensor. 
   */

  // Get sensor reading
  VL53L0X_RangingMeasurementData_t measure;
  uint16_t sample;
  lox.rangingTest(&measure, false);

  // if out of range, turn off the sound
  if (measure.RangeStatus == 4 || !digitalRead(MUTE_PIN))
  {
    volume = 0;
    ulcd_update_note(13);
  }
  else
  {
    total = total - freq_buf[read_idx];

    // Map sensor reading to frequency
    uint32_t range_low = 131;
    uint32_t range_high = 262;
    uint16_t min_height = 30;
    uint16_t max_height = 400;
    uint8_t scale_size = 13;

    // Map raw sensor reading to frequency
    uint32_t mapped_freq = map(measure.RangeMilliMeter, 30, 400, range_low, range_high);

    // Find the nearest frequency by basically flooring the mapped value.
    float width = (max_height - min_height) / (float)scale_size;
    uint32_t nearest_freq_index = (uint32_t)((measure.RangeMilliMeter - min_height + 1) / width);
    nearest_freq_index = constrain(nearest_freq_index, 0, scale_size - 1);

    uint32_t nearest_freq = c3_scale[nearest_freq_index];

    /**
      Now use linear interpolation to mix the directly mapped frequency to the nearest.
      Alpha increases when the mapped frequency is closer to the nearest frequency, 
      which causes the final blended output frequency to be closer to the nearest frequency.
    */

    // Dynamic adjustment of blend factor alpha
    float distance = fabs(mapped_freq - nearest_freq);
    // 10 Hz around the note
    float snap_range = 10.0;
    float alpha = 1.0 - (distance / snap_range);
    
    // The max blend factor is 0.6
    alpha = constrain(alpha, 0.0, 0.6); 

    /** 
      The (1 << octave) is to multiply the frequency by a power of two based on the current
      octave. Notes at different octaves are multiples of the same frequency.
    */
    float blended = (alpha * nearest_freq + (1 - alpha) * mapped_freq) * (1 << octave);
    freq = (uint32_t)blended;

    // Set tuning word to properly update phase
    tuning_word = get_tuning_word(freq);
    volume = pot_val;
    ulcd_update_volume(volume);

    ulcd_update_note(nearest_freq_index);

    // avg = total / NUM_TO_AVG;
  }
  // ulcd_save_settings(lut_index);
  // delay(10);
}
