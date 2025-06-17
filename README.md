# Sony CDP-CX355 WiFi Remote Control

A modern WiFi-based remote control system for Sony CDP-CX355 disc changers using Arduino Uno R4 WiFi and the S-Link protocol. Transform your vintage Sony disc changer into a network-connected device with a web-based interface for disc management and playback control.

## What It Does

This project creates a bridge between your Sony CDP-CX355 disc changer and modern WiFi networks, allowing you to:

- **Remote Control**: Play, stop, pause, skip tracks, and power on/off your disc changer from any device on your network
- **Disc Management**: Browse, edit, and organize disc titles/memos for up to 300 discs through a web interface
- **Direct Disc Selection**: Jump directly to any disc number (1-300) with a single click
- **Real-time Updates**: Interactive web interface that loads disc information dynamically via JSON API
- **LED Matrix Display**: Visual feedback on the Arduino showing connection status and current operations

### User Experience

1. **Setup**: Connect the Arduino to your CDP-CX355's S-Link port and power on
2. **Connect**: The device creates a WiFi connection and displays its IP address on the LED matrix
3. **Control**: Open any web browser and navigate to the device's IP address
4. **Manage**: Use the web interface to:
   - View all your discs with custom titles/memos
   - Click play buttons to directly select and play any disc
   - Edit disc titles and save them to device memory
   - Control basic playback functions (play/stop/pause/next/previous)

## Architecture

### Hardware Components
- **Arduino Uno R4 WiFi**: Main controller with built-in WiFi and LED matrix
- **S-Link Connection**: Communicates with Sony CDP-CX355 using Sony's proprietary S-Link protocol
- **EEPROM Storage**: Persistent storage for disc titles/memos (up to 300 discs)

### Software Components
- **S-Link Protocol**: Custom library for Sony S-Link/Control-A1 communication
- **Web Server**: HTTP server serving both static HTML and JSON API endpoints
- **Disc Storage**: EEPROM-based persistence system for disc metadata
- **LED Matrix Controller**: Visual feedback system using Arduino's built-in LED matrix
- **WiFi Manager**: Network connection and status management

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
- [PlatformIO](https://platformio.org/) or [Arduino IDE](https://www.arduino.cc/en/software)
- Python 3.x (for PlatformIO installation)

### Installation

1. **Clone the Repository**
   ```bash
   git clone <repository-url>
   cd 241219-214510-uno_r4_wifi
   ```

2. **Install PlatformIO** (recommended)
   ```bash
   pip install platformio
   ```

3. **Configure WiFi Credentials**
   - Edit `src/Secrets.hpp`
   - Replace `WIFI_SSID` and `WIFI_PASSWORD` with your network credentials
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
   - Connect Arduino pin 2 to your CDP-CX355's S-Link port
   - Use a 220Ω resistor and diode for line protection
   - Ensure proper ground connection between devices

2. **Power Up**
   - Power on the Arduino (via USB or external power)
   - Power on your CDP-CX355
   - Watch the LED matrix for WiFi connection status

3. **Find Your Device**
   - Check the LED matrix display for the IP address
   - Or check your router's DHCP client list
   - Navigate to `http://[IP-ADDRESS]` in your web browser

### Usage

#### Web Interface
- **Main Page**: Browse disc collection with live-loaded titles
- **Playback Controls**: Transport controls (play/stop/pause/next/previous/power)
- **Disc Selection**: Click "Play" next to any disc to jump directly to it
- **Title Editing**: Modify disc titles inline and save to device memory

#### API Endpoints
- `GET /` - Main web interface
- `GET /discs` - JSON API returning all disc information
- `POST /` - Command endpoint for playback control and disc updates

## Contributing

### Development Setup
1. Follow the installation steps above
2. Make your changes to the source code
3. Test on hardware (the Sony S-Link protocol requires actual hardware)
4. Submit pull requests with clear descriptions

### Code Structure
- `src/main.cpp` - Main application logic and web server
- `src/WifiManager.*` - Network connection management  
- `src/HttpParser.*` - HTTP request parsing
- `src/DiscStorage.hpp` - EEPROM-based disc metadata storage
- `src/LedMatrixController.hpp` - LED matrix display control
- `lib/Sony_SLink/` - S-Link protocol implementation

### Testing
Build and verify compilation:
```bash
pio run
```

### Known Limitations
- Requires physical connection to CDP-CX355 for S-Link communication
- Limited to 300 discs (hardware limitation of CDP-CX355)
- S-Link protocol is slow (~355 bps) - commands take time to execute

## Hardware Compatibility

### Tested Devices
- Sony CDP-CX355 (primary target)

### Likely Compatible Devices
- Sony CDP-CX series multi-disc changers
- Other Sony devices with S-Link/Control-A1 ports

### S-Link Protocol
This project implements Sony's proprietary S-Link (Control-A1) protocol for communication with vintage Sony audio equipment. The protocol is well-documented and should work with other S-Link compatible devices with minimal modifications.

## License

[Add your preferred license here]

## Credits

- S-Link protocol implementation based on [Ircama's Sony_SLink library](https://github.com/Ircama/Sony_SLink)
- Arduino framework and WiFiS3 library for network connectivity
- PlatformIO for build system and dependency management

## Support

If you encounter issues:
1. Verify S-Link hardware connections and line protection
2. Check WiFi credentials in `src/Secrets.hpp`
3. Ensure CDP-CX355 is powered on and responsive
4. Check serial monitor output for debugging information

For questions or contributions, please open an issue in this repository.