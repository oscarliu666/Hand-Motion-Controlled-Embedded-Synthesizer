#include "signal_gen.h"
#include <esp32-hal-dac.h>
#include "Adafruit_VL53L0X.h"

#define DAC_PIN 25
#define POT_PIN 34

#define C3 130
#define C4 262

#define NUM_SAMPLES 10

Adafruit_VL53L0X lox = Adafruit_VL53L0X();
uint16_t samples[NUM_SAMPLES];
int readIndex = 0;          // the index of the current reading
int total = 0;              // the running total
int average = 0;            // the average

uint32_t freq;
uint32_t phase;
uint32_t tuning_word;

volatile uint16_t pot_val;

hw_timer_t *timer;

void ARDUINO_ISR_ATTR incFcw()
{
  phase += tuning_word;
  // uint8_t out = (uint8_t)(amp * lut[getTableIdx(phase)]);
  uint8_t out = (pot_val / 255.0) * lut[getTableIdx(phase)];
  dacWrite(DAC_PIN, out);
}

void setup() {
  for (int i = 0; i < NUM_SAMPLES; i++)
  {
    samples[i] = 0;
  }

  // put your setup code here, to run once:
  Serial.begin(115200);

  freq = C3;
  phase = 0;
  tuning_word = getFreqCtrlWord(freq);

  Serial.println("Adafruit VL53L0X test");
  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  }
  // power 
  Serial.println(F("VL53L0X API Simple Ranging example\n\n")); 

  timer = timerBegin(REFCLK);
  timerAttachInterrupt(timer, &incFcw);
  uint64_t prescaler = REFCLK / SAMPLE_RATE;
  timerAlarm(timer, prescaler, true, 0);
}

void loop() {
  pot_val = map(analogRead(POT_PIN), 0, 4095, 0, 255);

  // Map sensor value to frequency
  VL53L0X_RangingMeasurementData_t measure;
  total = total - samples[readIndex];
    
  // Serial.print("Reading a measurement... ");
  lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!

  if (measure.RangeStatus != 4) {  // phase failures have incorrect data
    // Serial.print("Distance (mm): "); Serial.println(tuning_word);
    samples[readIndex] = measure.RangeMilliMeter;
  } else {
    // Serial.println(" out of range ");
    samples[readIndex] = 30;
  }
  total += samples[readIndex];
  readIndex = (readIndex + 1) % NUM_SAMPLES;
  average = total / NUM_SAMPLES;

  freq = map(samples[readIndex], 30, 300, C3, C4);
  uint16_t sample = samples[readIndex]
  freq = constrain(freq, C3, C4);
  tuning_word = getFreqCtrlWord(freq);
  // Serial.println(phase);


  delay(10);
}
