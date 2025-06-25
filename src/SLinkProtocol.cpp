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

SLinkProtocol::SLinkProtocol(uint8_t pin) : _pin(pin), _commandCount(0), _currentCommand(nullptr) {
    memset(_titleBuffer, 0, sizeof(_titleBuffer));
}

void SLinkProtocol::begin() {
    pinMode(_pin, INPUT);
}

void SLinkProtocol::process() {
    if (_currentCommand) {
        Serial.print("Processing command: 0x");
        Serial.print(_currentCommand->command, HEX);
        Serial.print(" in state: ");
        Serial.println(_currentCommand->state);

        if (_currentCommand->timeout > 0 && millis() > _currentCommand->timeout) {
            Serial.println("Command timed out");
            _currentCommand = nullptr;
            executeNextCommand();
            return;
        }

        switch (_currentCommand->command) {
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
    } else {
        executeNextCommand();
    }
}

bool SLinkProtocol::getDiscTitle(int disc, SLinkCallback callback, void* userData) {
    if (_commandCount >= MAX_COMMANDS) return false;
    addCommand(CMD_GET_DISC_TITLE, disc, 0, callback, userData);
    return true;
}

bool SLinkProtocol::play() {
    if (_commandCount >= MAX_COMMANDS) return false;
    addCommand(CMD_PLAY, 0, 0, nullptr, nullptr);
    return true;
}

bool SLinkProtocol::stop() {
    if (_commandCount >= MAX_COMMANDS) return false;
    addCommand(CMD_STOP, 0, 0, nullptr, nullptr);
    return true;
}

bool SLinkProtocol::pause() {
    if (_commandCount >= MAX_COMMANDS) return false;
    addCommand(CMD_PAUSE, 0, 0, nullptr, nullptr);
    return true;
}

bool SLinkProtocol::nextTrack() {
    if (_commandCount >= MAX_COMMANDS) return false;
    addCommand(CMD_NEXT_TRACK, 0, 0, nullptr, nullptr);
    return true;
}

bool SLinkProtocol::prevTrack() {
    if (_commandCount >= MAX_COMMANDS) return false;
    addCommand(CMD_PREV_TRACK, 0, 0, nullptr, nullptr);
    return true;
}

bool SLinkProtocol::selectDisc(int disc) {
    if (_commandCount >= MAX_COMMANDS) return false;
    addCommand(CMD_SELECT_DISC, disc, 1, nullptr, nullptr); // Default to track 1
    return true;
}

bool SLinkProtocol::powerOn() {
    if (_commandCount >= MAX_COMMANDS) return false;
    addCommand(CMD_POWER_ON, 0, 0, nullptr, nullptr);
    return true;
}

bool SLinkProtocol::powerOff() {
    if (_commandCount >= MAX_COMMANDS) return false;
    addCommand(CMD_POWER_OFF, 0, 0, nullptr, nullptr);
    return true;
}

const char* SLinkProtocol::getTitle() const {
    return _titleBuffer;
}

void SLinkProtocol::processGetDiscTitle() {
    // This requires reading from the S-Link bus, which is not yet implemented.
    // For now, we'll just mark the command as complete.
    if (_currentCommand->callback) {
        _currentCommand->callback(this, _currentCommand->userData);
    }
    _currentCommand = nullptr;
    executeNextCommand();
}

void SLinkProtocol::processSimpleCommand() {
    uint8_t cmd[] = {0x90, (uint8_t)_currentCommand->command};
    sendCommand(cmd, sizeof(cmd));
    if (_currentCommand->callback) {
        _currentCommand->callback(this, _currentCommand->userData);
    }
    _currentCommand = nullptr;
    executeNextCommand();
}

void SLinkProtocol::processSelectDisc() {
    uint8_t device = (_currentCommand->disc > 200) ? 0x93 : 0x90;
    uint8_t discByte = toBCD(_currentCommand->disc > 200 ? _currentCommand->disc - 200 : _currentCommand->disc);
    uint8_t trackByte = toBCD(_currentCommand->track);
    uint8_t cmd[] = {device, (uint8_t)CMD_SELECT_DISC, discByte, trackByte};
    sendCommand(cmd, sizeof(cmd));
    if (_currentCommand->callback) {
        _currentCommand->callback(this, _currentCommand->userData);
    }
    _currentCommand = nullptr;
    executeNextCommand();
}

void SLinkProtocol::_lineReady() {
    unsigned long Start = micros();
    unsigned long beginTimeout = Start;
    do {
        delayMicroseconds(SLINK_LOOP_DELAY);
        if (digitalRead(_pin) == LOW)
            Start = micros();
    } while ((micros() - Start < SLINK_LINE_READY) && (micros() - beginTimeout < SLINK_LOOP_TIMEOUT));
}

void SLinkProtocol::_writeSync() {
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    delayMicroseconds(SLINK_MARK_SYNC);
    digitalWrite(_pin, HIGH);
    delayMicroseconds(SLINK_MARK_DELIMITER);
}

void SLinkProtocol::_writeByte(byte value) {
    pinMode(_pin, OUTPUT);
    for (int i = 7; i >= 0; i--) {
        if (value & 1 << i) {
            digitalWrite(_pin, LOW);
            delayMicroseconds(SLINK_MARK_ONE);
            digitalWrite(_pin, HIGH);
            delayMicroseconds(SLINK_MARK_DELIMITER);
        } else {
            digitalWrite(_pin, LOW);
            delayMicroseconds(SLINK_MARK_ZERO);
            digitalWrite(_pin, HIGH);
            delayMicroseconds(SLINK_MARK_DELIMITER);
        }
    }
}

void SLinkProtocol::sendCommand(const uint8_t* command, size_t length) {
    unsigned long Start;
    pinMode(_pin, INPUT);
    _lineReady();
    Start = micros();
    pinMode(_pin, OUTPUT);
    _writeSync();
    for (size_t i = 0; i < length; ++i) {
        _writeByte(command[i]);
    }
    pinMode(_pin, INPUT);
    do {
        delayMicroseconds(SLINK_LOOP_DELAY);
        if (digitalRead(_pin) == LOW)
            break;
    } while (micros() - Start < SLINK_WORD_DELIMITER);
}

int SLinkProtocol::readResponse(uint8_t* buffer, size_t length, unsigned long timeout) {
    // Not implemented yet
    return 0;
}

uint8_t SLinkProtocol::toBCD(int val) {
    return ((val / 10) << 4) | (val % 10);
}

void SLinkProtocol::addCommand(SLinkCommandType command, int disc, int track, SLinkCallback callback, void* userData, void* data) {
    if (_commandCount < MAX_COMMANDS) {
        Serial.print("Adding command to queue: 0x");
        Serial.println(command, HEX);
        SLinkCommand& cmd = _commandQueue[_commandCount++];
        cmd.command = command;
        cmd.disc = disc;
        cmd.track = track;
        cmd.state = 0;
        cmd.timeout = 0;
        cmd.data = data;
        cmd.callback = callback;
        cmd.userData = userData;
    } else {
        Serial.println("Command queue full!");
    }
}

void SLinkProtocol::executeNextCommand() {
    if (!_currentCommand && _commandCount > 0) {
        Serial.println("Executing next command from queue.");
        _currentCommand = &_commandQueue[0];
        for (int i = 0; i < _commandCount - 1; ++i) {
            _commandQueue[i] = _commandQueue[i + 1];
        }
        _commandCount--;
    }
}