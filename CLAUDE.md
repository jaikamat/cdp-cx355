# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a PlatformIO project for Arduino Uno R4 WiFi that creates a WiFi-based remote control system for Sony CDP-CX355 disc changers. The project bridges vintage Sony equipment with modern web interfaces using the proprietary S-Link protocol.

## Architecture

The system consists of several key components:

### Core Components
- **main.cpp**: Application setup, command routing, and HTTP request handling
- **WebUI.hpp**: Embedded single-page web interface (HTML/JS) and `/discs` JSON endpoint
- **SLinkProtocol**: Sony S-Link/Control-A1 protocol implementation for communicating with CDP-CX355
- **WiFiManager**: Network connection management and mDNS hostname resolution
- **DiscStorage**: EEPROM-based persistence for disc titles (up to 300 discs)
- **LedMatrixController**: Visual feedback using Arduino's built-in LED matrix
- **HttpParser**: HTTP request parsing for web server functionality

### Communication Flow
```
Web Browser → HTTP/JSON → Arduino Uno R4 WiFi → S-Link Protocol → Sony CDP-CX355
```

### Key Features
- Web-based remote control with transport controls (play/stop/pause/next/prev/power on/power off)
- Disc title management: write titles to the player's memo memory and read them back
- Progressive loading of disc collection (25 discs at a time)
- EEPROM storage for persistent disc titles
- mDNS hostname resolution (sony-remote.local)
- Global UI lock preventing command spam while the slow S-Link bus is busy

## Build Commands

### Build and Upload
```bash
pio run --target upload
```

### Build Only
```bash
pio run
```

### Monitor Serial Output
```bash
pio device monitor
```

## Hardware Configuration

- **Platform**: Arduino Uno R4 WiFi (Renesas RA4M1 + ESP32-S3)
- **S-Link Pin**: Digital Pin 8 (configurable in main.cpp)
- **Network**: WiFi credentials defined in `src/Secrets.hpp`
- **Storage**: 8KB emulated EEPROM (RA4M1 data flash); disc records use 6,000 bytes

## Development Notes

### S-Link Protocol Implementation
The S-Link protocol operates at ~355 bps with specific timing requirements. Key aspects:
- BCD encoding for disc numbers
- Device addressing (0x90 for discs 1-200, 0x93 for discs 201-300)
- Command queuing system with callbacks; one command on the bus at a time
- Transport commands are sent once (no retry) so they can't double-execute
- Title queries retry up to 5 times; responses parsed from `98 40 ...` frames
- `setDiscTitle` (memo write, command 0x80) is deliberately synchronous — it runs in the HTTP handler before the queue is next serviced, so a queued follow-up `getDiscTitle` always executes after it

### Web Interface
Single-page application in `src/WebUI.hpp` with:
- Progressive disc loading to prevent memory overflow
- Real-time command execution via POST requests
- JSON API for disc information (`/discs` endpoint)
- Global busy lock: all controls disable while a command is in flight
- Device constants (MAX_DISCS, PAGE_SIZE) injected into the page JS so the UI stays in sync with DiscStorage

### Storage Architecture
- **Record Size**: `sizeof(DiscInfo)` = 20 bytes (2-byte disc number + 16-byte memo + 1-byte reserved flag + 1 byte padding)
- **Total**: 300 × 20 = 6,000 bytes
- **Addressing**: Zero-indexed internally, 1-indexed for UI
- **Robust reads**: `readDisc` normalizes never-written (erased-flash) records and blanks memos containing non-printable bytes
- **Layout stability**: do not remove or reorder `DiscInfo` fields — it would shift every EEPROM address and scramble stored titles on deployed devices

## Code Structure

### Key Files
- `src/main.cpp`: Setup, command handlers, HTTP request routing
- `src/WebUI.hpp`: Embedded web interface and JSON endpoint
- `src/SLinkProtocol.cpp/.hpp`: S-Link protocol implementation
- `src/WifiManager.cpp/.hpp`: Network management and mDNS
- `src/DiscStorage.hpp`: EEPROM-based disc metadata storage
- `src/HttpParser.cpp/.hpp`: HTTP request parsing
- `src/LedMatrixController.hpp`: LED matrix display control
- `src/Secrets.hpp`: WiFi credentials (not in git)

### Command System
Uses `std::map<String, CommandHandler>` for routing web commands to S-Link operations:
- Transport commands: `play`, `stop`, `pause`, `next`, `prev`, `powerOn`, `powerOff`
- Disc operations: `playDisc`, `saveDiscTitle`, `discoverTitle`
- `setDiscTitle` is an alias of `saveDiscTitle` (kept for older clients)

Form parameters are extracted with `getFormParam()` in main.cpp, which anchors key matches at segment boundaries and URL-decodes values.

## Testing and Debugging

There are no automated tests; behavioral verification requires the physical hardware. The serial monitor (115200 baud) is the primary debugging tool and logs:
- S-Link command queuing and raw responses
- Parsed disc titles
- Incoming HTTP commands
- Network connection states

## Configuration

### WiFi Setup
Create `src/Secrets.hpp` (gitignored):
```cpp
#define WIFI_SSID "your-network-name"
#define WIFI_PASSWORD "your-password"
```

### S-Link Pin Configuration
Modify in `src/main.cpp`:
```cpp
SLinkProtocol slink(8); // Change pin number as needed
```

## Known Limitations

- S-Link protocol is inherently slow (~355 bps)
- Limited to 300 discs (CDP-CX355 hardware limitation)
- Requires physical connection to Sony equipment
- No authentication (designed for local network use)
- HTTP only (no HTTPS)

## Future Development

See the "Future Work" section in README.md for planned enhancements.
