# Sesame Robot Micro

This repository contains the firmware and hardware files for a compact Sesame-style robot build.

## Project intent

This repo is primarily for experimentation and active development.

It is not intended to be a polished, beginner-friendly open-source fun project in the same way as the original Sesame Robot project. Expect rough edges, frequent iteration, and implementation-first decisions.

## Repository layout

- `hardware/CAD/`: 3D CAD source files (`.f3z`, `.step`)
- `hardware/STL/`: printable mesh exports
- `main-board-firmware/`: main robot control firmware (movement, faces, servo control)
- `wifi-bridge-firmware/`: ESP32-C6 captive-portal web controller and UART bridge

## Firmware architecture

### Main board firmware

`main-board-firmware/mini-firmware.ino` handles:

- servo movement sequences and poses
- OLED face rendering and animation
- command parsing over USB serial and SoftwareSerial

Main command categories include:

- directional motion: `forward`, `backward`, `left`, `right`, `stop`
- pose/animation commands: `rest`, `stand`, `wave`, `dance`, `swim`, `point`, `pushup`, `bow`, `cute`, `freaky`, `worm`, `shake`, `shrug`, `dead`, `crab`
- tuning commands: `frameDelay`, `walkCycles`, `motorDelay`, `subtrim`, per-servo angle commands

### Wi-Fi bridge firmware

`wifi-bridge-firmware/wifi-bridge.ino` runs on a Seeed XIAO ESP32-C6 and provides:

- a Wi-Fi access point (`Sesame`)
- captive-portal web UI
- HTTP endpoints for motion and settings
- UART forwarding of commands to the main board
- optional BLE gamepad support via Bluepad32 (when available)

## Wiring (bridge to main board)

- common ground between ESP32-C6 and main board
- ESP32-C6 TX (D3 / GPIO21 in this sketch) -> main board RX (pin 12 in this sketch)

Current bridge behavior is one-way command transmission (main board replies are not required).

## Build and flash notes

### Main board firmware

Open `main-board-firmware/mini-firmware.ino` in Arduino IDE and install required libraries:

- Servo
- SoftwareSerial
- Adafruit GFX Library
- Adafruit SSD1306

Then compile/upload for your target main-board MCU.

### Wi-Fi bridge firmware

Open `wifi-bridge-firmware/wifi-bridge.ino` in Arduino IDE and use:

- board package: Espressif `esp32` (3.x)
- board: Seeed XIAO ESP32-C6

Optional:

- Bluepad32 for BLE gamepad support

## Development status

This codebase is optimized for fast iteration on behavior, control feel, and hardware integration.

If you are looking for a fully documented, beginner-oriented, feature-frozen project, this repository is probably not that. If you want a practical base for hacking and experimenting, it is exactly that.

Check out the Sesame Robot Project for a well maintained and documented larger version of this project.

## License

Apache-2.0. See `LICENSE`.