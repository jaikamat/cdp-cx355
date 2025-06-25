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
        if (_currentCommand->timeout > 0 && millis() > _currentCommand->timeout)
        {
            Serial.println("Command timed out");
            _currentCommand = nullptr;
            executeNextCommand();
            return;
        }

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

    String response;
    for (int attempt = 1; attempt <= 5; attempt++)
    {
        uint8_t cmd[] = {device, (uint8_t)CMD_GET_DISC_TITLE, discByte};
        sendCommand(cmd, sizeof(cmd));
        delayMicroseconds(attempt * 500); // 0.5ms, 1ms, 1.5ms...

        response = inputMonitorWithReturn(2, false, 200000UL); // 0.2 seconds

        if (response.indexOf("98,40,") >= 0)
        {
            break;
        }
        delay(100);
    }

    Serial.print("GetDiscTitle Response: ");
    Serial.println(response);

    if (response.length() > 0)
    {
        response.replace(",", " ");
        response.replace("START", "");
        response.replace("\n", " ");
        response.trim();

        String title;
        int start = 0;
        int byteCount = 0;
        while (start < response.length())
        {
            int end = response.indexOf(' ', start);
            if (end == -1)
                end = response.length();
            String hexByte = response.substring(start, end);
            if (hexByte.length() > 0)
            {
                long byteVal = strtol(hexByte.c_str(), NULL, 16);
                byteCount++;
                if (byteCount >= 4 && byteVal >= 0x20 && byteVal <= 0x7E)
                {
                    title += (char)byteVal;
                }
            }
            start = end + 1;
        }
        title.trim();
        strncpy(_titleBuffer, title.c_str(), 20);
    }
    else
    {
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
    String response;
    char expectedResponse[10];
    sprintf(expectedResponse, "98,%x,", _currentCommand->command);

    for (int attempt = 1; attempt <= 5; attempt++)
    {
        sendCommand(cmd, sizeof(cmd));
        delayMicroseconds(attempt * 500); // 0.5ms, 1ms, 1.5ms...
        response = inputMonitorWithReturn(2, false, 200000UL);
        if (response.indexOf(expectedResponse) >= 0)
        {
            break;
        }
        delay(100);
    }

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

int SLinkProtocol::readResponse(uint8_t *buffer, size_t length, unsigned long timeout_ms)
{
    unsigned long startTime = millis();
    int byteCount = 0;
    int bitCount = 0;
    uint8_t currentByte = 0;
    bool syncFound = false;

    memset(buffer, 0, length);

    while (millis() - startTime < timeout_ms)
    {
        unsigned long pulseDuration;

        // --- Critical Section: Measure one pulse without interruption ---
        noInterrupts();
        pulseDuration = pulseIn(_pin, LOW, 5000UL); // 5ms timeout for a single pulse
        interrupts();
        // --- End Critical Section ---

        if (pulseDuration == 0)
        { // No pulse was detected
            if (syncFound && byteCount > 0)
            {
                // If we were in the middle of a message, it has ended.
                return byteCount;
            }
            continue; // Otherwise, just keep waiting for a sync pulse.
        }

        if (!syncFound)
        {
            // We are waiting for a sync pulse to start a message.
            if (pulseDuration > 2000 && pulseDuration < 3000)
            { // Sync is ~2400us
                syncFound = true;
                bitCount = 0;
                byteCount = 0;
                currentByte = 0;
            }
            // If it wasn't a sync pulse, ignore it and keep waiting.
            continue;
        }

        // --- If we are here, a sync has been found and we are decoding data ---
        bool isOne = (pulseDuration > 900 && pulseDuration < 1500); // ONE is ~1200us
        bool isZero = (pulseDuration > 400 && pulseDuration < 900); // ZERO is ~600us

        if (isOne)
        {
            currentByte |= (1 << (7 - bitCount));
            bitCount++;
        }
        else if (isZero)
        {
            bitCount++;
        }
        else
        {
            // This was not a valid data bit. The message may have been corrupted or ended.
            // If we have some data, return it. Otherwise, wait for a new sync.
            if (byteCount > 0)
                return byteCount;
            syncFound = false;
            continue;
        }

        // If we have collected 8 bits, we have a full byte.
        if (bitCount == 8)
        {
            if (byteCount < length)
            {
                buffer[byteCount] = currentByte;
            }
            byteCount++;
            bitCount = 0;
            currentByte = 0;
            // If the buffer is full, we're done.
            if (byteCount >= length)
                return byteCount;
        }
    }

    return byteCount; // Return whatever we have if we time out.
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

void SLinkProtocol::addCommand(SLinkCommandType command, int disc, int track, SLinkCallback callback, void *userData, void *data)
{
    if (_commandCount < MAX_COMMANDS)
    {
        Serial.print("Adding command to queue: 0x");
        Serial.println(command, HEX);
        SLinkCommand &cmd = _commandQueue[_commandCount++];
        cmd.command = command;
        cmd.disc = disc;
        cmd.track = track;
        cmd.state = 0;
        cmd.timeout = 0;
        cmd.data = data;
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
        _currentCommand = &_commandQueue[0];
        for (int i = 0; i < _commandCount - 1; ++i)
        {
            _commandQueue[i] = _commandQueue[i + 1];
        }
        _commandCount--;
    }
}

void SLinkProtocol::queryDiscMemoryInfo(int disc) {
    uint8_t device = (disc > 200) ? 0x93 : 0x90;
    uint8_t discByte = toBCD(disc > 200 ? disc - 200 : disc);

    // --- Check for MEMO using 0x0E command ---
    uint8_t cmd_memo[] = {device, 0x0E, discByte};
    String response_memo;
    for (int attempt = 1; attempt <= 5; attempt++) {
        sendCommand(cmd_memo, sizeof(cmd_memo));
        delayMicroseconds(attempt * 500);
        response_memo = inputMonitorWithReturn(2, false, 200000UL);
        if (response_memo.indexOf("71,") >= 0) {
            break;
        }
        delay(100);
    }

    Serial.print("QueryDiscMemoryInfo (MEMO check) Response: ");
    Serial.println(response_memo);

    if (response_memo.indexOf("71,") >= 0) {
        response_memo.replace(",", " ");
        response_memo.trim();
        int start = response_memo.indexOf("71");
        start = response_memo.indexOf(" ", start) + 1;
        int end = response_memo.indexOf(" ", start);
        if (end == -1) end = response_memo.length();

        String hexByte = response_memo.substring(start, end);
        if (hexByte.length() > 0) {
            long byteVal = strtol(hexByte.c_str(), NULL, 16);
            if (byteVal & 0x01) {
                Serial.println("Disc has MEMO");
            }
        }
    }

    // --- Check for CDTEXT using 0x98 command ---
    uint8_t cmd_cdtext[] = {device, 0x98, discByte};
    String response_cdtext;
    for (int attempt = 1; attempt <= 5; attempt++) {
        sendCommand(cmd_cdtext, sizeof(cmd_cdtext));
        delayMicroseconds(attempt * 500);
        response_cdtext = inputMonitorWithReturn(2, false, 200000UL);
        if (response_cdtext.indexOf("98,") >= 0) { // Wait for a valid response from device 98
            break;
        }
        delay(100);
    }

    Serial.print("QueryDiscMemoryInfo (CDTEXT check) Response: ");
    Serial.println(response_cdtext);

    response_cdtext.toUpperCase(); // Use uppercase for case-insensitive check
    if (response_cdtext.indexOf("98,E,") == -1 && response_cdtext.indexOf("98,") >= 0) {
        Serial.println("Disc has CDTEXT");
    }
}

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
