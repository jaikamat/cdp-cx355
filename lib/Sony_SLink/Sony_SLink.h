/*
  Arduino SONY S-LINK/Control-A1 Protocol Library

  https://github.com/Ircama/Sony_SLink.git

  (C) Ircama, 2017, CC-BY-SA 4.0
  https://creativecommons.org/licenses/by-sa/4.0/

  Feel free to share this source code, but include explicit mention to the author.
  Licensed under creative commons - see http://creativecommons.org/licenses/by-sa/3.0/
*/

#ifndef SLINK_H
#define SLINK_H
#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

// Ref. http://boehmel.de/slink.htm

#define SLINK_DEVICE_MD 176         // MD Send Commands (to MD recorder) prefix 10110000b = B0h
#define SLINK_DEVICE_AMP 192        // AMP Send Commands (to amplifier) 11000000b=C0h - Send commands to a Sony STR-DA50ES receiver/amplifier
#define SLINK_DEVICE_TUNER 193      // TUNER Send Commands (to tuner) 11000001b=C1h
#define SLINK_DEVICE_SURROUND 194   // SUR Send Commands (to surround) prefix 11000011b = C3h
#define SLINK_DEVICE_AMP_NEW 112    // AMP Send Commands (to new models of amplifier like STR-DA1000ES) 01110000b=70h
#define SLINK_DEVICE_CDP_CX1L 0x90  // Send commands to a Sony CDP-CX jukebox for CD Player 1 to drive number <=200
#define SLINK_DEVICE_CDP_CX1H 0x93  // Send commands to a Sony CDP-CX jukebox for CD Player 1 to drive disc number > 200
#define SLINK_DEVICE_CDP_CX2L 0x91  // Send commands to a Sony CDP-CX jukebox for CD Player 2 to drive number <=200
#define SLINK_DEVICE_CDP_CX2H 0x94  // Send commands to a Sony CDP-CX jukebox for CD Player 2 to drive disc number > 200
#define SLINK_DEVICE_CDP_CX3L 0x92  // Send commands to a Sony CDP-CX jukebox for CD Player 3 to drive number <=200
#define SLINK_DEVICE_CDP_CX3H 0x95  // Send commands to a Sony CDP-CX jukebox for CD Player 3 to drive disc number > 200
#define SLINK_DEVICE_CDP_CXALL 0x97 // Send commands to a Sony CDP-CX jukebox for all CD players 1, 2, 3 (200 or less disc players only)

// List of commands for AMP/RECEIVER
#define SLINK_CMD_AMP_MUTE_ON 6
#define SLINK_CMD_AMP_MUTE_OFF 7
#define SLINK_CMD_AMP_5_1_IN_ON 12
#define SLINK_CMD_AMP_5_1_IN_OFF 13
#define SLINK_CMD_AMP_VOLUME_UP 20
#define SLINK_CMD_AMP_VOLUME_DOWN 21
#define SLINK_CMD_AMP_POWER_ON 46
#define SLINK_CMD_AMP_POWER_OFF 47
#define SLINK_CMD_AMP_SET_INPUT_CHAN 80     // 0x50: select input channel (check the below list of parameters)
#define SLINK_CMD_AMP_SET_2ND_IN_AUDIO 0x52 // Set 2nd Audio	00=Tuner, 02=CD, 03=DAT, 04=MD, 05=Tape, 0F=Source (answer 71)
#define SLINK_CMD_AMP_SET_IN_TYPE 0x83      // Set input type	01=optical, 02=coax, 04=analog, answer 43
#define SLINK_CMD_AMP_SET_SOURCE_NAME 0x88  // 88 SS [8 x CC]	Set Source name	SS=Source (see command 0x50)

// List of commands for TUNER
#define SLINK_CMD_TUNER_BAND 3 // 03 BB	Band	BB=00: FM, BB=01: AM, answer 70
#define SLINK_CMD_TUNER_SCAN_UP 6
#define SLINK_CMD_TUNER_SCAN_DOWN 7
#define SLINK_CMD_TUNER_PRESET_UP 8
#define SLINK_CMD_TUNER_PRESET_DOWN 9
#define SLINK_CMD_TUNER_MONO 10
#define SLINK_CMD_TUNER_STEREO 11

#define SLINK_CMD_TUNER_DIRECT_TUNE 0x50 // 50 BB HH LL	Direct Tune
                                         // BB=00: FM, (HH*256+LL)/100= frequency [MHz] in 50 kHz steps (value rounded down)
                                         // BB=01: AM, HH*256+LL= frequency [kHz] in 9 kHz (USA: 10 kHz) steps (value rounded down)

#define SLINK_CMD_TUNER_PRESET_STATION 0x51 // 51 BB NN	Preset Station	BB=[01..03] Bank A..C, NN=[00..09] Station number, answer 70, E1

#define SLINK_CMD_TUNER_PTY_SEARCH 0x52 // 52 PP	PTY Search
                                        // 00=None, 01=News, 02=Current Affairs, 03=Information, 04=Sports, 05=Education, 06=Drama, 07=Culture...

// List of commands for CDP, MD
#define SLINK_CMD_CD_PLAY 0x00         // Play or start recording
#define SLINK_CMD_CD_STOP 0x01         // Stop
#define SLINK_CMD_CD_PAUSE 0x02        // Pause
#define SLINK_CMD_CD_PAUSE_TOGGLE 0x03 // Toggle Pause
#define SLINK_CMD_CD_EJECT 0x04        // Eject
#define SLINK_CMD_CD_RECORD_PAUSE 0x07 // Record Pause
#define SLINK_CMD_CD_NEXT 0x08         // Next Track
#define SLINK_CMD_CD_PREV 0x09         // Previous Track
#define SLINK_CMD_CD_FWD_FAST 0x10     // Forward Fast
#define SLINK_CMD_CD_REW_FAST 0x11     // Rewind Fast
#define SLINK_CMD_CD_FWD_SLOW 0x12     // Forward Slow
#define SLINK_CMD_CD_REW_SLOW 0x13     // Rewind Slow
#define SLINK_CMD_CD_NORMAL 0x1F       // End Forward or Rewind

#define SLINK_CMD_CD_REMOTE_DISABLE 0x20 // Disable remote control
#define SLINK_CMD_CD_REMOTE_ENABLE 0x21  // Enable remote control

#define SLINK_CMD_CD_QUERY_STORED 0x0E // Query Stored (Answer 71)
#define SLINK_CMD_CD_SETUP_INFO 0x0F   // Setup Info (Answer 70)
#define SLINK_CMD_CD_DEVICE_TYPE 0x22  // Query Device Type (Answer 61)

#define SLINK_CMD_CD_AUTO_PAUSE_OFF 0x23  // Auto Pause Off
#define SLINK_CMD_CD_AUTO_PAUSE_ON 0x24   // Auto Pause On
#define SLINK_CMD_CD_TIME_UPDATE_ON 0x25  // Time Update On
#define SLINK_CMD_CD_TIME_UPDATE_OFF 0x26 // Time Update Off

#define SLINK_CMD_CD_POWER_ON 0x2E  // Power On
#define SLINK_CMD_CD_POWER_OFF 0x2F // Power Off

#define SLINK_CMD_CD_DIVIDE 0x32   // Divide
#define SLINK_CMD_CD_MENU_YES 0x33 // Menu Yes
#define SLINK_CMD_CD_MENU_NO 0x34  // Menu No
#define SLINK_CMD_CD_UNDO 0x35     // Undo

#define SLINK_CMD_CD_DISABLE_KEYS 0x3E // Disable device keys
#define SLINK_CMD_CD_ENABLE_KEYS 0x3F  // Enable device keys

// Track Operations
#define SLINK_CMD_CD_ERASE_TRACK 0x40        // Erase Track
#define SLINK_CMD_CD_MOVE_TRACK 0x41         // Move Track
#define SLINK_CMD_CD_COMBINE_TRACK_PREV 0x42 // Combine with Previous Track
#define SLINK_CMD_CD_COMBINE_TRACK 0x43      // Combine with Track TT

// Query Operations
#define SLINK_CMD_CD_DOWNLOAD_TITLE 0x40   // Download Title (Answer: 40 dsk ascii_text) - CDP-CX250 or higher
#define SLINK_CMD_CD_QUERY_DISC 0x44       // Query Disc Info (Answer 60)
#define SLINK_CMD_CD_QUERY_TRACK 0x45      // Query Track Info (Answer 62)
#define SLINK_CMD_CD_QUERY_DISC_NAME 0x58  // Query Disc Name (Answer 16 if no disc name)
#define SLINK_CMD_CD_QUERY_TRACK_NAME 0x5A // Query Track Name (Answer 17 if no track name)

// Playback Controls
#define SLINK_CMD_CD_PLAY_DIRECT_TRACK 0x50  // Play Direct Track
#define SLINK_CMD_CD_PAUSE_DIRECT_TRACK 0x51 // Pause Direct Track

// Time Operations
#define SLINK_CMD_CD_REMAIN_TIME_DISC 0x54 // Remaining Time on Disc

// Titling Operations
#define SLINK_CMD_CD_WRITE_DISC_TEXT 0x98       // Write Disc Text
#define SLINK_CMD_CD_WRITE_DISC_MORE_TEXT 0x99  // Write More Disc Text
#define SLINK_CMD_CD_WRITE_TRACK_TEXT 0x9A      // Write Track Text
#define SLINK_CMD_CD_WRITE_TRACK_MORE_TEXT 0x9B // Write More Track Text

// Other Operations
#define SLINK_CMD_CD_DEVICE_NAME 0x6A     // Device Name
#define SLINK_CMD_CD_RECORD_DATE 0x72     // Record Date
#define SLINK_CMD_CD_ADJUST_POINT_A 0x7A  // Adjust Point A
#define SLINK_CMD_CD_ADJUST_POINT_B 0x7B  // Adjust Point B
#define SLINK_CMD_CD_CONFIRM_POINT_A 0x7C // Confirm Point A
#define SLINK_CMD_CD_CONFIRM_POINT_B 0x7D // Confirm Point B

// List of parameters for SLINK_CMD_AMP_SET_INPUT_CHAN
#define SLINK_CMDP_IN_TUNER 0
#define SLINK_CMDP_IN_PHONO 1
#define SLINK_CMDP_IN_CD 2
#define SLINK_CMDP_IN_DAT 3
#define SLINK_CMDP_IN_MD 4
#define SLINK_CMDP_IN_TAPE 5
#define SLINK_CMDP_IN_DIGITAL_AUDIO 7
#define SLINK_CMDP_IN_VIDEO1 16
#define SLINK_CMDP_IN_VIDEO2 17
#define SLINK_CMDP_IN_VIDEO3 18
#define SLINK_CMDP_IN_VIDEO4 19
#define SLINK_CMDP_IN_DVD_LD 21
#define SLINK_CMDP_IN_TV_SAT 22
#define SLINK_CMDP_IN_TV 23
#define SLINK_CMDP_IN_DVD 25

// S-LINK Protocol
#define SLINK_MARK_SYNC 2400       // microseconds
#define SLINK_MARK_ONE 1200        // microseconds
#define SLINK_MARK_ZERO 600        // microseconds
#define SLINK_MARK_DELIMITER 600   // microseconds
#define SLINK_MARK_RANGE +1.2      // timing detection tolerance
#define SLINK_WORD_DELIMITER 30000 // padding microseconds after transmitting a command (45msecs should be the correct value...)
#define SLINK_LINE_READY 3000      // microseconds to wait for the line to become ready before transmitting a command
#define SLINK_LOOP_DELAY 25        // microseconds timer thick during a wait operation
#define SLINK_LOOP_TIMEOUT 500000  // microseconds total timeout while waiting

class Slink
{
public:
    void init(int slinkPin);
    void sendCommand(unsigned int deviceId = 0x00, unsigned int commandId1 = 0x00, int commandId2 = -1, int commandId3 = -1);
    // New method to send a larger payload:
    void sendLongCommand(uint8_t deviceId, const uint8_t *data, size_t length);
    // Method to receive S-Link responses
    String receiveResponse(unsigned long timeoutMs = 5000);
    // Method specifically for receiving title responses
    String receiveTitle(unsigned long timeoutMs = 5000);
    // Send command and immediately listen for response (no padding delay)
    String sendCommandAndReceive(unsigned int deviceId, unsigned int commandId1, int commandId2 = -1, unsigned long timeoutMs = 5000);
    // Send command and capture response using working inputMonitor approach
    String sendCommandAndCapture(unsigned int deviceId, unsigned int commandId1, int commandId2 = -1, unsigned long timeoutMs = 3000);
    // Send command and capture response using exact inputMonitor approach
    String sendCommandAndMonitor(unsigned int deviceId, unsigned int commandId1, int commandId2 = -1);
    // Capture response using inputMonitor logic but return the result
    String captureInputMonitor(unsigned long uSecTimeout = 3000000UL);
    // Simple capture method that processes data bits directly without sync detection
    String captureDataBits(unsigned long uSecTimeout = 3000000UL);
    // Modified inputMonitor that returns the captured hex string instead of printing
    String inputMonitorCapture(unsigned long uSecTimeout = 3000000UL);
#if !defined(__AVR_ATtiny85__)
    void inputMonitor(int type = 0, boolean idle = false, unsigned long uSecTimeout = 10000000UL, unsigned long serialSpeed = 115200UL);
    // Version of inputMonitor that returns the buffer instead of printing it
    String inputMonitorWithReturn(int type = 2, boolean idle = false, unsigned long uSecTimeout = 3000000UL);
#endif
    int pin();

private:
    int _slinkPin;
    void _lineReady();
    void _writeSync();
    void _writeByte(byte value);
    // Private methods for receiving data
    uint8_t _readByte(unsigned long timeoutMs);
    bool _waitForSync(unsigned long timeoutMs);
    unsigned long _readBitTiming();
};

#endif