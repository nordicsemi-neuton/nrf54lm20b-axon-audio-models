# Wakeword KWS Application

## Description
This application performs wakeword-gated keyword spotting (KWS) from a PDM microphone stream on the nRF54LM20DK.

Audio is captured from DMIC continuously. The application first waits for the wakeword "Okay Nordic". After wakeword detection, it switches to keyword spotting mode for a 7-second active window, logs detected keywords with confidence, and returns to wakeword mode when the window times out.

The repository also contains a desktop UART visualizer for raw model outputs:
- [tools/serial_visualizer.py](/Users/andrei/notebooks/NeutonAI/west_workspaces/nrf54lm20b-axon-audio-models/app/visualizer/tools/serial_visualizer.py)
- The visualizer auto-discovers series from metadata sent by the firmware, so the PC side does not need hardcoded wakeword/KWS class names.

Supported keywords:
- Down
- Go
- Left
- No
- Off
- On
- Right
- Silence
- Stop
- Unknown
- Up
- Yes

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
2. Wire SEL microphone pin to GND on the PDK so PDM data is sampled on the clock rising edge.
3. Wire microphone CLK to P1-04 and DAT to P1-05 on the PDK.

Note: The microphone is 1.8 V to 3.3 V tolerant.

## Notes
If hardware wiring does not match the current DTS overlay pin selection, update either wiring or overlay so both use the same CLK/DAT pins.

## UART Visualizer

Run the desktop app with:

```bash
python3 /Users/andrei/notebooks/NeutonAI/west_workspaces/nrf54lm20b-axon-audio-models/app/visualizer/tools/serial_visualizer.py
```

Optional startup arguments:

```bash
python3 /Users/andrei/notebooks/NeutonAI/west_workspaces/nrf54lm20b-axon-audio-models/app/visualizer/tools/serial_visualizer.py \
  --port /dev/tty.usbmodem0010518634983 \
  --baudrate 115200
```

The script shows:
- a dropdown with available serial ports
- a rolling time window similar to an oscilloscope
- labeled X/Y axes
- one line for wakeword or multiple lines for KWS classes

Runtime modes:
- If the active Python has `tkinter + matplotlib`, the script starts the desktop GUI directly.
- If those modules are unavailable, the same script automatically falls back to a local browser UI and prints a `http://127.0.0.1:...` URL.
- Browser mode requires Chrome or Edge because it uses the Web Serial API.

Browser fallback example:

```bash
/opt/homebrew/bin/python3 /Users/andrei/notebooks/NeutonAI/west_workspaces/nrf54lm20b-axon-audio-models/app/visualizer/tools/serial_visualizer.py --no-browser
```

### UART Protocol

Transport format: one JSON object per line (`JSON Lines`).

The current firmware example exposes one compile-time switch in [src/main.cpp](/Users/andrei/notebooks/NeutonAI/west_workspaces/nrf54lm20b-axon-audio-models/app/visualizer/src/main.cpp):
- `VISUALIZER_STREAM_MODE = VISUALIZER_STREAM_WAKEWORD`
- `VISUALIZER_STREAM_MODE = VISUALIZER_STREAM_KWS`

By default, [src/main.cpp](/Users/andrei/notebooks/NeutonAI/west_workspaces/nrf54lm20b-axon-audio-models/app/visualizer/src/main.cpp) also sets `VISUALIZER_RUN_SELECTED_MODEL_ONLY = 1`, so the firmware continuously streams the selected model output without the wakeword-gated demo flow.

If you use `VISUALIZER_STREAM_KWS`, update the placeholder `KWS_VISUALIZER_LABELS` array to match the real output order of your trained model.

The firmware should first send one metadata packet:

```json
{"type":"meta","mode":"wakeword","x_label":"Time (s)","y_label":"Probability","series":[{"name":"Okay Nordic"}]}
```

For KWS, the same metadata packet contains all class names:

```json
{"type":"meta","mode":"kws","x_label":"Time (s)","y_label":"Probability","series":[{"name":"class_0"},{"name":"class_1"},{"name":"class_2"}]}
```

Then it should continuously send frames:

```json
{"type":"frame","timestamp_ms":1234,"entities":[{"name":"Okay Nordic","value":0.83}]}
```

```json
{"type":"frame","timestamp_ms":1250,"entities":[{"name":"class_0","value":0.01},{"name":"class_1","value":0.92},{"name":"class_2","value":0.07}]}
```

Protocol assumptions:
- `wakeword` uses exactly one entity in `entities`
- `kws` uses one entity per output class
- series names should come from firmware, because the current repo does not contain a reliable source of truth for all KWS labels
- non-JSON log lines are ignored by the visualizer, so regular `printk` logs can stay enabled
