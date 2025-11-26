# ESP32 Ethernet‑to‑Serial Bridge (TCP ↔ UART2)

A minimal ESP32 firmware that bridges a TCP socket to a hardware UART. It exposes a password‑protected web UI to configure network and serial settings, which are persisted in EEPROM.

- Transport: Ethernet (ESP32 native EMAC + external PHY; `ETH.h`)
- TCP server: listens for a single client and forwards bytes both ways to UART2
- Web server: HTTP on port 80 for configuration (Basic Auth)
- Persistent settings in EEPROM with a hardware “factory reset” pin

## Features

- Static Ethernet IP configuration (IP, Gateway, Subnet)
- TCP port and UART baudrate configuration
- HTTP Basic authentication (username/password)
- Settings saved to EEPROM
- One‑click reboot from the web UI
- Factory reset via a GPIO on boot

## Defaults

- Device IP: 192.168.1.19
- Gateway: 192.168.1.1
- Subnet: 255.255.255.0
- TCP port: 23
- UART2: 9600 8N1
- Web UI credentials: username `admin`, password `12345`
- Serial console (USB): 115200 baud
- Factory reset pin: `GPIO2` with internal pull‑up (pull LOW at boot to write defaults)

## Hardware

- Target: ESP32 with native Ethernet MAC + RMII PHY (e.g., LAN8720). Pinout/PHY options depend on your board.
- UART2 wiring:
  - RX2: `GPIO5`
  - TX2: `GPIO17`
  - Voltage levels: 3.3V TTL
  - Flow control: disabled

Note: This sketch calls `ETH.begin()` with board defaults. If your board requires explicit PHY parameters, adjust `ETH.begin(...)` and/or `ETH.config(...)` accordingly.

## How it works

- TCP server (`WiFiServer`) is started on the configured port and accepts a client from the Ethernet interface.
- Bytes from the TCP client are written to `Serial2`, and bytes from `Serial2` are echoed back to the client.
- A web UI at `/` lets you update:
  - IP Address, Gateway, Subnet
  - TCP Port
  - UART Baudrate
  - Web Username/Password
- Endpoints:
  - `/` configuration page (auth required)
  - `/submit` saves settings to EEPROM (auth required)
  - `/reset` reboots the device (auth required)
  - `/login` simple auth test (auth required)

## Applying changes

- Network (IP/GW/Subnet) and TCP port changes are persisted to EEPROM and applied on next reboot.
- Baudrate changes are persisted and applied on next reboot.
- Use the “Reset device” button in the UI or power‑cycle to apply changes.

## Build and flash

1. Install Arduino IDE (or PlatformIO) with the ESP32 core by Espressif.
2. Open `Ethernet_to_serial/Ethernet_to_serial.ino`.
3. Select an ESP32 board with Ethernet support (configure PHY as needed).
4. Compile and upload.
5. Connect the device to your Ethernet network.
6. Set your PC to the same subnet (192.168.1.0/24) and browse to `http://192.168.1.19/` (auth: `admin` / `12345`).

## Usage

- Configure settings in the web UI and reboot if required.
- Connect from a host to `tcp <device-ip> <port>` (default `23`) using `telnet`/`nc`/your app.
- All bytes sent to the TCP socket are forwarded to UART2 and vice‑versa.

Example:
- `telnet 192.168.1.19 23`

## Factory reset

- Hold `GPIO2` LOW during boot to write all default settings to EEPROM.
- You can also press “Reset device” in the web UI to reboot (does not rewrite defaults).

## Security notes

- HTTP is clear‑text; credentials are sent using Basic Auth without TLS.
- Place the device on a trusted network and change the default credentials.

## Troubleshooting

- No web UI? Ensure your PC is on 192.168.1.x/24 and the cable/link is up.
- Can’t reach `192.168.1.19`? Use the Serial Monitor (115200) to read logs and confirm the configured IP.
- If your board needs custom PHY settings, adjust `ETH.begin(...)` and `ETH.config(...)` for your hardware.

## Repository structure

- `Ethernet_to_serial/Ethernet_to_serial.ino` — main firmware
- EEPROM layout (offsets used in code):
  - `EEPROM_DEFAULT` (0)
  - `EEP_IP_ADDRESS` (1), `EEP_IP_GATEWAY` (5), `EEP_SUBNET` (9)
  - `EEP_MAC` (13; not currently written)
  - `EEP_PORT` (19)
  - `EEP_BOUDRATE` (21)
  - `EEP_USERNAME` (25), `EEP_PASSWORD` (41)

---
Made with GitHub Copilot to be simple, robust, and easy to adapt for your hardware.