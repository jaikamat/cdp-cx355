#include "SLinkProtocol.hpp"

// Timings from Sony_SLink.h
#define SLINK_MARK_SYNC 2400
#define SLINK_MARK_ONE 1200
#define SLINK_MARK_ZERO 600
#define SLINK_MARK_DELIMITER 600
#define SLINK_WORD_DELIMITER 30000
#define SLINK_LINE_READY 3000
#define SLINK_LOOP_DELAY 25
#define SLINK_LOOP_TIMEOUT 500000
#define SLINK_MARK_RANGE 1.2

SLinkProtocol::SLinkProtocol(uint8_t pin) : _pin(pin), _commandCount(0), _currentCommand(nullptr)
{
    memset(_titleBuffer, 0, sizeof(_titleBuffer));
}

void SLinkProtocol::begin()
{
    pinMode(_pin, INPUT);
}

void SLinkProtocol::process()
{
    if (_currentCommand)
    {
        switch (_currentCommand->command)
        {
        case CMD_GET_DISC_TITLE:
            processGetDiscTitle();
            break;
        case CMD_PLAY:
        case CMD_STOP:
        case CMD_PAUSE:
        case CMD_NEXT_TRACK:
        case CMD_PREV_TRACK:
        case CMD_POWER_ON:
        case CMD_POWER_OFF:
            processSimpleCommand();
            break;
        case CMD_SELECT_DISC:
            processSelectDisc();
            break;
        }
    }
    else if (isBusIdle())
    {
        executeNextCommand();
    }
}

bool SLinkProtocol::getDiscTitle(int disc, SLinkCallback callback, void *userData)
{
    if (_commandCount >= MAX_COMMANDS)
        return false;
    addCommand(CMD_GET_DISC_TITLE, disc, 0, callback, userData);
    return true;
}

bool SLinkProtocol::play()
{
    if (_commandCount >= MAX_COMMANDS)
        return false;
    addCommand(CMD_PLAY, 0, 0, nullptr, nullptr);
    return true;
}

bool SLinkProtocol::stop()
{
    if (_commandCount >= MAX_COMMANDS)
        return false;
    addCommand(CMD_STOP, 0, 0, nullptr, nullptr);
    return true;
}

bool SLinkProtocol::pause()
{
    if (_commandCount >= MAX_COMMANDS)
        return false;
    addCommand(CMD_PAUSE, 0, 0, nullptr, nullptr);
    return true;
}

bool SLinkProtocol::nextTrack()
{
    if (_commandCount >= MAX_COMMANDS)
        return false;
    addCommand(CMD_NEXT_TRACK, 0, 0, nullptr, nullptr);
    return true;
}

bool SLinkProtocol::prevTrack()
{
    if (_commandCount >= MAX_COMMANDS)
        return false;
    addCommand(CMD_PREV_TRACK, 0, 0, nullptr, nullptr);
    return true;
}

bool SLinkProtocol::selectDisc(int disc)
{
    if (_commandCount >= MAX_COMMANDS)
        return false;
    addCommand(CMD_SELECT_DISC, disc, 1, nullptr, nullptr); // Default to track 1
    return true;
}

bool SLinkProtocol::powerOn()
{
    if (_commandCount >= MAX_COMMANDS)
        return false;
    addCommand(CMD_POWER_ON, 0, 0, nullptr, nullptr);
    return true;
}

bool SLinkProtocol::powerOff()
{
    if (_commandCount >= MAX_COMMANDS)
        return false;
    addCommand(CMD_POWER_OFF, 0, 0, nullptr, nullptr);
    return true;
}

const char *SLinkProtocol::getTitle() const
{
    return _titleBuffer;
}

void SLinkProtocol::processGetDiscTitle()
{
    uint8_t device = (_currentCommand->disc > 200) ? 0x93 : 0x90;
    uint8_t discByte = toBCD(_currentCommand->disc > 200 ? _currentCommand->disc - 200 : _currentCommand->disc);

    // The reply we want is the frame "98 40 <disc> <13 title bytes>". The
    // capture window also picks up unsolicited status frames from the player
    // (e.g. "92 22"), so we locate this exact header and parse only the bytes
    // that follow it, up to the end of that frame. inputMonitorWithReturn
    // renders each frame as "\nSTART,<hex>,<hex>,...\n" with unpadded
    // lowercase hex, so the header for disc 7 is "98,40,7,".
    String frameHeader = String("98,40,") + String(discByte, HEX) + ",";

    String response;
    int frameIdx = -1;
    for (int attempt = 1; attempt <= 5; attempt++)
    {
        uint8_t cmd[] = {device, (uint8_t)CMD_GET_DISC_TITLE, discByte};
        sendCommand(cmd, sizeof(cmd));
        delayMicroseconds(attempt * 500); // 0.5ms, 1ms, 1.5ms...

        response = inputMonitorWithReturn(2, false, 200000UL); // 0.2 seconds

        frameIdx = response.indexOf(frameHeader);
        if (frameIdx >= 0)
        {
            break;
        }
        delay(100);
    }

    Serial.print("GetDiscTitle Response: ");
    Serial.println(response);

    if (frameIdx >= 0)
    {
        // Title bytes run from just after the header to the end of the frame.
        int titleStart = frameIdx + frameHeader.length();
        int frameEnd = response.indexOf('\n', titleStart);
        if (frameEnd < 0)
            frameEnd = response.length();

        String title;
        int pos = titleStart;
        while (pos < frameEnd)
        {
            int comma = response.indexOf(',', pos);
            if (comma < 0 || comma > frameEnd)
                comma = frameEnd;
            if (comma > pos)
            {
                long byteVal = strtol(response.substring(pos, comma).c_str(), NULL, 16);
                if (byteVal >= 0x20 && byteVal <= 0x7E)
                {
                    title += (char)byteVal;
                }
            }
            pos = comma + 1;
        }
        title.trim();
        strncpy(_titleBuffer, title.c_str(), 20);
        _titleBuffer[20] = '\0';
    }
    else
    {
        // No frame for this disc arrived. The callback treats this sentinel
        // as "leave EEPROM alone" rather than clearing the stored title.
        strcpy(_titleBuffer, "Timeout");
    }

    if (_currentCommand->callback)
    {
        _currentCommand->callback(this, _currentCommand->userData);
    }
    _currentCommand = nullptr;
    executeNextCommand();
}

void SLinkProtocol::processSimpleCommand()
{
    uint8_t cmd[] = {0x90, (uint8_t)_currentCommand->command};

    // Transport commands (play/stop/pause/next/prev) are fire-and-forget
    // Send once without retry to prevent multiple executions
    sendCommand(cmd, sizeof(cmd));
    delayMicroseconds(500);
    String response = inputMonitorWithReturn(2, false, 200000UL);

    Serial.print("SimpleCommand (0x");
    Serial.print(_currentCommand->command, HEX);
    Serial.print(") Response: ");
    Serial.println(response);

    if (_currentCommand->callback)
    {
        _currentCommand->callback(this, _currentCommand->userData);
    }
    _currentCommand = nullptr;
    executeNextCommand();
}

void SLinkProtocol::processSelectDisc()
{
    uint8_t device = (_currentCommand->disc > 200) ? 0x93 : 0x90;
    uint8_t discByte = toBCD(_currentCommand->disc > 200 ? _currentCommand->disc - 200 : _currentCommand->disc);
    uint8_t trackByte = toBCD(_currentCommand->track);
    uint8_t cmd[] = {device, (uint8_t)CMD_SELECT_DISC, discByte, trackByte};

    String response;
    for (int attempt = 1; attempt <= 5; attempt++)
    {
        sendCommand(cmd, sizeof(cmd));
        delayMicroseconds(attempt * 500); // 0.5ms, 1ms, 1.5ms...
        response = inputMonitorWithReturn(2, false, 200000UL);
        // For select disc, we don't have a clear expected response, so we just capture.
        // A more advanced implementation could wait for a specific status like 'Playing'.
        if (response.length() > 0)
        {
            break;
        }
        delay(100);
    }

    Serial.print("SelectDisc Response: ");
    Serial.println(response);

    if (_currentCommand->callback)
    {
        _currentCommand->callback(this, _currentCommand->userData);
    }
    _currentCommand = nullptr;
    executeNextCommand();
}

void SLinkProtocol::_lineReady()
{
    unsigned long Start = micros();
    unsigned long beginTimeout = Start;
    do
    {
        delayMicroseconds(SLINK_LOOP_DELAY);
        if (digitalRead(_pin) == LOW)
            Start = micros();
    } while ((micros() - Start < SLINK_LINE_READY) && (micros() - beginTimeout < SLINK_LOOP_TIMEOUT));
}

void SLinkProtocol::_writeSync()
{
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    delayMicroseconds(SLINK_MARK_SYNC);
    digitalWrite(_pin, HIGH);
    delayMicroseconds(SLINK_MARK_DELIMITER);
}

void SLinkProtocol::_writeByte(byte value)
{
    pinMode(_pin, OUTPUT);
    for (int i = 7; i >= 0; i--)
    {
        if (value & 1 << i)
        {
            digitalWrite(_pin, LOW);
            delayMicroseconds(SLINK_MARK_ONE);
            digitalWrite(_pin, HIGH);
            delayMicroseconds(SLINK_MARK_DELIMITER);
        }
        else
        {
            digitalWrite(_pin, LOW);
            delayMicroseconds(SLINK_MARK_ZERO);
            digitalWrite(_pin, HIGH);
            delayMicroseconds(SLINK_MARK_DELIMITER);
        }
    }
}

void SLinkProtocol::sendCommand(const uint8_t *command, size_t length)
{
    unsigned long Start;
    pinMode(_pin, INPUT);
    _lineReady();
    Start = micros();
    pinMode(_pin, OUTPUT);
    _writeSync();
    for (size_t i = 0; i < length; ++i)
    {
        _writeByte(command[i]);
    }
    pinMode(_pin, INPUT);
    do
    {
        delayMicroseconds(SLINK_LOOP_DELAY);
        if (digitalRead(_pin) == LOW)
            break;
    } while (micros() - Start < SLINK_WORD_DELIMITER);
}

String SLinkProtocol::inputMonitorWithReturn(int type, boolean idle, unsigned long uSecTimeout)
{
    unsigned long value = 0;
    unsigned long Start = micros();
    int nl = 0;
    int count = 0;
    int byte = 0;
    String buffer = "";

    do
    {
        value = pulseIn(_pin, idle, 3000UL); // timeout to 3 milliseconds=3000 uSec
        if (value == 0)
        {
            if (nl == 0)
                buffer += String("\n");
            nl = 1;
            count = 0;
            byte = 0;
        }
        else
        {
            switch (type)
            {
            case 0: // timing
                buffer += String(",");
                buffer += String(value);
                break;
            case 1: // binary - HEX
            case 2: // HEX
                if ((value > (SLINK_MARK_SYNC / SLINK_MARK_RANGE)) && (value < SLINK_MARK_SYNC * SLINK_MARK_RANGE))
                {
                    buffer += String("\n");
                    buffer += String("START,");
                    count = 0;
                    byte = 0;
                }
                else
                {
                    if ((value > SLINK_MARK_ONE / SLINK_MARK_RANGE) && (value < SLINK_MARK_ONE * SLINK_MARK_RANGE))
                    {
                        byte |= 128 >> count;

                        if (type == 1)
                            buffer += String("1,");
                    }
                    if ((value > SLINK_MARK_ZERO / SLINK_MARK_RANGE) && (value < SLINK_MARK_ZERO * SLINK_MARK_RANGE))
                        if (type == 1)
                            buffer += String("0,");

                    if (count++ == 7)
                    {
                        if (type == 1)
                            buffer += String(" = ");
                        buffer += String(byte, HEX) + String(",");
                        if (type == 1)
                            buffer += String("  ");
                        count = 0;
                        byte = 0;
                    }
                }
                break;
            }
            nl = 0;
        } // else
    } // do
    while (micros() - Start < uSecTimeout);
    return buffer;
}

uint8_t SLinkProtocol::toBCD(int val)
{
    return ((val / 10) << 4) | (val % 10);
}

void SLinkProtocol::addCommand(SLinkCommandType command, int disc, int track, SLinkCallback callback, void *userData)
{
    if (_commandCount < MAX_COMMANDS)
    {
        Serial.print("Adding command to queue: 0x");
        Serial.println(command, HEX);
        SLinkCommand &cmd = _commandQueue[_commandCount++];
        cmd.command = command;
        cmd.disc = disc;
        cmd.track = track;
        cmd.callback = callback;
        cmd.userData = userData;
    }
    else
    {
        Serial.println("Command queue full!");
    }
}

void SLinkProtocol::executeNextCommand()
{
    if (!_currentCommand && _commandCount > 0)
    {
        Serial.println("Executing next command from queue.");
        // Copy the head command out before shifting; shifting overwrites
        // slot 0 in place, so pointing into the queue would corrupt it.
        _activeCommand = _commandQueue[0];
        for (int i = 0; i < _commandCount - 1; ++i)
        {
            _commandQueue[i] = _commandQueue[i + 1];
        }
        _commandCount--;
        _currentCommand = &_activeCommand;
    }
}

// Deliberately synchronous (not queued): it runs inside the HTTP handler,
// before loop() next calls process(), so a getDiscTitle queued right after
// it is guaranteed to execute once the title write has finished. Queueing
// it would also require copying the title String into the command struct.
void SLinkProtocol::setDiscTitle(int disc, const String& title) {
    uint8_t device = (disc > 200) ? 0x93 : 0x90;
    uint8_t discByte = toBCD(disc > 200 ? disc - 200 : disc);

    uint8_t cmd[16];
    cmd[0] = device;
    cmd[1] = 0x80;
    cmd[2] = discByte;

    for (int i = 0; i < 13; i++) {
        if (i < title.length()) {
            cmd[3 + i] = title[i];
        } else {
            cmd[3 + i] = 0x20; // Pad with spaces
        }
    }

    String response;
    for (int attempt = 1; attempt <= 5; attempt++) {
        sendCommand(cmd, sizeof(cmd));
        delayMicroseconds(attempt * 500);
        response = inputMonitorWithReturn(2, false, 200000UL);
        if (response.length() > 0) { // Any response is good enough for now
            break;
        }
        delay(100);
    }

    Serial.print("SetDiscTitle Response: ");
    Serial.println(response);

    response.toUpperCase();
    if (response.indexOf("1B") == -1) {
        Serial.println("SUCCESS: Text write command sent. Assuming success as no error was received.");
    } else {
        Serial.println("ERROR: Text write command failed (Received 1B error)");
    }
}

bool SLinkProtocol::isBusIdle() {
    unsigned long startTime = millis();
    while (millis() - startTime < 50) { // Check for 50ms
        if (digitalRead(_pin) == LOW) {
            return false; // Bus is busy
        }
        delay(1);
    }
    return true; // Bus is idle
}
