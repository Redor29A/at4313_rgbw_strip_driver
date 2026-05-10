#!/usr/bin/env python3
"""Tkinter RGBW controller for the ATtiny4313 firmware."""

from __future__ import annotations

import colorsys
import json
import math
import time
import tkinter as tk
from dataclasses import asdict, dataclass
from pathlib import Path
from tkinter import messagebox, ttk

try:
    import serial
    from serial.tools import list_ports
except ImportError:  # pragma: no cover - shown in GUI at runtime
    serial = None
    list_ports = None


CONFIG_PATH = Path(__file__).with_name("rgbw_config.json")
FRAME_HEADER = 0xA5
CMD_RGBW = 0x01
CMD_RELEASE = 0x02


@dataclass
class AppConfig:
    port: str = "/dev/ttyUSB0"
    baud: int = 14400
    send_interval_ms: int = 50
    brightness: int = 100
    red: int = 255
    green: int = 0
    blue: int = 0
    white: int = 0
    effect: str = "Static"
    effect_speed: int = 50
    saturation: int = 255
    temperature: int = 127


def clamp_byte(value: float | int) -> int:
    return max(0, min(255, int(value)))


def load_config() -> AppConfig:
    if not CONFIG_PATH.exists():
        config = AppConfig()
        save_config(config)
        return config

    try:
        data = json.loads(CONFIG_PATH.read_text(encoding="utf-8"))
        defaults = asdict(AppConfig())
        defaults.update(data)
        return AppConfig(**defaults)
    except (OSError, json.JSONDecodeError, TypeError):
        return AppConfig()


def save_config(config: AppConfig) -> None:
    CONFIG_PATH.parent.mkdir(parents=True, exist_ok=True)
    CONFIG_PATH.write_text(
        json.dumps(asdict(config), indent=2, ensure_ascii=False),
        encoding="utf-8",
    )


def make_frame(command: int, *payload: int) -> bytes:
    data = [FRAME_HEADER, command, 0, 0, 0, 0, 0]
    for index, value in enumerate(payload[:5], start=2):
        data[index] = clamp_byte(value)

    checksum = 0
    for value in data:
        checksum ^= value
    data.append(checksum)
    return bytes(data)


class SerialLink:
    def __init__(self) -> None:
        self._port = None

    @property
    def connected(self) -> bool:
        return self._port is not None and self._port.is_open

    def connect(self, port: str, baud: int) -> None:
        if serial is None:
            raise RuntimeError("pyserial is not installed. Run: pip install pyserial")
        self.close()
        self._port = serial.Serial(port=port, baudrate=baud, timeout=0)

    def send_rgbw(self, r: int, g: int, b: int, w: int, brightness: int) -> None:
        if self.connected:
            self._port.write(make_frame(CMD_RGBW, r, g, b, w, brightness))

    def release(self) -> None:
        if self.connected:
            self._port.write(make_frame(CMD_RELEASE))

    def close(self) -> None:
        if self.connected:
            self._port.close()
        self._port = None


class RgbwControlApp(tk.Tk):
    EFFECTS = ("Static", "Rainbow", "Pulse", "White Temp")

    def __init__(self) -> None:
        super().__init__()
        self.title("RGBW Controller")
        self.resizable(False, False)

        self.config_data = load_config()
        self.serial_link = SerialLink()
        self.effect_phase = 0.0
        self.last_send_at = 0.0

        self.port_var = tk.StringVar(value=self.config_data.port)
        self.baud_var = tk.IntVar(value=self.config_data.baud)
        self.status_var = tk.StringVar(value="Disconnected")
        self.effect_var = tk.StringVar(value=self.config_data.effect)

        self.sliders: dict[str, tk.Scale] = {}
        self._build_ui()
        self.protocol("WM_DELETE_WINDOW", self.on_close)
        self.after(self.config_data.send_interval_ms, self.tick)

    def _build_ui(self) -> None:
        root = ttk.Frame(self, padding=10)
        root.grid(row=0, column=0, sticky="nsew")

        connection = ttk.LabelFrame(root, text="Connection", padding=8)
        connection.grid(row=0, column=0, sticky="ew")

        ports = self.available_ports()
        self.port_box = ttk.Combobox(connection, textvariable=self.port_var, values=ports, width=18)
        self.port_box.grid(row=0, column=0, padx=(0, 6))
        ttk.Entry(connection, textvariable=self.baud_var, width=8).grid(row=0, column=1, padx=(0, 6))
        ttk.Button(connection, text="Connect", command=self.connect).grid(row=0, column=2, padx=(0, 6))
        ttk.Button(connection, text="Release", command=self.release).grid(row=0, column=3)
        ttk.Label(connection, textvariable=self.status_var).grid(row=1, column=0, columnspan=4, sticky="w")

        controls = ttk.LabelFrame(root, text="Color", padding=8)
        controls.grid(row=1, column=0, sticky="ew", pady=(8, 0))

        self.add_slider(controls, "Brightness", "brightness", self.config_data.brightness, 0)
        self.add_slider(controls, "R", "red", self.config_data.red, 1)
        self.add_slider(controls, "G", "green", self.config_data.green, 2)
        self.add_slider(controls, "B", "blue", self.config_data.blue, 3)
        self.add_slider(controls, "W", "white", self.config_data.white, 4)

        effects = ttk.LabelFrame(root, text="Effect", padding=8)
        effects.grid(row=2, column=0, sticky="ew", pady=(8, 0))
        ttk.Combobox(effects, textvariable=self.effect_var, values=self.EFFECTS, state="readonly", width=18).grid(
            row=0, column=0, columnspan=2, sticky="ew", pady=(0, 6)
        )
        self.add_slider(effects, "Speed", "effect_speed", self.config_data.effect_speed, 1)
        self.add_slider(effects, "Saturation", "saturation", self.config_data.saturation, 2)
        self.add_slider(effects, "Temperature", "temperature", self.config_data.temperature, 3)

        buttons = ttk.Frame(root)
        buttons.grid(row=3, column=0, sticky="ew", pady=(8, 0))
        ttk.Button(buttons, text="Send Now", command=self.send_current).grid(row=0, column=0, padx=(0, 6))
        ttk.Button(buttons, text="Save Config", command=self.save_current_config).grid(row=0, column=1)

    def add_slider(self, parent: ttk.Frame, label: str, key: str, value: int, row: int) -> None:
        ttk.Label(parent, text=label, width=12).grid(row=row, column=0, sticky="w")
        slider = tk.Scale(parent, from_=0, to=255 if key != "brightness" else 100, orient=tk.HORIZONTAL, length=280)
        slider.set(value)
        slider.grid(row=row, column=1, sticky="ew")
        self.sliders[key] = slider

    def available_ports(self) -> list[str]:
        if list_ports is None:
            return []
        return [port.device for port in list_ports.comports()]

    def connect(self) -> None:
        try:
            self.serial_link.connect(self.port_var.get(), int(self.baud_var.get()))
            self.status_var.set(f"Connected: {self.port_var.get()}")
            self.save_current_config(show_message=False)
            self.send_current()
        except Exception as exc:  # noqa: BLE001 - GUI should show the actual connection error
            messagebox.showerror("Connection error", str(exc))
            self.status_var.set("Disconnected")

    def release(self) -> None:
        self.serial_link.release()
        self.status_var.set("Released UART control")

    def slider(self, key: str) -> int:
        return int(self.sliders[key].get())

    def current_rgbw(self) -> tuple[int, int, int, int]:
        effect = self.effect_var.get()
        if effect == "Rainbow":
            hue = self.effect_phase % 1.0
            sat = self.slider("saturation") / 255.0
            r, g, b = colorsys.hsv_to_rgb(hue, sat, 1.0)
            return clamp_byte(r * 254), clamp_byte(g * 254), clamp_byte(b * 254), clamp_byte((1.0 - sat) * 180)

        if effect == "Pulse":
            pulse = (math.sin(self.effect_phase * math.tau) + 1.0) * 0.5
            return (
                clamp_byte(self.slider("red") * pulse),
                clamp_byte(self.slider("green") * pulse),
                clamp_byte(self.slider("blue") * pulse),
                clamp_byte(self.slider("white") * pulse),
            )

        if effect == "White Temp":
            temp = self.slider("temperature")
            warm = 255 - temp
            return (
                clamp_byte(255 - temp * 0.45),
                clamp_byte(80 + temp * 0.5),
                clamp_byte(temp),
                clamp_byte(160 + warm * 0.35),
            )

        return self.slider("red"), self.slider("green"), self.slider("blue"), self.slider("white")

    def send_current(self) -> None:
        r, g, b, w = self.current_rgbw()
        self.serial_link.send_rgbw(r, g, b, w, self.slider("brightness"))
        self.last_send_at = time.monotonic()

    def tick(self) -> None:
        speed = max(1, self.slider("effect_speed"))
        self.effect_phase = (self.effect_phase + speed / 12000.0) % 1.0

        if self.serial_link.connected:
            self.send_current()

        self.after(self.config_data.send_interval_ms, self.tick)

    def save_current_config(self, show_message: bool = True) -> None:
        self.config_data = AppConfig(
            port=self.port_var.get(),
            baud=int(self.baud_var.get()),
            send_interval_ms=self.config_data.send_interval_ms,
            brightness=self.slider("brightness"),
            red=self.slider("red"),
            green=self.slider("green"),
            blue=self.slider("blue"),
            white=self.slider("white"),
            effect=self.effect_var.get(),
            effect_speed=self.slider("effect_speed"),
            saturation=self.slider("saturation"),
            temperature=self.slider("temperature"),
        )
        save_config(self.config_data)
        if show_message:
            messagebox.showinfo("Config", f"Saved to {CONFIG_PATH}")

    def on_close(self) -> None:
        self.save_current_config(show_message=False)
        self.serial_link.close()
        self.destroy()


if __name__ == "__main__":
    app = RgbwControlApp()
    app.mainloop()
