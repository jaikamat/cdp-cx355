# Sony CDP-CX355 WiFi Remote Control

A WiFi-based remote control system for Sony CDP-CX355 disc changers using Arduino Uno R4 WiFi and the S-Link protocol. Transform your vintage Sony disc changer into a network-connected device with a web-based interface for disc management and playback control.

## What It Does

This project creates a bridge between your Sony CDP-CX355 disc changer and your home network, allowing you to:

- **Remote Control**: Play, stop, pause, skip tracks, and power on/off your disc changer from any device on your network
- **Disc Management**: Browse, edit, and organize disc titles/memos for up to 300 discs through a web interface
- **Direct Disc Selection**: Jump directly to any disc number (1-300) with a single click
- **Title Sync**: Write titles to the CDP-CX355's own memo memory over S-Link, and read titles back from the device
- **LED Matrix Display**: Visual feedback on the Arduino showing connection status and the device hostname

### User Experience

1. **Setup**: Connect the Arduino to your CDP-CX355's S-Link port and power on
2. **Connect**: The device joins your WiFi network and scrolls its hostname (`sony-remote.local`) on the LED matrix
3. **Control**: Open any web browser and navigate to `http://sony-remote.local`
4. **Manage**: Use the web interface to:
   - View your discs with custom titles/memos, 25 at a time
   - Click play buttons to directly select and play any disc
   - Edit disc titles and save them to the player and EEPROM
   - Control basic playback functions (play/stop/pause/next/previous)

## Architecture

### Hardware Components
- **Arduino Uno R4 WiFi**: Main controller with built-in WiFi and LED matrix
- **S-Link Connection**: Communicates with Sony CDP-CX355 using Sony's proprietary S-Link protocol (single wire, pin 8)
- **EEPROM Storage**: Persistent storage for disc titles/memos (up to 300 discs)

### Software Components
- **S-Link Protocol** (`SLinkProtocol`): Sony S-Link/Control-A1 communication with command queuing
- **Web Server / UI** (`main.cpp` + `WebUI.hpp`): HTTP server serving the single-page interface and JSON API
- **Disc Storage** (`DiscStorage`): EEPROM-based persistence for disc metadata
- **LED Matrix Controller** (`LedMatrixController`): Visual feedback using Arduino's built-in LED matrix
- **WiFi Manager** (`WiFiManager`): Network connection and mDNS hostname management
- **HTTP Parser** (`HttpParser`): Minimal HTTP request parsing

### Communication Flow
```
Web Browser → HTTP/JSON → Arduino Uno R4 WiFi → S-Link Protocol → CDP-CX355
```

## Getting Started

### Prerequisites

#### Hardware Required
- Arduino Uno R4 WiFi board
- Sony CDP-CX355 (or compatible CX-series) disc changer
- S-Link cable (or suitable connector to interface with the S-Link port)
- 220Ω resistor and diode for S-Link line protection

#### Software Required
- [PlatformIO](https://platformio.org/)

### Installation

1. **Clone the Repository**
   ```bash
   git clone <repository-url>
   cd 241219-214510-uno_r4_wifi
   ```

2. **Install PlatformIO**
   ```bash
   pip install platformio
   ```

3. **Configure WiFi Credentials**
   - Create `src/Secrets.hpp` (it is gitignored)
   ```cpp
   #define WIFI_SSID "your-network-name"
   #define WIFI_PASSWORD "your-password"
   ```

4. **Build and Upload**
   ```bash
   pio run --target upload
   ```

### Hardware Setup

1. **S-Link Connection**
   - Connect Arduino pin 8 to your CDP-CX355's S-Link port
   - Use a 220Ω resistor and diode for line protection
   - Ensure proper ground connection between devices

2. **Power Up**
   - Power on the Arduino (via USB or external power)
   - Power on your CDP-CX355
   - Watch the LED matrix for WiFi connection status

3. **Find Your Device**
   - The LED matrix scrolls the hostname (`sony-remote.local`) once connected
   - Navigate to `http://sony-remote.local` in your web browser
   - Alternative: check your router's DHCP client list for the IP address and use `http://[IP-ADDRESS]`

## Web Interface

The interface has three functional areas, protected by a global busy lock so commands can't be spammed while the slow S-Link bus is working.

### Transport Controls
- **Play / Stop / Pause / Next / Previous**: Basic playback control
- **Power**: Toggles CDP-CX355 power state

### Set Disc Title
A standalone form for titling any disc by number without paging to its range. Submits the same command as the per-disc Save button.

### Disc Collection
A progressive loading system keeps memory usage small:

- **Range Selector**: Pick a 25-disc range (e.g., "Discs 1-25") from the dropdown; only one range is loaded at a time
- **Per-Disc Controls**, for each disc in the loaded range:
  ```
  Disc 1: [Title Text Field] [Play] [Save] [Auto-Discover]
  ```
  - **Title Field**: Editable text input (13 character limit)
  - **Play**: Immediately selects and plays the disc
  - **Save**: Writes the title to the CDP-CX355's memo memory via S-Link, then reads it back and stores it in EEPROM
  - **Auto-Discover**: Queries the CDP-CX355 for the disc's title (memo or CD TEXT) and stores it in EEPROM

### UI Locking During Operations
- All buttons and inputs dim while an operation is in progress
- A status indicator shows the current operation (e.g., "Saving disc 5...")
- The UI re-enables automatically after the operation's expected duration (0.5-5s depending on the command)

## API Reference

### `GET /`
Returns the main HTML interface.

### `GET /discs?page=N&limit=M`
Paginated disc information.
- `page` (optional): page number, default 1
- `limit` (optional): items per page, default 25, max 50

```json
{
  "discs": [
    {"d": 1, "m": "Album Title"},
    {"d": 2, "m": "Another Album"}
  ],
  "page": 1,
  "limit": 25,
  "total": 300,
  "hasMore": true
}
```

### `POST /`
Command execution. Content-Type: `application/x-www-form-urlencoded`.

**Transport commands**:
- `command=play` / `stop` / `pause` / `next` / `prev` / `power`

**Disc operations**:
- `command=playDisc&disc=5` — select and play a disc
- `command=saveDiscTitle&disc=5&title=MyAlbum` — write the title to the CDP-CX355 and sync it back to EEPROM (`setDiscTitle` is accepted as an alias)
- `command=discoverTitle&disc=8` — read a disc's title from the CDP-CX355 into EEPROM

Example automation:
```bash
curl -X POST http://sony-remote.local/ -d "command=playDisc&disc=5"
curl "http://sony-remote.local/discs?page=1&limit=10"
```

## Storage

```cpp
struct DiscInfo {
    uint16_t discNumber;  // Disc number (1-300)
    char memo[16];        // Title/memo (13 chars used, null-terminated)
    bool isDataCD;        // Reserved; kept for record-layout stability
};
```

- **Record Size**: `sizeof(DiscInfo)` = 20 bytes (2 + 16 + 1, padded to 2-byte alignment)
- **Total**: 300 × 20 = 6,000 bytes of the Uno R4's 8KB emulated EEPROM (RA4M1 data flash)
- **Addressing**: sequential records from address 0; zero-indexed internally, 1-indexed in the UI
- **Robust Reads**: never-written records (erased flash) are normalized on read, so the UI shows clean empty slots

## S-Link Protocol Notes

This project implements Sony's proprietary S-Link (Control-A1) protocol, based on [Ircama's Sony_SLink library](https://github.com/Ircama/Sony_SLink) timings.

### Addressing and Encoding
```
0x90 - CDP unit, discs 1-200
0x93 - CDP unit, discs 201-300
Disc numbers are BCD encoded.
```

### Commands Used
- Transport: play `0x00`, stop `0x01`, pause `0x02`, next `0x08`, prev `0x09`
- Power: on `0x2E`, off `0x2F`
- Disc select: `0x50` (disc + track)
- Title query: `0x40` (responds with `98 40 ...` followed by ASCII title bytes)
- Memo write: `0x80` (disc + 13 title bytes, space padded)

### Timing and Retry Behavior
- The line runs at roughly 355 bps; every command takes visible time
- Transport commands are sent once (fire-and-forget) so a retry can't double-execute them
- Title queries and disc selection retry up to 5 times, with a short growing delay between send and listen (0.5ms increments) and 100ms between attempts
- Responses are captured with a 0.2s listening window
- Commands are queued and executed one at a time; the queue waits for the bus to be idle

### Serial Monitor
The serial monitor (115200 baud) logs every queued command, raw S-Link responses, parsed titles, and HTTP commands as they arrive — this is the main debugging tool.

### Troubleshooting
- **No response to commands**: confirm the CDP-CX355 is powered on, check the pin 8 connection and shared ground
- **Title discovery fails**: ensure a disc is actually loaded in that slot; watch the serial monitor for raw responses
- **Line protection**: a 220Ω series resistor and diode protect the single-wire bus

## Code Structure

- `src/main.cpp` — setup, command handlers, HTTP request routing
- `src/WebUI.hpp` — embedded web interface (HTML/JS) and `/discs` JSON endpoint
- `src/SLinkProtocol.cpp/.hpp` — S-Link protocol implementation and command queue
- `src/WifiManager.cpp/.hpp` — network connection and mDNS
- `src/HttpParser.cpp/.hpp` — HTTP request parsing
- `src/DiscStorage.hpp` — EEPROM-based disc metadata storage
- `src/LedMatrixController.hpp` — LED matrix display control
- `src/Secrets.hpp` — WiFi credentials (not in git)

### Building
```bash
pio run              # build
pio run --target upload
pio device monitor   # serial output at 115200 baud
```
Behavioral testing requires the actual hardware — the S-Link protocol can't be exercised without a connected CDP unit.

## Technical Specifications

- **Microcontroller**: Renesas RA4M1 (ARM Cortex-M4 @ 48MHz): 256KB flash, 32KB SRAM, 8KB data-flash EEPROM
- **WiFi**: ESP32-S3 module (802.11 b/g/n), HTTP on port 80, mDNS as `sony-remote.local`
- **S-Link**: single-wire bidirectional, 5V logic, ~355 bps
- **Disc capacity**: 300 (CDP-CX355 hardware limit); titles 13 characters

## Known Limitations

- S-Link is slow; saving or discovering a title takes several seconds
- No authentication and HTTP only — intended for trusted local networks
- Power state is assumed, not queried: if the player is toggled from its front panel, the web Power button's sense inverts until toggled again
- CD TEXT discs: the player's own CD TEXT overrides a written memo when the disc is selected

## Future Work

- Indicate in the UI whether a title is editable (memo) or read-only (CD TEXT)
- Load the first 25 discs automatically on page load
- Show a failure state on Auto-Discover when no disc or title is found
- Playback animations on the LED matrix

## License

[Add your preferred license here]

## Credits

- S-Link protocol implementation based on [Ircama's Sony_SLink library](https://github.com/Ircama/Sony_SLink)
- Arduino framework and WiFiS3 library for network connectivity
- PlatformIO for build system and dependency management
