# Build Instructions

Build this application from the app root:

`/Users/andrei/notebooks/NeutonAI/west_workspaces/nrf54lm20b-axon-audio-models/app/wakeword_kws`

This project uses `sysbuild` and targets:

`nrf54lm20dk/nrf54lm20b/cpuapp`

## Preferred Commands

Pristine build:

```sh
west build -p always -b nrf54lm20dk/nrf54lm20b/cpuapp --sysbuild -d build .
```

Incremental rebuild using the existing build directory:

```sh
west build -b nrf54lm20dk/nrf54lm20b/cpuapp --sysbuild -d build .
```

Flash the already-built image:

```sh
west flash -d build
```

## Notes

- Run the commands inside an activated nRF Connect SDK west workspace.
- `sample.yaml` sets `sysbuild: true`, so keep `--sysbuild` in the build command.
- If an old build directory fails with `ccache: command not found`, do a pristine rebuild instead of reusing the stale directory:

```sh
west build -p always -b nrf54lm20dk/nrf54lm20b/cpuapp --sysbuild -d build .
```
