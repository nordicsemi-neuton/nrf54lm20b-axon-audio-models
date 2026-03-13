# Snoring Detection Application

## Description
This application performs continuous snoring detection from a PDM microphone stream on the nRF54LM20DK.

Audio is captured through the PDM interface, processed with an nRF Edge AI model, and post-processed with rolling confidence logic to reduce unstable detections. The application can drive LEDs to indicate detection state.

## Hardware Configuration

### Target Board
- nRF54LM20DK

### Microphone
- PDM microphone breakout board: https://www.adafruit.com/product/3492

Use the project DTS overlay as the source of truth for active pin mapping in this app:
- `boards/nrf54lm20dk_nrf54lm20b_cpuapp.overlay`

Current overlay PDM signals:
- PDM clock: `P1.04`
- PDM data in: `P1.05`

### PDM microphone breakout board wiring
1. Wire microphone 3V to VDDIO and GND to GND on the PDK.
2. Wire SEL microphone pin to the GND on the PDK, so PDM data is sampled on the clock rising edge. 
3. Wire microphone CLK to P1-04 and DAT to P1-05 on the PDK

Note: The microphone is 1.8 V to 3.3 V tolerant.

## Notes
If hardware wiring does not match the current DTS overlay pin selection, update either wiring or overlay so both use the same CLK/DAT pins.
