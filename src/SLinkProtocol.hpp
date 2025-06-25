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

struct SLinkCommand {
    SLinkCommandType command;
    int disc;
    int track;
    int state;
    unsigned long timeout;
    void* data;
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
    void queryDiscMemoryInfo(int disc);
    void setDiscTitle(int disc, const String& title);

    // Getters for application to retrieve data
    const char* getTitle() const;

private:
    uint8_t _pin;
    static const int MAX_COMMANDS = 10;
    SLinkCommand _commandQueue[MAX_COMMANDS];
    int _commandCount;

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
    int readResponse(uint8_t* buffer, size_t length, unsigned long timeout);
    String inputMonitorWithReturn(int type, boolean idle, unsigned long uSecTimeout);
    uint8_t toBCD(int val);

    void addCommand(SLinkCommandType command, int disc, int track, SLinkCallback callback, void* userData, void* data = nullptr);
    void executeNextCommand();

    char _titleBuffer[21]; // For storing disc title
    uint8_t _responseBuffer[32]; // For reading responses
};

#endif // SLINK_PROTOCOL_HPP
