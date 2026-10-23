# Sony CDP-CX355 WiFi Remote Control

A WiFi-based remote control system for Sony CDP-CX355 disc changers using Arduino Uno R4 WiFi and the S-Link protocol. Transform your vintage Sony disc changer into a network-connected device with a web-based interface for disc management and playback control.

## What It Does

This project creates a bridge between your Sony CDP-CX355 disc changer and your home network, allowing you to:

- **Remote Control**: Play, stop, pause, skip tracks, and power on/off your disc changer from any device on your network
- **Disc Management**: Browse, edit, and organize disc titles/memos for up to 300 discs through a web interface
- **Direct Disc Selection**: Jump directly to any disc number (1-300) with a single click
- **Title Sync**: Write titles to the CDP-CX355's own memo memory over S-Link, and read titles back from the device
- **LED Matrix Display**: Visual feedback on the Arduino showing connection status and the device hostname
- **Now Playing**: A banner at the bottom of the page shows the disc, its title, and the track whenever the player reports it is playing or paused

### User Experience

1. **Setup**: Connect the Arduino to your CDP-CX355's S-Link port and power on
2. **Connect**: The device joins your WiFi network and scrolls its hostname (`sony-remote.local`) on the LED matrix
3. **Control**: Open any web browser and navigate to `http://sony-remote.local`
4. **Manage**: Use the web interface to:
   - View your discs with custom titles/memos, 25 at a time
   - Click play buttons to directly select and play any disc
   - Edit disc titles and save them to the player and EEPROM
   - Control basic playback functions (play/stop/pause/next/previous)
   - See what is playing in a banner that follows the player, not the buttons

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
- **HTTP Parser** (`HttpParser`): Minimal HTTP request parsing with a 2 s per-request deadline
- **Watchdog**: 5 s hardware watchdog; the board resets itself instead of hanging if the modem or a socket locks up

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
- **Power On / Power Off**: Explicit power commands; no state is tracked, so they cannot get out of sync with the player

### Now Playing Banner
A fixed banner at the bottom of the page appears while the player is playing or paused and shows the disc number, its stored title, the track number and track length. It is hidden when the player is stopped or off.

The banner is driven entirely by status frames the player broadcasts on the S-Link bus; the Arduino listens passively and the page polls `GET /status` every 2 seconds. Pressing buttons in the UI never updates it directly, so it stays correct when the player is operated from its front panel or remote. After the Arduino reboots the state is unknown until the player next reports something, which happens at the latest when the next track starts.

### Set Disc Title
A standalone form for titling any disc by number without scrolling to it. Submits the same command as the per-disc Save button.

### Disc Collection
The list loads 25 discs at a time as you scroll, so the Arduino never serves more than one page per request while the browser keeps every row it has loaded:

- **Infinite Scroll**: The first 25 discs load when the page opens; the next 25 load as you near the bottom, through disc 300
- **Jump to Range**: Pick a range (e.g., "Discs 101-125") from the dropdown to clear the list and restart scrolling from there
- **Per-Disc Controls**, for each loaded disc:
  ```
  Disc 1: [Title Text Field] [Play] [Save] [Auto-Discover]
  ```
  - **Title Field**: Editable text input (13 character limit)
  - **Play**: Immediately selects and plays the disc
  - **Save**: Writes the title to the CDP-CX355's memo memory via S-Link, then reads it back and stores it in EEPROM
  - **Auto-Discover**: Queries the CDP-CX355 for the disc's title (memo or CD TEXT) and stores it in EEPROM. If the player reports no title, the stored title is cleared. An empty slot and a disc with neither memo nor CD TEXT get the same reply, so both come back blank; type the title by hand for the latter

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

### `GET /status`
Last playback state reported by the player.

```json
{"state":"playing","disc":6,"track":2,"len":"3:27","title":"Jurassic Park"}
```

- `state`: `unknown` (nothing heard since boot), `off`, `stopped`, `paused`, `playing`
- `disc`, `track`: 0 until the player has reported them
- `len`: track length `m:ss` from the last track-start frame
- `title`: the EEPROM memo for `disc`, or `""`

### `POST /`
Command execution. Content-Type: `application/x-www-form-urlencoded`. Responds `204 No Content`; results arrive asynchronously over S-Link and are visible by re-fetching `/discs`.

**Transport commands**:
- `command=play` / `stop` / `pause` / `next` / `prev` / `powerOn` / `powerOff`

**Disc operations**:
- `command=playDisc&disc=5` — select and play a disc
- `command=saveDiscTitle&disc=5&title=MyAlbum` — write the title to the CDP-CX355 and sync it back to EEPROM (`setDiscTitle` is accepted as an alias)
- `command=discoverTitle&disc=8` — read a disc's title from the CDP-CX355 into EEPROM

Example automation:
```bash
curl -X POST http://sony-remote.local/ -d "command=playDisc&disc=5"
curl "http://sony-remote.local/discs?page=1&limit=10"
```

## Backing Up and Restoring Titles

The player's memo memory is the primary copy of each title; the Arduino's EEPROM is a cache of it. If only the Arduino is replaced, **Auto-Discover** can re-read titles from the player. A backup protects against the other cases: the player is replaced, its memory is lost, or you want the list in a file.

Both procedures use the HTTP API, so they run from any machine on the network. Set `HOST` to the IP address rather than `sony-remote.local` if `.local` lookups are slow on your machine; the restore loop does one lookup per disc.

### Export

Writes one line per titled disc, `disc<TAB>title`, to `disc-titles.tsv`:

```bash
HOST=http://sony-remote.local
for p in 1 2 3 4 5 6; do
  curl -s "$HOST/discs?page=$p&limit=50" |
    python3 -c 'import sys, json
for d in json.load(sys.stdin)["discs"]:
    if d["m"]: print(d["d"], d["m"], sep="\t")'
done > disc-titles.tsv
```

### Restore

Writes each title to the player over S-Link and syncs it back into EEPROM, exactly as the **Save** button does. Each title takes several seconds on the bus, so the loop waits 5 seconds between discs; a full 300-disc restore takes about 25 minutes. Keep the player powered on.

```bash
HOST=http://sony-remote.local
while IFS=$'\t' read -r disc title; do
  echo "Disc $disc: $title"
  curl -s -o /dev/null -X POST "$HOST/" \
    --data-urlencode "command=saveDiscTitle" \
    --data-urlencode "disc=$disc" \
    --data-urlencode "title=$title"
  sleep 5
done < disc-titles.tsv
```

Edit `disc-titles.tsv` first to restore only some discs. The serial monitor logs `Setting title for disc N` and `Callback received for disc N: <title>` for each one.

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
0x93 - CDP unit, discs 201-300 (disc byte carries disc - 200; untested on hardware)
Disc byte: 1-99 BCD; 100-200 = value + 54 (100 -> 0x9A, 115 -> 0xA9, 200 -> 0xFE).
```

### Commands Used
- Transport: play `0x00`, stop `0x01`, pause `0x02`, next `0x08`, prev `0x09`
- Power: on `0x2E`, off `0x2F`
- Disc select: `0x50` (disc + track)
- Title query: `0x40` (responds with `98 40 ...` followed by ASCII title bytes)
- Memo write: `0x80` (disc + 13 title bytes, space padded)
- Status query: `0x0F` (responds `98 70 00 SS 00 DD 00`: mode flags and current disc, but not play state)

### Status Frames the Player Broadcasts
Captured on a CDP-CX355; they match BigDave's 1998 notes for the CDP-CX250. First byte `98` = "from CD player 1".

| Frame | Meaning |
|---|---|
| `98 00` / `98 01` / `98 02` | Play / stop / pause (also sent as the ack to our commands) |
| `98 10` | Reply to a title query: no memo. Same for an empty slot and an untitled disc |
| `98 0F` | Reply: unknown command |
| `98 50 DD TT MM SS` | Track started: disc, track, track length. Sent ~2.5 s after play and on every track change |
| `98 0C` | 29 seconds left in the track |
| `98 52 DD` / `98 58 DD` | Displaying / loaded disc DD (carousel moves take several seconds) |
| `98 61 FE 6B`, `98 08` | Model id and "ready" after power on |
| `98 2E` / `98 2F` | Power on / off |
| `98 51 TT II MM SS` | Elapsed time, once per second, only after command `0x25` (off with `0x26`); not used |

Disc byte encoding: 1-99 are BCD; 100-200 are `value - 54` (`0x9A`..`0xFE`). Track and times are BCD.

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
- **Auto-Discover blanks the title**: the player answered `98 10`, "no memo". Either the slot is empty or the disc has no memo and no CD TEXT; the player cannot tell these apart, so type the title manually
- **Line protection**: a 220Ω series resistor and diode protect the single-wire bus
- **Device unreachable after a router reboot**: it re-joins on its own within about 10 seconds of the network returning; the serial log shows `WiFi link lost. Reconnecting...` followed by the new IP
- **Device rebooted by itself**: the serial banner reappearing without a power cycle means the watchdog fired; the preceding log lines show which phase hung

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
- CD TEXT discs: the player's own CD TEXT overrides a written memo when the disc is selected

## Future Work

- Indicate in the UI whether a title is editable (memo) or read-only (CD TEXT)
- Playback animations on the LED matrix

## License

[Add your preferred license here]

## Credits

- S-Link protocol implementation based on [Ircama's Sony_SLink library](https://github.com/Ircama/Sony_SLink)
- Arduino framework and WiFiS3 library for network connectivity
- PlatformIO for build system and dependency management
