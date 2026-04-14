# Wakeword KWS Application

## Description
This application performs wakeword-gated keyword spotting (KWS) from a PDM microphone stream on the nRF54LM20DK.

Audio is captured from DMIC continuously. The application first waits for the wakeword "Okay Nordic". After wakeword detection, it switches to keyword spotting mode for a 7-second active window, logs detected keywords with confidence, and returns to wakeword mode when the window times out.

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
