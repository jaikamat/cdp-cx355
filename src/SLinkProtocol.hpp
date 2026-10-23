#ifndef SLINK_PROTOCOL_HPP
#define SLINK_PROTOCOL_HPP

#include <Arduino.h>

// Forward declaration
class SLinkProtocol;

// Using a function pointer for callbacks
using SLinkCallback = void (*)(SLinkProtocol* protocol, void* userData);

enum SLinkCommandType {
    CMD_GET_DISC_TITLE = 0x40, // Correct command for disc memo
    CMD_PLAY = 0x00,
    CMD_STOP = 0x01,
    CMD_PAUSE = 0x02,
    CMD_NEXT_TRACK = 0x08,
    CMD_PREV_TRACK = 0x09,
    CMD_SELECT_DISC = 0x50,
    CMD_POWER_ON = 0x2E,
    CMD_POWER_OFF = 0x2F
};

// Playback state as reported by the player itself over the bus. The player
// announces play/stop/pause and every track start unsolicited, and this is
// the only source of truth: nothing in the web UI or command handlers writes
// to it. After a reboot the state is Unknown until the player next speaks.
enum class PlayState : uint8_t { Unknown, Off, Stopped, Paused, Playing };

struct PlaybackStatus {
    PlayState state = PlayState::Unknown;
    uint16_t disc = 0;      // 1-based; 0 = not yet reported
    uint8_t track = 0;      // 1-based; 0 = not yet reported
    uint8_t lengthMin = 0;  // Track length from the last "50" frame
    uint8_t lengthSec = 0;
    unsigned long updatedMs = 0;
};

struct SLinkCommand {
    SLinkCommandType command;
    int disc;
    int track;
    SLinkCallback callback;
    void* userData;
};

class SLinkProtocol {
public:
    SLinkProtocol(uint8_t pin);
    void begin();
    void process();

    // Public command methods
    bool getDiscTitle(int disc, SLinkCallback callback, void* userData);
    bool play();
    bool stop();
    bool pause();
    bool nextTrack();
    bool prevTrack();
    bool selectDisc(int disc);
    bool powerOn();
    bool powerOff();
    void setDiscTitle(int disc, const String& title);

    // Getters for application to retrieve data
    const char* getTitle() const;
    const PlaybackStatus& getPlaybackStatus() const { return _status; }

private:
    uint8_t _pin;
    static const int MAX_COMMANDS = 10;
    SLinkCommand _commandQueue[MAX_COMMANDS];
    int _commandCount;

    SLinkCommand _activeCommand;
    SLinkCommand* _currentCommand;

    // Command processing state machines
    void processGetDiscTitle();
    void processSimpleCommand();
    void processSelectDisc();

    // Low-level communication
    void _lineReady();
    void _writeSync();
    void _writeByte(byte value);
    void sendCommand(const uint8_t* command, size_t length);
    String inputMonitorWithReturn(int type, boolean idle, unsigned long uSecTimeout);
    uint8_t toBCD(int val);
    uint8_t encodeDiscByte(int disc);
    bool isBusIdle();

    void addCommand(SLinkCommandType command, int disc, int track, SLinkCallback callback, void* userData);
    void executeNextCommand();

    char _titleBuffer[21]; // For storing disc title

    // ---- Passive bus listener ----
    // An interrupt on the S-Link pin records the width of every LOW pulse
    // into a ring buffer; process() decodes those widths into frames and
    // feeds them to handleBusFrame(). A zero entry marks a gap between
    // frames. Indices are uint8_t so they wrap at 256 for free.
    static const uint16_t FRAME_GAP = 0;
    static const uint32_t FRAME_GAP_US = 5000;
    static SLinkProtocol* _instance;
    static void onEdgeISR();
    void handleEdge();
    void attachListener();
    void detachListener();
    void drainBusCapture();
    void finishBusFrame();
    void handleBusFrame(const uint8_t* frame, int len);
    static uint16_t decodeDiscByte(uint8_t b);

    volatile uint16_t _pulses[256];
    volatile uint8_t _pulseHead = 0;
    volatile uint8_t _pulseTail = 0;
    volatile uint32_t _lowStartUs = 0;
    volatile uint32_t _lastRiseUs = 0;
    uint8_t _rxFrame[16];
    int _rxLen = 0;
    uint8_t _rxByte = 0;
    int _rxBits = 0;
    bool _rxInFrame = false;
    PlaybackStatus _status;
};

#endif // SLINK_PROTOCOL_HPP
