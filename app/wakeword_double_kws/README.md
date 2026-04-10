# Wakeword Double KWS Application

## Description
This application performs wakeword-gated two-word command spotting from a PDM microphone stream on the nRF54LM20DK.

Audio is captured from DMIC continuously. The application first waits for the wakeword "Okay Nordic". After wakeword detection, it switches to keyword spotting mode for a 7-second active window. In this mode it listens for two-word command phrases composed of a first word followed by a second word. The active window resets on every successfully detected command phrase. If no command is detected within the timeout the application returns to wakeword mode.

Supported commands:
- START RUNNING
- START SWIMMING
- SWITCH ACTIVITY
- END ACTIVITY
- END SLEEP
- LOG MEAL

Ignored classes: OTHER, SILENCE
