# MOTION-SYNTHESIZER
## Expected Behavior
This system is a motion-controlled distance-based synthesizer based on ESP-WROVER. The user controls the synthesizer’s pitch with their hand height; by raising their hand higher, they can play higher notes. The user can switch audio waveforms and octave with a 5-way navigation switch; left and right keys to switch within Sine, Square, and Triangle; up and down keys to adjust frequency from the base note. A potentiometer is provided to adjust the volume of the speaker's output audio, and there is a push button to mute the audio at any time. The uLCD screen will display the real-time audio waveform, the volume (percentage), and the pitch. The waveforms and the volume will be automatically  saved in the Non-Volatile memory (NVM), maintaining the same settings at next power-on.
## Peripherals
- ESP32-WROVER (main microcontroller and DAC output source)
- TOF sensor VL53LOX (gesture-controlled pitch input over I²C)
- uLcd-144G2 (UART communication, UI for wave/volume/pitch)
- Sparkfun 5 Way Tactile Switch (waveform and octave selection as UI navigation)
- Potentiometer (analog volume control)
- Push button (interrupt to mute the Synthesizer)
- Speaker (sound output)
## Libraries used in Arduino IDE
- Adafruit_VL53L0X.h
- esp32-hal-dac.h
- Preferences.h
- Goldelox_Serial_4DLib.h
- Goldelox_Const4D.h

## Schematic Diagram
Here is the schematic diagram for the circuit.
![image](https://github.gatech.edu/user-attachments/assets/3135417c-fa21-4202-84f5-a7e5e6973dfd)



## Why Unique
## Challenges
## Future Work

We are considering adding a TFT touch screen to make it a more immersive and interactive system. We hope to develop a circular UI in which the outer part works as a rotary encoder to adjust the audio frequency in real-time. The user can scroll the outer part of the circle with the frequency displayed in the center of the circular UI.

Additionally, we are considering designing an external enclosure for the entire system. A custom 3D-printed case would not only improve aesthetics but also protect the electronics and make the device safer and more durable for everyday use.


