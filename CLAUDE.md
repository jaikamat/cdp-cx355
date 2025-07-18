# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a PlatformIO project for Arduino Uno R4 WiFi that creates a WiFi-based remote control system for Sony CDP-CX355 disc changers. The project bridges vintage Sony equipment with modern web interfaces using the proprietary S-Link protocol.

## Architecture

The system consists of several key components:

### Core Components
- **main.cpp**: Main application with HTTP server, command routing, and web interface
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
- Web-based remote control with transport controls (play/stop/pause/next/prev)
- Disc title management with auto-discovery from CDP-CX355
- Progressive loading of disc collection (25 discs at a time)
- EEPROM storage for persistent disc titles
- mDNS hostname resolution (sony-remote.local)
- S-Link protocol implementation for Sony equipment communication

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
- **S-Link Pin**: Digital Pin 8 (configurable in main.cpp:18)
- **Network**: WiFi credentials defined in `src/Secrets.hpp`
- **Storage**: EEPROM for disc titles (5.7KB for 300 discs)

## Development Notes

### S-Link Protocol Implementation
The S-Link protocol operates at 355 bps with specific timing requirements. Key aspects:
- BCD encoding for disc numbers
- Device addressing (0x90 for primary unit, 0x93 for secondary)
- Command queuing system with callbacks
- Response parsing for disc title retrieval

### Web Interface
Single-page application with:
- Progressive disc loading to prevent memory overflow
- Real-time command execution via POST requests
- JSON API for disc information (`/discs` endpoint)
- Form-based title updates with bulk operations

### Storage Architecture
- **Record Size**: 19 bytes per disc (2 bytes disc number + 16 bytes memo + 1 byte flags)
- **Addressing**: Zero-indexed internally, 1-indexed for UI
- **Persistence**: EEPROM-based, survives power cycles

## Code Structure

### Key Files
- `src/main.cpp`: Main application logic, HTTP server, command handlers
- `src/SLinkProtocol.cpp/.hpp`: S-Link protocol implementation
- `src/WifiManager.cpp/.hpp`: Network management and mDNS
- `src/DiscStorage.hpp`: EEPROM-based disc metadata storage
- `src/HttpParser.cpp/.hpp`: HTTP request parsing
- `src/LedMatrixController.hpp`: LED matrix display control
- `src/Secrets.hpp`: WiFi credentials (not in git)

### Command System
Uses `std::map<String, CommandHandler>` for routing web commands to S-Link operations:
- Transport commands: play, stop, pause, next, prev, power
- Disc operations: bulkUpdate, discoverTitle, queryDiscMemory
- Special commands: setDiscTitle (PS/2 keyboard simulation)

## Testing and Debugging

### Serial Monitor
Comprehensive logging shows:
- S-Link communication details
- Command execution status
- Network connection states
- Error conditions and recovery

### Hardware Testing
- S-Link line state verification
- Device response validation
- Title discovery with retry logic
- Connection integrity checks

## Configuration

### WiFi Setup
Edit `src/Secrets.hpp`:
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

- S-Link protocol is inherently slow (355 bps)
- Limited to 300 discs (CDP-CX355 hardware limitation)
- Requires physical connection to Sony equipment
- No authentication (designed for local network use)
- HTTP only (no HTTPS)

## Future Development

See `src/TODO: Features.md` for planned enhancements including:
- PS/2 keyboard simulation for direct title writing
- Better CDText vs memo handling
- UI improvements for disc management
- Enhanced error handling and recovery