#!/usr/bin/env python3
"""Real-time UART visualizer for wakeword/KWS model outputs.

Expected transport: one JSON object per line.

Metadata example:
{"type":"meta","mode":"wakeword","x_label":"Time (s)","y_label":"Probability","series":[{"name":"Okay Nordic"}]}

Frame example:
{"type":"frame","timestamp_ms":1234,"values":[0.83],"count":1}
"""

from __future__ import annotations

import argparse
import functools
import glob
import http.server
import json
import math
import os
import pathlib
import queue
import select
import sys
import tempfile
import threading
import time
import urllib.parse
import webbrowser
from collections import deque
from dataclasses import dataclass

if "MPLCONFIGDIR" not in os.environ:
    os.environ["MPLCONFIGDIR"] = os.path.join(tempfile.gettempdir(), "model-visualizer-mpl")

if "XDG_CACHE_HOME" not in os.environ:
    os.environ["XDG_CACHE_HOME"] = os.path.join(tempfile.gettempdir(), "model-visualizer-cache")

os.makedirs(os.environ["MPLCONFIGDIR"], exist_ok=True)
os.makedirs(os.environ["XDG_CACHE_HOME"], exist_ok=True)

DESKTOP_GUI_AVAILABLE = False
DESKTOP_GUI_ERROR: str | None = None

try:
    import tkinter as tk
    from tkinter import messagebox, ttk

    import matplotlib

    matplotlib.use("TkAgg")

    from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
    from matplotlib.figure import Figure

    DESKTOP_GUI_AVAILABLE = True
except Exception as exc:
    tk = None
    ttk = None
    messagebox = None
    Figure = None
    FigureCanvasTkAgg = None
    DESKTOP_GUI_ERROR = str(exc)

if os.name == "posix":
    import termios
else:
    termios = None

WINDOW_SECONDS = 15.0
PLOT_UPDATE_MS = 50
DEFAULT_BAUDRATE = 115200
DEFAULT_X_LABEL = "Time (s)"
DEFAULT_Y_LABEL = "Probability"
SERIAL_PORT_PATTERNS = (
    "/dev/tty.*",
    "/dev/cu.*",
    "/dev/ttyACM*",
    "/dev/ttyUSB*",
)
COMMON_BAUDRATES = ("9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600")


@dataclass
class MetaMessage:
    mode: str | None
    x_label: str
    y_label: str
    series_names: list[str]


@dataclass
class FrameMessage:
    timestamp_s: float
    entities: dict[str, float]
    values: list[float] | None = None


def list_serial_ports() -> list[str]:
    ports: set[str] = set()

    for pattern in SERIAL_PORT_PATTERNS:
        ports.update(glob.glob(pattern))

    return sorted(ports)


def _baudrate_flag(baudrate: int) -> int:
    if termios is None:
        raise RuntimeError("POSIX serial configuration is required for this tool.")

    name = f"B{baudrate}"
    if not hasattr(termios, name):
        raise ValueError(f"Unsupported baudrate: {baudrate}")

    return getattr(termios, name)


def open_serial_port(port_path: str, baudrate: int) -> int:
    if termios is None:
        raise RuntimeError("This visualizer currently supports POSIX serial ports only.")

    baud_flag = _baudrate_flag(baudrate)
    fd = os.open(port_path, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)

    try:
        attrs = termios.tcgetattr(fd)
        attrs[0] = termios.IGNPAR
        attrs[1] = 0
        attrs[2] = termios.CLOCAL | termios.CREAD | termios.CS8
        attrs[3] = 0
        attrs[4] = baud_flag
        attrs[5] = baud_flag
        attrs[6][termios.VMIN] = 0
        attrs[6][termios.VTIME] = 1

        termios.tcflush(fd, termios.TCIFLUSH)
        termios.tcsetattr(fd, termios.TCSANOW, attrs)
    except Exception:
        os.close(fd)
        raise

    return fd


def _series_names_from_meta(payload: dict) -> list[str]:
    names: list[str] = []

    for item in payload.get("series", []):
        if isinstance(item, str):
            names.append(item)
            continue

        if isinstance(item, dict):
            name = item.get("name")
            if isinstance(name, str) and name:
                names.append(name)

    return names


def parse_json_line(line: str) -> MetaMessage | FrameMessage | None:
    stripped = line.strip()
    if not stripped or not stripped.startswith("{"):
        return None

    try:
        payload = json.loads(stripped)
    except json.JSONDecodeError:
        return None

    if not isinstance(payload, dict):
        return None

    message_type = payload.get("type")

    if message_type == "meta":
        series_names = _series_names_from_meta(payload)
        if not series_names:
            return None

        return MetaMessage(
            mode=payload.get("mode") if isinstance(payload.get("mode"), str) else None,
            x_label=payload.get("x_label") if isinstance(payload.get("x_label"), str) else DEFAULT_X_LABEL,
            y_label=payload.get("y_label") if isinstance(payload.get("y_label"), str) else DEFAULT_Y_LABEL,
            series_names=series_names,
        )

    if message_type not in (None, "frame"):
        return None

    entities: dict[str, float] = {}
    values: list[float] | None = None

    raw_entities = payload.get("entities")
    if isinstance(raw_entities, list):
        for item in raw_entities:
            if not isinstance(item, dict):
                continue

            name = item.get("name")
            value = item.get("value")
            if isinstance(name, str) and isinstance(value, (int, float)):
                entities[name] = float(value)

    raw_values = payload.get("values")
    if isinstance(raw_values, list):
        parsed_values = [float(value) for value in raw_values if isinstance(value, (int, float))]
        if parsed_values:
            values = parsed_values

    if not entities and values is None:
        return None

    if isinstance(payload.get("timestamp_ms"), (int, float)):
        timestamp_s = float(payload["timestamp_ms"]) / 1000.0
    elif isinstance(payload.get("timestamp_s"), (int, float)):
        timestamp_s = float(payload["timestamp_s"])
    else:
        timestamp_s = time.monotonic()

    return FrameMessage(timestamp_s=timestamp_s, entities=entities, values=values)


class SerialReader(threading.Thread):
    def __init__(self, port_path: str, baudrate: int, output_queue: queue.Queue) -> None:
        super().__init__(daemon=True)
        self._port_path = port_path
        self._baudrate = baudrate
        self._output_queue = output_queue
        self._stop_event = threading.Event()
        self._fd: int | None = None

    def stop(self) -> None:
        self._stop_event.set()

    def run(self) -> None:
        buffer = ""

        try:
            self._fd = open_serial_port(self._port_path, self._baudrate)
            self._output_queue.put(("status", f"Connected: {self._port_path} @ {self._baudrate}"))

            while not self._stop_event.is_set():
                ready, _, _ = select.select([self._fd], [], [], 0.1)
                if not ready:
                    continue

                try:
                    chunk = os.read(self._fd, 4096)
                except OSError:
                    if self._stop_event.is_set():
                        break
                    raise

                if not chunk:
                    continue

                buffer += chunk.decode("utf-8", errors="ignore")

                while "\n" in buffer:
                    line, buffer = buffer.split("\n", 1)
                    message = parse_json_line(line)
                    if isinstance(message, MetaMessage):
                        self._output_queue.put(("meta", message))
                    elif isinstance(message, FrameMessage):
                        self._output_queue.put(("frame", message))
        except Exception as exc:
            self._output_queue.put(("error", str(exc)))
        finally:
            if self._fd is not None:
                try:
                    os.close(self._fd)
                except OSError:
                    pass
                self._fd = None

            self._output_queue.put(("disconnected", None))


class VisualizerApp:
    def __init__(self, root: tk.Tk, port: str | None, baudrate: int, window_seconds: float) -> None:
        self.root = root
        self.root.title("Model UART Visualizer")
        self.root.geometry("1200x720")
        self.root.protocol("WM_DELETE_WINDOW", self.on_close)

        self.window_seconds = window_seconds
        self.selected_mode = tk.StringVar(value="mode: unknown")
        self.status_text = tk.StringVar(value="Disconnected")
        self.port_var = tk.StringVar()
        self.baudrate_var = tk.StringVar(value=str(baudrate))

        self.message_queue: queue.Queue = queue.Queue()
        self.reader: SerialReader | None = None

        self.series_order: list[str] = []
        self.times: deque[float] = deque()
        self.values_by_series: dict[str, deque[float]] = {}
        self.lines = {}
        self.last_timestamp_s: float | None = None
        self.default_port = port

        self._build_ui()
        self._reset_plot()
        self._refresh_ports()

        if self.default_port:
            self.port_var.set(self.default_port)
            self.root.after(150, self.connect)

        self.root.after(PLOT_UPDATE_MS, self._poll_queue)

    def _build_ui(self) -> None:
        controls = ttk.Frame(self.root, padding=12)
        controls.pack(fill=tk.X)

        ttk.Label(controls, text="Port").pack(side=tk.LEFT)
        self.port_box = ttk.Combobox(controls, textvariable=self.port_var, width=36, state="normal")
        self.port_box.pack(side=tk.LEFT, padx=(8, 12))

        ttk.Button(controls, text="Refresh", command=self._refresh_ports).pack(side=tk.LEFT)

        ttk.Label(controls, text="Baud").pack(side=tk.LEFT, padx=(16, 0))
        self.baud_box = ttk.Combobox(
            controls,
            textvariable=self.baudrate_var,
            values=COMMON_BAUDRATES,
            width=10,
            state="normal",
        )
        self.baud_box.pack(side=tk.LEFT, padx=(8, 12))

        self.connect_button = ttk.Button(controls, text="Connect", command=self._toggle_connection)
        self.connect_button.pack(side=tk.LEFT)

        ttk.Label(controls, textvariable=self.selected_mode).pack(side=tk.RIGHT)

        status_bar = ttk.Frame(self.root, padding=(12, 0, 12, 8))
        status_bar.pack(fill=tk.X)
        ttk.Label(status_bar, textvariable=self.status_text).pack(side=tk.LEFT)

        self.figure = Figure(figsize=(12, 6), dpi=100)
        self.axes = self.figure.add_subplot(111)
        self.canvas = FigureCanvasTkAgg(self.figure, master=self.root)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True, padx=12, pady=(0, 12))

    def _reset_plot(self, x_label: str = DEFAULT_X_LABEL, y_label: str = DEFAULT_Y_LABEL) -> None:
        self.axes.clear()
        self.axes.set_xlim(-self.window_seconds, 0.0)
        self.axes.set_ylim(-0.05, 1.05)
        self.axes.set_xlabel(x_label)
        self.axes.set_ylabel(y_label)
        self.axes.grid(True, alpha=0.3)
        self.lines = {}

        for name in self.series_order:
            (line,) = self.axes.plot([], [], linewidth=1.8, label=name)
            self.lines[name] = line

        if self.series_order:
            self.axes.legend(loc="upper left")

        self.canvas.draw_idle()

    def _refresh_ports(self) -> None:
        ports = list_serial_ports()
        self.port_box["values"] = ports

        if self.port_var.get():
            return

        if self.default_port:
            self.port_var.set(self.default_port)
        elif ports:
            self.port_var.set(ports[0])

    def _toggle_connection(self) -> None:
        if self.reader is None:
            self.connect()
        else:
            self.disconnect()

    def connect(self) -> None:
        if self.reader is not None:
            return

        port = self.port_var.get().strip()
        if not port:
            messagebox.showerror("No port selected", "Select a serial port before connecting.")
            return

        try:
            baudrate = int(self.baudrate_var.get().strip())
        except ValueError:
            messagebox.showerror("Invalid baudrate", "Baudrate must be an integer.")
            return

        self.status_text.set(f"Connecting to {port} ...")
        self.reader = SerialReader(port_path=port, baudrate=baudrate, output_queue=self.message_queue)
        self.reader.start()
        self.connect_button.configure(text="Disconnect")

    def disconnect(self) -> None:
        if self.reader is None:
            return

        reader = self.reader
        self.reader = None
        reader.stop()
        reader.join(timeout=1.0)
        self.status_text.set("Disconnected")
        self.connect_button.configure(text="Connect")

    def _apply_meta(self, meta: MetaMessage) -> None:
        self.series_order = list(meta.series_names)
        self.times.clear()
        self.values_by_series = {name: deque() for name in self.series_order}
        self.last_timestamp_s = None
        self.selected_mode.set(f"mode: {meta.mode or 'unknown'}")
        self.status_text.set(f"Metadata received: {len(self.series_order)} series")
        self._reset_plot(x_label=meta.x_label, y_label=meta.y_label)

    def _ensure_series(self, series_names: list[str]) -> None:
        if not self.series_order:
            self.series_order = list(series_names)
            self.values_by_series = {name: deque() for name in self.series_order}
            self._reset_plot()
            return

        added = False
        current_len = len(self.times)

        for name in series_names:
            if name in self.values_by_series:
                continue

            history = deque()
            history.extend([math.nan] * current_len)
            self.values_by_series[name] = history
            self.series_order.append(name)
            added = True

        if added:
            self._reset_plot(self.axes.get_xlabel(), self.axes.get_ylabel())

    def _trim_history(self, newest_timestamp_s: float) -> None:
        while self.times and newest_timestamp_s - self.times[0] > self.window_seconds:
            self.times.popleft()
            for history in self.values_by_series.values():
                if history:
                    history.popleft()

    def _apply_frame(self, frame: FrameMessage) -> None:
        frame_entities = dict(frame.entities)

        if frame.values is not None:
            if not self.series_order:
                return

            for name, value in zip(self.series_order, frame.values):
                frame_entities[name] = value

        if not frame_entities:
            return

        self._ensure_series(list(frame_entities))

        self.last_timestamp_s = frame.timestamp_s
        self.times.append(frame.timestamp_s)

        for name in self.series_order:
            self.values_by_series[name].append(frame_entities.get(name, math.nan))

        self._trim_history(frame.timestamp_s)
        self.status_text.set(f"Streaming {len(self.series_order)} series")

    def _redraw(self) -> None:
        if not self.times:
            return

        latest_timestamp_s = self.last_timestamp_s if self.last_timestamp_s is not None else self.times[-1]
        x_values = [timestamp - latest_timestamp_s for timestamp in self.times]

        y_min = 0.0
        y_max = 1.0

        for name in self.series_order:
            line = self.lines.get(name)
            history = self.values_by_series.get(name)
            if line is None or history is None:
                continue

            y_values = list(history)
            line.set_data(x_values, y_values)

            finite_values = [value for value in y_values if math.isfinite(value)]
            if finite_values:
                y_min = min(y_min, min(finite_values))
                y_max = max(y_max, max(finite_values))

        padding = max(0.05, (y_max - y_min) * 0.1)
        self.axes.set_xlim(-self.window_seconds, 0.0)
        self.axes.set_ylim(y_min - padding, y_max + padding)
        self.canvas.draw_idle()

    def _poll_queue(self) -> None:
        needs_redraw = False

        while True:
            try:
                kind, payload = self.message_queue.get_nowait()
            except queue.Empty:
                break

            if kind == "meta":
                self._apply_meta(payload)
                needs_redraw = True
            elif kind == "frame":
                self._apply_frame(payload)
                needs_redraw = True
            elif kind == "status":
                self.status_text.set(payload)
            elif kind == "error":
                self.status_text.set(f"Error: {payload}")
            elif kind == "disconnected":
                self.reader = None
                self.connect_button.configure(text="Connect")

        if needs_redraw:
            self._redraw()

        self.root.after(PLOT_UPDATE_MS, self._poll_queue)

    def on_close(self) -> None:
        self.disconnect()
        self.root.destroy()


def run_web_visualizer(args: argparse.Namespace) -> int:
    script_dir = pathlib.Path(__file__).resolve().parent
    handler = functools.partial(http.server.SimpleHTTPRequestHandler, directory=str(script_dir))
    try:
        server = http.server.ThreadingHTTPServer(("127.0.0.1", args.web_port), handler)
    except OSError as exc:
        print(f"Failed to start local web server: {exc}", file=sys.stderr)
        print("Retry with another --web-port or run the script in an environment that allows localhost sockets.", file=sys.stderr)
        return 1

    query = urllib.parse.urlencode(
        {
            "baudrate": args.baudrate,
            "windowSeconds": args.window_seconds,
        }
    )
    url = f"http://127.0.0.1:{server.server_port}/serial_visualizer_web.html?{query}"

    if args.port:
        print("Note: --port is ignored in browser mode. Select the UART device in the page.", file=sys.stderr)

    if DESKTOP_GUI_AVAILABLE and args.force_web:
        print("Desktop GUI is available, but browser mode was requested explicitly.")
    elif DESKTOP_GUI_ERROR:
        print(f"Desktop GUI is unavailable: {DESKTOP_GUI_ERROR}")

    print("Serving browser visualizer on localhost.")
    print("Open the page in Chrome or Edge because Web Serial is required.")
    print(url)

    if not args.no_browser:
        try:
            webbrowser.open(url)
        except Exception:
            pass

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()

    return 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Real-time UART visualizer for wakeword/KWS model outputs.")
    parser.add_argument("--port", help="Serial device path, for example /dev/tty.usbmodem0010518634983")
    parser.add_argument("--baudrate", type=int, default=DEFAULT_BAUDRATE, help="UART baudrate")
    parser.add_argument("--window-seconds", type=float, default=WINDOW_SECONDS, help="Visible plot window")
    parser.add_argument("--force-web", action="store_true", help="Serve the browser UI even if desktop GUI is available")
    parser.add_argument("--web-port", type=int, default=8765, help="Local HTTP port for browser mode, use 0 for auto")
    parser.add_argument("--no-browser", action="store_true", help="Do not auto-open the browser page")
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    if args.force_web or not DESKTOP_GUI_AVAILABLE:
        return run_web_visualizer(args)

    if os.name != "posix":
        print("Desktop mode currently supports POSIX serial ports only. Use --force-web instead.", file=sys.stderr)
        return 1

    root = tk.Tk()
    app = VisualizerApp(
        root=root,
        port=args.port,
        baudrate=args.baudrate,
        window_seconds=args.window_seconds,
    )
    root.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
