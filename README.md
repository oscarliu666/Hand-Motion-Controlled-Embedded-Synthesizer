# Digital Theremin
The Digital Theremin is a motion-controlled distance-based synthesizer. The user controls the synthesizer’s pitch with their hand height; by raising their hand higher, they can play higher notes. Through the hand control, this project offers a more interactive way of making digital music compared to regular keyboard-based synths, combining the novelty of playing an analog instrument with the variety of sounds a digital synthesizer can produce. Other gesture-controlled synthesizers are available, but they use more complicated computer vision systems and can be cost-prohibitive. 

## Circuit Diagram
<img width="1052" height="593" alt="image" src="https://github.com/user-attachments/assets/70f1b38e-9f62-4677-b1c1-ddcb15886f15" />


## Peripherals
- ESP32-WROVER-E (main microcontroller and DAC output source)
- VL53LOX time-of-flight sensor (gesture-controlled pitch input over I²C)
- uLcd-144G2 (UART communication, UI for wave/volume/pitch)
- Sparkfun 5 Way Tactile Switch (waveform and octave selection as UI navigation)
- Potentiometer (analog volume control)
- Speaker (sound output)

The time-of-flight (TOF) sensor is used to measure the relative height of the user's hand. The user can switch between sounds/waveforms and octaves with a 5-way navigation switch, using the down switch to switch between sine, square, sawtooth, and triangle waves, and the up and down keys to change between three octaves. The instrument's note range is C3-C5. A potentiometer is provided to adjust the volume of the speaker's output audio. Pressing the nav switch mutes the audio, allowing the user to play rests. The uLCD screen is used to display the current note being played, the current output waveform, and the volume. The selected waveform is saved to non-volatile memory, allowing settings to be stored between power cycles.

## Major Challenges
### Implementing the synthesis
To emulate the glissando of an analog theremin, we needed a method of generating sine waves at any arbritrary frequency while also avoiding expensive real-time sin calculation. We found Direct Digital Synthesis (DDS), which uses a lookup table (LUT) and a calculated value proportional to the desired frequency to efficiently generate a signal at any frequency. 

To implement DDS, we need a phase variable that starts at 0. At a sampling rate of 44100 Hz, or every 22.7 us, the phase variable gets incremented by a 32-bit value called the tuning word, which is proportional to the frequency. The first 10 bits of the tuning word are used to index into a LUT with 1024 elements; there are LUTs for each different sound in `signal_gen.h`.

$$\text{tuning word} = 2^{32} \cdot \frac{f}{\text{sampling rate}}$$

We referred to Doug Couler's *Digital Audio Processing* and the [Analog Devices DDS tutorial](https://www.ieee.li/pdf/essay/dds.pdf) as guides for our implementation; these resources did not contain any code.

To ensure proper timing, we used a 10 MHz timer with a prescaler set so that an interrupt would trigger every 22.7 us, or at a 44100 Hz sampling rate. Originally, we used an ESP32-C6 and the 12-bit I2C DAC to output the signal. However, we had issues getting a clean sine wave output because the latency of communicating with the DAC over I2C was greater than our sample rate. Assuming we send three 9-bit I2C packets at 400 kHz, it would take 45 us to communicate with the DAC, not including the overhead introduced by Arduino's Wire library--this is at a lower frequency than the sample rate. We still had issues even when we lowered the sample rate to 5000 Hz, which is not enough for high-quality audio. 

The best solution was to switch to the ESP32-WROVER, which has a built-in 8-bit DAC. Despite its lower resolution, it is much faster to send data to than the I2C DAC, which fixed our issues with synthesis.

### Mapping Sensor Input to Output Sound
When directly mapping the sensor input to a specific frequency, we found that it was very hard to play when we tested the system for the first time. To somewhat alleviate this, we decided to add some interpolation. By storing an array holding the frequencies for the 13 notes between C3 and C4 and "binning" the measured distance from the sensor into one of 13 divisions, the synthesizer can find an approximate nearest note. Then for a blend factor $\alpha$, we can use the linear interpolation formula to "mix" the mapped value and the nearest value together.

$$f_{blended} = \alpha f_{nearest} + (1 - \alpha) f_{mapped}$$

The lower the alpha value, the smoother the note changes are, and vice versa. For example, an alpha of 0.0 would not factor in the nearest frequency at all, but an alpha of 1 would only include the nearest frequency, making the note transitions sound discrete and choppy. By combining the mapped and nearest frequencies, we can achieve some smoothness/glissando between note changes like a theremin, while also making the output sound closer to the notes on a chromatic scale, making it easier to play. 

Furthermore, by letting alpha adjust dynamically, starting from 0.0 and getting closer to 0.6 as the mapped sensor input approaches a note boundary, the user can reach correct notes even more easily, while keeping smoother gliding when not close to a note. With a higher alpha, the mapped frequency hass less impact, decreasing the effects of fluctuations from the sensor readings that get translated into the mapped frequency.

## Citations/Libraries Used
- Adafruit_VL53L0X.h
    - https://github.com/adafruit/Adafruit_VL53L0X
- Goldelox_Serial_4DLib.h
    - https://github.com/4dsystems/Goldelox-Serial-Arduino-Library
- Goldelox_Const4D.h
    - https://github.com/4dsystems/Goldelox-Serial-Arduino-Library
- esp32-hal-dac.h
    - https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-dac.h
- Preferences.h
    - https://github.com/espressif/arduino-esp32/blob/d1270e54b64729f8e13a9fc43f806999430e5f2e/libraries/Preferences/src/Preferences.h
    - https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/preferences.html
- ESP32 HW Timer
    - https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-timer.h
    - https://docs.espressif.com/projects/arduino-esp32/en/latest/api/timer.html
- Lookup Table Generation
    - https://ppelikan.github.io/drlut/
- Reference code for smoothing (unused and doesn't affect functionality, but still appears in source)
    - https://docs.arduino.cc/built-in-examples/analog/Smoothing/

## Demo Video
[![Motion Synthesizer Demo](https://img.youtube.com/vi/aG5xdUHbO2w/maxresdefault.jpg)](https://youtube.com/shorts/aG5xdUHbO2w)
## Future Work
We are planning on adding a MIDI interface so that users can connect the synthesizer to their computers and record their playing, allowing for more serious music production. We are also considering more complicated preset sounds, which would be created using methods like FM synthesis or subtractive synthesis.

We are also considering looking for longer-range distance sensors. Our time-of-flight sensor could only consistently detect a maximum hand height of 400 mm above the sensor. A longer range sensor would allow users to achieve a high note range without having to switch octaves as much with the nav switch.

We are also interested in exploring alternative modes of input, such a TFT touch screen, to make it a more immersive and interactive system. We hope to develop a circular UI in which the outer part works as a rotary encoder to adjust the pitch in real-time. The user can scroll the outer part of the circle with the pitch displayed in the center of the circular UI.

Finally, we are thinking of designing an external enclosure for the entire system. A custom 3D-printed case would not only improve aesthetics but also protect the electronics and make the device safer and more durable for everyday use.
