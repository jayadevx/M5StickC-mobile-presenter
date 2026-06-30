# M5StickC Mobile Presenter

A Bluetooth Low Energy (BLE) presentation controller for M5StickC that turns your device into a wireless keyboard for controlling slideshows.

## Features

- **Wireless Control**: Navigate presentations using BLE HID keyboard protocol
- **Simple Button Mapping**:
  - M5 Button (BtnA) → Next slide (RIGHT ARROW)
  - Side Button (BtnB) → Previous slide (LEFT ARROW)
  - Power Button → Exit slideshow (ESCAPE)
- **Visual Feedback**: On-screen status display and LED indicators
- **Battery Monitor**: Real-time battery voltage and percentage display
- **Auto-Reconnect**: Just-works bonding for automatic reconnection

## Hardware Requirements

- **M5StickC** (original model with ESP32-PICO-D4 and AXP192 PMIC)

## Software Dependencies

Both libraries are available through the Arduino Library Manager:

1. **M5StickC** by M5Stack
2. **NimBLE-Arduino** by h2zero (search for "NimBLE")

This project uses NimBLE-Arduino 2.x (current Library Manager release).

## Installation

### 1. Install Arduino IDE
Download and install the [Arduino IDE](https://www.arduino.cc/en/software).

### 2. Add ESP32 Board Support
1. Open Arduino IDE
2. Go to **File → Preferences**
3. Add this URL to "Additional Boards Manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Go to **Tools → Board → Boards Manager**
5. Search for "esp32" and install "esp32 by Espressif Systems"

### 3. Install Required Libraries
1. Go to **Sketch → Include Library → Manage Libraries**
2. Search and install:
   - "M5StickC" by M5Stack
   - "NimBLE-Arduino" by h2zero

### 4. Configure Board Settings
1. Connect your M5StickC via USB
2. Select **Tools → Board → M5Stick-C**
3. **Important**: Set **Tools → Partition Scheme → Huge APP (3MB No OTA)**
4. Set **Tools → Upload Speed → 1500000**
5. Select the correct **Port**

### 5. Upload the Sketch
1. Open `M5StickC_Presenter_NimBLE_v20260628-1230.ino`
2. Click the Upload button

## Usage

### First Time Setup

1. **Power on** the M5StickC
2. The screen will display "WAITING... Pair on host"
3. On your computer/tablet:
   - Open Bluetooth settings
   - Look for a device named **"M5 Presenter"**
   - Click to pair (no PIN required)
4. Once connected, the screen will show "CONNECTED"

### During Presentation

- **M5 Button (front)**: Advance to next slide
- **Side Button**: Go back to previous slide
- **Power Button (short press)**: Exit presentation mode

The screen will briefly flash the action taken, and the built-in red LED will blink on each keypress.

### Battery Status

The battery voltage and percentage are displayed at the bottom of the screen.

## Technical Details

### Button Mapping

| Button | GPIO | Key Sent | Action |
|--------|------|----------|--------|
| M5 (BtnA) | 37 | RIGHT ARROW (0x4F) | Next slide |
| BtnB | 39 | LEFT ARROW (0x50) | Previous slide |
| Power (AXP192 PEK) | - | ESCAPE (0x29) | Exit slideshow |

### Debounce

All buttons use a 120ms debounce delay to prevent accidental double-presses.

### BLE Protocol

- **Device Name**: M5 Presenter
- **Appearance**: HID Keyboard (0x03C1)
- **Report Format**: Standard 101-key boot keyboard protocol
- **Security**: Just-works bonding with secure connections

### Compatibility

Works with any device that supports BLE HID keyboards:
- Windows 10/11
- macOS
- Linux
- iOS
- Android
- Chrome OS

## Troubleshooting

### Upload Fails / Flash Overflow
Make sure you've selected **Huge APP (3MB No OTA)** in the Partition Scheme setting.

### Device Won't Pair
1. Reset the M5StickC (press and hold power button)
2. Remove "M5 Presenter" from your host's Bluetooth device list
3. Re-pair from scratch

### Keys Not Working
1. Ensure the screen shows "CONNECTED"
2. Check that your presentation software is in focus
3. Try disconnecting and reconnecting

### Version Compatibility
This code targets NimBLE-Arduino 2.x. If you're using the older 1.x version, some method names differ (see comments in source code).

## License

This project is open source and available under the MIT License.

## Author

Version: v20260628-1230

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.
