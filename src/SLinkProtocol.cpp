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

SLinkProtocol* SLinkProtocol::_instance = nullptr;

SLinkProtocol::SLinkProtocol(uint8_t pin) : _pin(pin), _commandCount(0), _currentCommand(nullptr)
{
    memset(_titleBuffer, 0, sizeof(_titleBuffer));
    _instance = this;
}

void SLinkProtocol::begin()
{
    pinMode(_pin, INPUT);
    attachListener();
}

void SLinkProtocol::process()
{
    drainBusCapture();

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
    uint8_t discByte = encodeDiscByte(_currentCommand->disc > 200 ? _currentCommand->disc - 200 : _currentCommand->disc);

    // The reply we want is the frame "98 40 <disc> <13 title bytes>". The
    // capture window also picks up unsolicited status frames from the player
    // (e.g. "92 22"), so we locate this exact header and parse only the bytes
    // that follow it, up to the end of that frame. inputMonitorWithReturn
    // renders each frame as "\nSTART,<hex>,<hex>,...\n" with unpadded
    // lowercase hex, so the header for disc 7 is "98,40,7,".
    String frameHeader = String("98,40,") + String(discByte, HEX) + ",";

    // "98 10" is the player's explicit "no memo" answer. It is sent both for
    // an empty slot and for a loaded disc with neither memo nor CD TEXT (the
    // player offers no way to tell those apart; verified with the sniffer),
    // so it means "this slot has no title" and the stored title is blanked.
    // Only a genuine lack of any reply yields the "Timeout" sentinel, which
    // leaves EEPROM untouched.
    static const char *noMemoHeader = "98,10,";

    String response;
    int frameIdx = -1;
    bool noMemo = false;
    for (int attempt = 1; attempt <= 5; attempt++)
    {
        uint8_t cmd[] = {device, (uint8_t)CMD_GET_DISC_TITLE, discByte};
        sendCommand(cmd, sizeof(cmd));
        delayMicroseconds(attempt * 500); // 0.5ms, 1ms, 1.5ms...

        response = inputMonitorWithReturn(2, false, 200000UL); // 0.2 seconds

        frameIdx = response.indexOf(frameHeader);
        noMemo = response.indexOf(noMemoHeader) >= 0;
        if (frameIdx >= 0 || noMemo)
        {
            break;
        }
        delay(100);
    }

    Serial.print("GetDiscTitle Response: ");
    Serial.println(response);

    if (frameIdx < 0 && noMemo)
    {
        Serial.println("Player reports no memo for this slot (empty, or disc without memo/CD TEXT).");
        _titleBuffer[0] = '\0';
    }
    else if (frameIdx >= 0)
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
    uint8_t discByte = encodeDiscByte(_currentCommand->disc > 200 ? _currentCommand->disc - 200 : _currentCommand->disc);
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
    // The pin is always left as INPUT between transmissions, so no pinMode()
    // is needed before waiting for the bus. (On the RA4M1, pinMode() rewrites
    // the pin function register and silently drops the edge interrupt, which
    // is why the listener is detached here and re-attached below.)
    _lineReady();
    detachListener();
    Start = micros();
    pinMode(_pin, OUTPUT);
    _writeSync();
    for (size_t i = 0; i < length; ++i)
    {
        _writeByte(command[i]);
    }
    pinMode(_pin, INPUT);
    attachListener(); // Re-arm before the reply can start
    do
    {
        delayMicroseconds(SLINK_LOOP_DELAY);
        if (digitalRead(_pin) == LOW)
            break;
    } while (micros() - Start < SLINK_WORD_DELIMITER);
}

// ---------------------------------------------------------------------------
// Passive bus listener
//
// Frames the player sends on its own (verified on a CDP-CX355, matching
// BigDave's 1998 CDP-CX250 notes; first byte 0x98 = "from CD player 1"):
//   98 00 / 01 / 02            play / stop / pause (also acks our commands)
//   98 50 DD TT MM SS          track started: disc, track, length
//   98 0C                      29 seconds left in the track
//   98 52 DD / 58 DD           displaying / loaded disc DD
//   98 70 00 SS 00 DD 00       reply to query 0F: mode flags, current disc
//   98 61 cc xx, 98 08         model id and "ready" after power on
//   98 2E / 2F                 power on / power off
//   98 0E                      reply: command rejected ("power is off"); also
//                              sent for a memo write to a CD TEXT disc
// Disc byte: 1-99 BCD, 100-200 = value - 54 (0x9A..0xFE). Track and time BCD.
// ---------------------------------------------------------------------------

void SLinkProtocol::attachListener()
{
    _pulseHead = _pulseTail = 0;
    _rxInFrame = false;
    _rxLen = _rxBits = 0;
    attachInterrupt(digitalPinToInterrupt(_pin), onEdgeISR, CHANGE);
}

void SLinkProtocol::detachListener()
{
    detachInterrupt(digitalPinToInterrupt(_pin));
}

void SLinkProtocol::onEdgeISR()
{
    if (_instance)
        _instance->handleEdge();
}

// Runs in interrupt context: record the LOW pulse width on each rising edge,
// and a FRAME_GAP marker when a falling edge follows a long idle period.
void SLinkProtocol::handleEdge()
{
    uint32_t now = micros();
    uint8_t next = _pulseHead + 1;
    if (next == _pulseTail)
        return; // Buffer full; drop the edge rather than overwrite

    if (digitalRead(_pin) == LOW)
    {
        if (now - _lastRiseUs > FRAME_GAP_US)
        {
            _pulses[_pulseHead] = FRAME_GAP;
            _pulseHead = next;
        }
        _lowStartUs = now;
    }
    else
    {
        uint32_t width = now - _lowStartUs;
        _pulses[_pulseHead] = width > 0xFFFE ? 0xFFFE : (uint16_t)width;
        _pulseHead = next;
        _lastRiseUs = now;
    }
}

static inline bool nearMark(uint16_t width, uint16_t mark)
{
    return width > mark / SLINK_MARK_RANGE && width < mark * SLINK_MARK_RANGE;
}

// Called from process(): turn buffered pulse widths into frames.
void SLinkProtocol::drainBusCapture()
{
    while (_pulseTail != _pulseHead)
    {
        uint16_t width = _pulses[_pulseTail];
        _pulseTail = _pulseTail + 1;

        if (width == FRAME_GAP)
        {
            finishBusFrame();
        }
        else if (nearMark(width, SLINK_MARK_SYNC))
        {
            finishBusFrame();
            _rxInFrame = true;
        }
        else if (!_rxInFrame)
        {
            // Mid-frame join (e.g. we re-attached during a frame): skip to next sync
        }
        else if (nearMark(width, SLINK_MARK_ONE) || nearMark(width, SLINK_MARK_ZERO))
        {
            _rxByte = (_rxByte << 1) | (nearMark(width, SLINK_MARK_ONE) ? 1 : 0);
            if (++_rxBits == 8)
            {
                if (_rxLen < (int)sizeof(_rxFrame))
                    _rxFrame[_rxLen++] = _rxByte;
                _rxByte = 0;
                _rxBits = 0;
            }
        }
        else
        {
            _rxInFrame = false; // Unrecognised pulse: discard this frame
            _rxLen = _rxBits = 0;
        }
    }

    // The last frame on the bus is only closed by the next falling edge, so
    // also close it once the line has been quiet long enough.
    if (_rxInFrame && micros() - _lastRiseUs > FRAME_GAP_US)
        finishBusFrame();
}

void SLinkProtocol::finishBusFrame()
{
    if (_rxInFrame && _rxLen > 0)
        handleBusFrame(_rxFrame, _rxLen);
    _rxInFrame = false;
    _rxLen = _rxBits = 0;
    _rxByte = 0;
}

uint16_t SLinkProtocol::decodeDiscByte(uint8_t b)
{
    uint8_t hi = b >> 4, lo = b & 0x0F;
    return hi > 9 ? (uint16_t)b - 54 : hi * 10 + lo;
}

static inline uint8_t fromBCD(uint8_t b)
{
    return (b >> 4) * 10 + (b & 0x0F);
}

void SLinkProtocol::handleBusFrame(const uint8_t *frame, int len)
{
    if (len < 2 || frame[0] != 0x98)
        return;

    PlaybackStatus before = _status;
    switch (frame[1])
    {
    case 0x00: _status.state = PlayState::Playing; break;
    case 0x01: _status.state = PlayState::Stopped; break;
    case 0x02: _status.state = PlayState::Paused; break;
    case 0x0C: _status.state = PlayState::Playing; break; // Only sent while playing
    case 0x2E: _status.state = PlayState::Stopped; break; // Just powered on, idle
    case 0x2F: _status.state = PlayState::Off; break;
    // 0x0E ("power is off") is a reply to a rejected command, not a broadcast:
    // a powered-on player also answers 0E to a memo write on a CD TEXT disc.
    // It is deliberately not treated as a power-state change.
    case 0x50:
        if (len < 4) return;
        _status.state = PlayState::Playing;
        _status.disc = decodeDiscByte(frame[2]);
        _status.track = fromBCD(frame[3]);
        if (len >= 6)
        {
            _status.lengthMin = fromBCD(frame[4]);
            _status.lengthSec = fromBCD(frame[5]);
        }
        break;
    case 0x52:
    case 0x58:
    case 0x70:
    {
        // Current disc without track info. The carousel takes several seconds
        // to load a new disc before the first "50" frame arrives, so forget
        // the previous disc's track rather than show it against the new disc.
        int discIdx = frame[1] == 0x70 ? 4 : 2;
        if (len <= discIdx) return;
        uint16_t disc = decodeDiscByte(frame[discIdx]);
        if (disc != _status.disc)
        {
            _status.track = 0;
            _status.lengthMin = _status.lengthSec = 0;
        }
        _status.disc = disc;
        break;
    }
    default:
        return; // Not a status frame we track
    }
    _status.updatedMs = millis();

    if (before.state != _status.state || before.disc != _status.disc || before.track != _status.track)
    {
        static const char *names[] = {"unknown", "off", "stopped", "paused", "playing"};
        Serial.print("Player status: ");
        Serial.print(names[(int)_status.state]);
        Serial.print(" disc ");
        Serial.print(_status.disc);
        Serial.print(" track ");
        Serial.println(_status.track);
    }
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

// Encode a disc number within one address range (1-200 for 0x90, 1-100 for
// 0x93) the way the player expects. Discs 1-99 are BCD, but 100-200 are the
// byte value minus 54 (100 -> 0x9A, 115 -> 0xA9, 200 -> 0xFE), so plain BCD
// would address the wrong slot: BCD 115 is 0xB5, which the player reads as
// disc 127. Inverse of decodeDiscByte(). Verified on hardware with disc 115.
uint8_t SLinkProtocol::encodeDiscByte(int disc)
{
    return disc >= 100 ? (uint8_t)(disc + 54) : toBCD(disc);
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
    uint8_t discByte = encodeDiscByte(disc > 200 ? disc - 200 : disc);

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
    if (response.indexOf("98,E,") >= 0) {
        // Seen on hardware when the disc carries CD TEXT: the player keeps
        // the disc's own title and refuses the memo.
        Serial.println("ERROR: Player rejected the memo write (reply 0E). CD TEXT discs cannot be retitled.");
    } else if (response.indexOf("1B") >= 0) {
        Serial.println("ERROR: Text write command failed (Received 1B error)");
    } else {
        // No error reply is not proof of success: while playing, the player
        // ignores a memo write for a CD TEXT disc without answering at all.
        // The getDiscTitle queued after this call reads back what was stored.
        Serial.println("Memo write sent with no error reply; the read-back decides what is stored.");
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
