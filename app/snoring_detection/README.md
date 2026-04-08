# Snoring Detection Application

## Description
This application performs continuous snoring detection from a PDM microphone stream on the nRF54LM20DK.

Audio is captured through the PDM interface, processed with an nRF Edge AI model, and post-processed with rolling confidence logic to reduce unstable detections. The application can drive LEDs to indicate detection state.