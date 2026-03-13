# nrf54lm20b-axon-audio-models

Audio ML sample applications for nRF54LM20B using Axon and nRF Edge AI.

This repository is a west manifest project and currently contains these applications:

- [Snoring Detection](app/snoring_detection/README.md)

## 1) Prerequisites

Install the following:

- nRF Connect for Desktop (including Toolchain Manager)
- VS Code
- nRF Connect for VS Code extension pack
- SEGGER J-Link drivers
- Nordic board: `nrf54lm20dk/nrf54lm20b/cpuapp`

Recommended SDK/toolchain baseline in this repo:

- NCS `v3.3.0-preview2` (from `west.yml`)

## nRF EdgeAI Library Version Used

- nRF EdgeAI version: `2.2.0`
- Library/module location: `lib/nrf_edgeai`

## Axon Driver Version Used

- Axon driver version: `1.1.0`
- Driver/module location: `lib/axon`

## 2) Initialize west Workspace (CLI)

Use this if you are setting up from scratch with this repository as the manifest root.

```powershell
# Clone this repository first, then run from the repo root
west init -l .
west update
west zephyr-export
```

Optional verification:

```powershell
west list
west topdir
```

## 3) Build Samples with west (CLI)

All commands below are run from repository root.

### Building Example (Snoring Detection)

```powershell
west build -p always --sysbuild -b nrf54lm20dk/nrf54lm20b/cpuapp app/snoring_detection -d build/snoring_detection
```

Notes:

- `--sysbuild` is expected for these applications.
- `-p always` ensures a pristine rebuild; use `-p auto` or omit for faster incremental builds.

## 4) Flash Samples with west (CLI)

Connect the DK over USB, then flash from the matching build directory.

### Flashing Example (Snoring Detection)

```powershell
west flash -d build/snoring_detection
```

If multiple boards/debug probes are connected, specify a runner option as needed (for example serial number).

## 5) Build and Flash with nRF Connect for VS Code

Use this path if you prefer GUI workflows.

### 5.1 Open and Configure Workspace

1. Open this repository in VS Code.
2. Open nRF Connect extension view.
3. Ensure Toolchain and SDK point to a compatible NCS installation (matching `v3.3.0-preview2` if possible).
4. Add an application:
	 - For Snoring: `app/snoring_detection`
5. Select board: `nrf54lm20dk/nrf54lm20b/cpuapp`.
6. Enable Sysbuild in the build configuration.

### 5.2 Build

1. Click Build Configuration.
2. Build from the nRF Connect side panel.
3. Use Pristine Build if you change Kconfig, devicetree overlays, toolchain, or board target.

### 5.3 Flash

1. Connect the board and verify it appears in Connected Devices.
2. Select the build configuration.
3. Click Flash.

### 5.4 Monitor Logs (optional)

- Open a serial terminal for RTT/UART logs depending on your debug setup.
- Confirm startup prints for the selected sample.

## 6) Sample Descriptions

### Snoring Detection ([app/snoring_detection/README.md](app/snoring_detection/README.md))

Continuously captures PDM microphone audio, feeds it into an nRF Edge AI snoring model, applies rolling confidence postprocessing to reduce false positives, and signals detection events by logging and blinking LED0.

## 7) Troubleshooting

- Build fails due to missing modules:
	- Run `west update` again from repo root.
- Build target mismatch:
	- Confirm board is exactly `nrf54lm20dk/nrf54lm20b/cpuapp`.
- Flash fails to connect:
	- Check USB cable, J-Link driver, and that no other tool is holding the probe.
- Incorrect runtime behavior after config changes:
	- Rebuild with pristine (`-p always`).