# Hey Man, Nice Shot...Timer
### A Feature-Rich Shooting Sport Timer for the M5StickC Plus2

A versatile shot timer for live fire and dry fire practice, built on the M5StickC Plus2 (ESP32). Detects shots via microphone and/or IMU, records split times, supports Bluetooth audio output, and can be fully configured over WiFi from a phone or browser.

---

## Hardware

| Component | Details |
|-----------|---------|
| **M5StickC Plus2** | Core device — [M5Stack Store](https://shop.m5stack.com/products/m5stickc-plus2-esp32-pico-d4) |
| **External Buzzer** | Optional but recommended. Active buzzer on GPIO 25/GND and GPIO 2/GND for louder local audio |
| **Bluetooth Speaker/Headset** | Optional. Any A2DP-compatible device for wireless audio prompts |

---

## Flashing a Pre-Built Binary

### Browser (Any OS)
1. Download the latest `.bin` from [Releases](https://github.com/jcarletto27/HeyManNiceShotTimer/releases)
2. Open the [Espressif Web Programmer](https://espressif.github.io/esptool-js/)
3. Click **Connect** and select the device COM port
4. Set Flash Address to `0x0`, select the `.bin` file
5. Click **Program** — this may take up to 5 minutes

### Windows (Command Line)
```powershell
# In a new folder (e.g. C:\Users\%USERNAME%\Documents\HeyManNiceShot):
python -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install --upgrade pip esptool

# Find your device COM port
Get-PnpDevice -Class 'Ports' | Where-Object { $_.Present } | Select-Object Name, Present, Status

# Flash (replace COM6 with your port)
esptool -p COM6 write_flash 0x0 HMNS_T.bin
```

---

## Operating Modes

### Live Fire
Standard shot timer using microphone detection. Press the front button to arm — after a configurable random delay, a beep sounds and the timer starts. Records elapsed time and splits for each detected shot.

### Dry Fire Par
Audio-only mode — no microphone needed. A random delay is followed by a programmable sequence of beeps at user-defined intervals (par times). Useful for practicing draws and presentations to a par time.

### Noisy Range
Combines microphone detection with IMU (accelerometer) recoil detection. A shot is only registered when *both* a sound peak and a subsequent recoil impulse are detected within a 100ms window. Greatly reduces false positives in loud environments.

---

## Features

- **Split timing** — records and displays each shot's elapsed time and split from previous
- **Configurable random start delay** — prevents anticipation of the start beep
- **Adjustable shot limit** — cap shots at 1–99, or set to 0 for unlimited
- **Bluetooth A2DP** — stream start beeps and par tones to any Bluetooth speaker/headset
- **BT audio offset calibration** — compensate for Bluetooth latency to keep buzzer and speaker in sync
- **WiFi settings page** — configure all settings and upload new firmware from a phone browser
- **Screen rotation** — four orientations supported
- **Boot animation** — optional JPG image sequence on startup
- **Auto-sleep** — light sleep after 1 minute of inactivity (disabled when Bluetooth is connected)
- **Device status screen** — battery voltage/%, IMU readings, LittleFS usage
- **Dual-core FreeRTOS** — buzzer runs on Core 0 to avoid blocking display/logic on Core 1

---

## Settings Menu Structure

```
SETTINGS
├── Live Fire
│   ├── Max Shots          (0 = unlimited, 1–99)
│   ├── Shot Threshold     (mic sensitivity, RMS)
│   ├── Min 1st Shot       (ms delay before 1st shot registers)
│   ├── Start Delay Min    (ms, minimum random pre-beep delay)
│   ├── Start Delay Max    (ms, maximum random pre-beep delay)
│   └── Calibrate Thresh.
├── Dry Fire
│   ├── Par Beep Count     (1–10)
│   └── Par Time 1–10      (0.1–10.0 seconds each)
├── Noisy Range
│   ├── Recoil Threshold   (G-force, 0.5–5.0)
│   └── Calibrate Recoil
├── Beep Settings
│   ├── Beep Duration      (ms)
│   ├── Beep Tone          (Hz)
│   ├── Post Beep Delay    (ms)
│   └── Tone Sweep
├── Bluetooth
│   ├── Connect / Disconnect
│   ├── Volume             (0–127)
│   ├── BT Audio Offset    (ms, -1000 to +500)
│   ├── Auto Reconnect
│   └── Scan for Devices
├── Device
│   ├── Screen Rotation    (0–3)
│   ├── Boot Animation     (On/Off)
│   ├── Auto Sleep         (On/Off)
│   ├── Device Status
│   ├── List Files
│   └── WiFi Settings      (opens WiFi AP for browser config / OTA)
├── Power Off Now
└── Save & Exit
```

---

## WiFi Settings & OTA Firmware Updates

The device can host a WiFi access point for browser-based configuration and over-the-air firmware updates.

1. Navigate to **Settings → Device → WiFi Settings**
2. Connect to the WiFi network:
   - **SSID:** `ShotTimer`
   - **Password:** `shottimer`
3. Open a browser and go to `http://192.168.4.1`
4. The settings page mirrors the on-device menu — edit any setting and tap **Save Settings**
5. To update firmware, scroll to the **Firmware Update** section, choose a `.bin` file, and tap **Upload Firmware**. The device reboots automatically on success.

> WiFi is **only active** while on the WiFi Settings screen. It shuts off when you exit to preserve battery life.

---

## Building from Source

### Required Libraries

Install these **exact versions** — other versions may not be compatible:

| Library | Version | Source |
|---------|---------|--------|
| `m5stack:esp32` board | `2.1.4` | Arduino Board Manager |
| `esp32:esp32` board | `3.3.2` | Arduino Board Manager |
| `M5GFX` | `0.2.7` | Arduino Library Manager |
| `M5Unified` | `0.2.5` | Arduino Library Manager |
| `ESP32-A2DP` | `1.8.7` | Arduino Library Manager |
| `ESP32-BT-Scanner` | latest | [GitHub](https://github.com/jcarletto27/ESP32-BT-Scanner) — install manually |

> **ESP32-BT-Scanner** is not in the Library Manager. Download the ZIP from the link above and install via **Sketch → Include Library → Add .ZIP Library**.

### Arduino IDE

1. **Install Arduino IDE 2.x** from [arduino.cc](https://www.arduino.cc/en/software)

2. **Add board manager URLs** — go to *File → Preferences* and add both URLs to "Additional boards manager URLs":
   ```
   https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```

3. **Install board packages** via *Tools → Board → Boards Manager*:
   - Search `m5stack` → install **M5Stack** version `2.1.4`
   - Search `esp32` → install **esp32 by Espressif Systems** version `3.3.2`

4. **Install libraries** via *Tools → Manage Libraries*:
   - `M5GFX` version `0.2.7`
   - `M5Unified` version `0.2.5`
   - `ESP32-A2DP` version `1.8.7`

5. **Install ESP32-BT-Scanner manually** — download the ZIP from [GitHub](https://github.com/jcarletto27/ESP32-BT-Scanner) and install via *Sketch → Include Library → Add .ZIP Library*

6. **Select the board** — *Tools → Board → M5Stack → M5StickC-Plus2*

7. **Select your port** — *Tools → Port* → choose the COM port for your device

8. **Open the sketch** — open `code/code.ino`

9. **Upload** — click the Upload button (or *Sketch → Upload*)

> **Note:** To upload the boot animation filesystem, install the [arduino-littlefs-upload plugin](https://github.com/earlephilhower/arduino-littlefs-upload) and use *Tools → Upload LittleFS to Pico/ESP8266/ESP32*.

### arduino-cli / Makefile

This project uses [arduino-cli](https://arduino.github.io/arduino-cli/). Install it, then install the board packages above.

### Makefile Targets

Run these from inside the `code/` directory:

| Target | Description |
|--------|-------------|
| `make` or `make build` | Compile only |
| `make flash` | Compile and upload firmware |
| `make filesystem.bin` | Generate LittleFS image from `data/` |
| `make flash-fs` | Upload LittleFS filesystem image |
| `make merge` | Create a single merged `.bin` (bootloader + partitions + firmware + FS) |
| `make release` | Clean build + merged binary for distribution |
| `make monitor` | Open serial monitor at 115200 baud |
| `make all-flash` | Build + flash + upload FS + monitor |
| `make clean` | Remove build artifacts |

> Edit the `PORT` variable at the top of the Makefile to match your device's serial port (e.g. `/dev/cu.usbserial-XXXX` on macOS, `COM6` on Windows).

### Project Structure

All `.h` and `.cpp` files must be in the same directory as `code.ino`:

```
code/
├── code.ino
├── config.h / config.cpp
├── globals.h
├── timer_modes.h / timer_modes.cpp
├── input_handler.h / input_handler.cpp
├── display_utils.h / display_utils.cpp
├── audio_utils.h / audio_utils.cpp
├── bluetooth_utils.h / bluetooth_utils.cpp
├── nvs_utils.h / nvs_utils.cpp
├── ota_utils.h / ota_utils.cpp
├── system_utils.h / system_utils.cpp
└── data/                  ← optional boot animation JPGs (1.jpg, 2.jpg, ...)
```

### Boot Animation (Optional)

Place sequentially numbered JPG files (`1.jpg`, `2.jpg`, ...) in the `data/` folder, then run `make filesystem.bin && make flash-fs` to upload them. Enable via **Settings → Device → Boot Animation**.

---

## Controls

| Button | Action |
|--------|--------|
| **BtnA (front)** — short press | Select / Confirm / Start |
| **BtnA (front)** — long press | Back / Cancel |
| **BtnB (side)** | Scroll down / Adjust value down |
| **BtnPWR (top)** | Scroll up / Adjust value up |
| **BtnA during boot animation** | Skip animation |

---

## Hardware Gallery

![Attached to Blue Gun](https://github.com/jcarletto27/HeyManNiceShotTimer/blob/main/images/PXL_20250506_164010292.MP.jpg?raw=true)
![Mounted in Lanyard Mode](https://github.com/jcarletto27/HeyManNiceShotTimer/blob/main/images/PXL_20250506_164025303.MP.jpg?raw=true)
![Closeup of Mounting Mechanism](https://github.com/jcarletto27/HeyManNiceShotTimer/blob/main/images/PXL_20250506_164028005.jpg?raw=true)

---

## Contributing

Pull requests are welcome. Please target the `dev` branch.
