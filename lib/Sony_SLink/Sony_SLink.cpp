/*
  Arduino SONY S-LINK/Control-A1 Protocol Library

  https://github.com/Ircama/Sony_SLink.git

  (C) Ircama, 2017, CC-BY-SA 4.0
  https://creativecommons.org/licenses/by-sa/4.0/


  Code heavily based on
  - hajdbo's code:
    https://github.com/hajdbo/sony-jukebox-slink/blob/master/jukebox.pde
  - EdsterG's code:
    https://github.com/EdsterG/LANC_Library

  Reference documents:
  - Sony S-Link gateway document:
    http://ee.bradley.edu/projects/proj2001/sonyntwk/SLink.PDF

  - How SONY's S-LINK/CTRL-A(II) protocol works (waveform):
    http://boehmel.de/slink.htm

  - Codes for Sony STR-DA50ES receiver:
    http://www.hifi-remote.com/sony/Sony_dsp_192.htm

  - S-Link Parallel Device for Linux
    http://web.archive.org/web/20040722060841/http://www.undeadscientist.com/slink/index.html

  - Reverse Engineered Control-A1 codes (Using the Sony CDP-CX250) Written by BigDave (2/19/98)
    http://web.archive.org/web/19981202175957/http://www.cc.gatech.edu/people/home/bigdave/cdplayer/control-a1.txt

  - Working S-Link code - Background of the S-link protocol
    http://forums.parallax.com/discussion/70973/working-s-link-code-long

  - Slink Send and Receive Commands for Sony MiniDisc Recorders
    http://web.archive.org/web/20030419100153/http://web2.airmail.net/will/mdslink.txt

  - Jukebox Interface Guide
    http://web.archive.org/web/20030414200645/http://www.upl.cs.wisc.edu/~orn/jukebox/guide.html

  - Original Control-A1 document
    http://web.archive.org/web/20030414231523/http://www.upl.cs.wisc.edu/~orn/jukebox/controla1.html

  Tested with a Sony STR-DA50ES receiver/amplifier.
  Service manual: http://sportsbil.com/sony/STR/STR-D/STR-DA30ES_DA50ES_V55ES_v1.1.pdf

  This protocol is very slow: average 355 bps half duplex for a standard two-byte send transmission taking 45 millisecs (355=16/0,045).

  Feel free to share this source code, but include explicit mention to the author.
  Licensed under creative commons - see http://creativecommons.org/licenses/by-sa/3.0/

*/

#include "Sony_SLink.h"
// #include <util/atomic.h>

#define bitWidth 3000 // Default bit width (in micro seconds)

/* Initialize protocol */

void Slink::init(int slinkPin)
{
    _slinkPin = slinkPin;
    pinMode(_slinkPin, INPUT); // Define pin mode INPUT for slinkPin.
                               // Will be changed to output when writing.

    /*
     * Notice that activating the internal pullup resistor is not needed,
     * as S-LINK is already pulled up to +5.1V with a 5kOhm resistor (ref.
     * Sony STR-DA50ES; other devices might have sightly different pull-up
     * resistors and voltage). Besides, S-LINK is a bidirectional protocol.
     * Do not forget a diode and a 220 Ohm resistor between slinkPin and
     * the Control-A1 jack, to limit the maximum current drawn by the
     * microcontroller in case of transmission collision.
     */
}

// We advise to issue every command twice, in case the first time collided
// with an incoming response sequence from the other device

void Slink::sendCommand(unsigned int deviceId, unsigned int commandId1, int commandId2, int commandId3)
{
    unsigned long Start;

    pinMode(_slinkPin, INPUT);
    _lineReady(); // Check line availability to write a frame
    Start = micros();
    pinMode(_slinkPin, OUTPUT); // Change pin mode to OUTPUT for slinkPin.
    _writeSync();               // Beginning of new transmission
    _writeByte(deviceId);       // Select device (one byte)
    _writeByte(commandId1);     // Send the actual Command-Code (one byte)
    if (commandId2 >= 0)
        _writeByte(commandId2); // Send the actual Command-Code (one byte)
    if (commandId3 >= 0)
        _writeByte(commandId3); // Send an additional Command-Code (one byte)
    pinMode(_slinkPin, INPUT);  // Return to INPUT
    do
    { // The command sequence must be padded (by 5V) at the end to make it 45 millisec. long
        delayMicroseconds(SLINK_LOOP_DELAY);
        if (digitalRead(_slinkPin) == LOW)
            break; // break padding if the other device starts transmitting meanwhile
    } //  (but the padding rule is apparently not very strict)
    while (micros() - Start < SLINK_WORD_DELIMITER);
}

/* Check line availability before transmitting or receiving

 * S-Link has no bus arbitration logic. Messages can
 * be sent at any time by any participant. Bus
 * collision may occur. Because there is not much
 * traffic, this happens extremely rarely. Checking the
 * receiver state immediately before starting a new
 * transmission prevents from collisions in nearly all cases.
 */

void Slink::_lineReady()
{
    unsigned long Start = micros();
    unsigned long beginTimeout = Start;

    // Search for a 3ms=3000uSec gap (at least), being HIGH continuosly.

    do
    {
        delayMicroseconds(SLINK_LOOP_DELAY);
        if (digitalRead(_slinkPin) == LOW)
            Start = micros(); // reset the loop each time traffic is detected
    } while ((micros() - Start < SLINK_LINE_READY) && (micros() - beginTimeout < SLINK_LOOP_TIMEOUT));
}

/*
 * Synchronisation:
 * holding the line low for SLINK_MARK_SYNC uS indicates the beginning of new transmission
 */
void Slink::_writeSync()
{
    pinMode(_slinkPin, OUTPUT);              // Change pin mode to OUTPUT for slinkPin.
    digitalWrite(_slinkPin, LOW);            // start sync (line low)
    delayMicroseconds(SLINK_MARK_SYNC);      // keep line low for SLINK_MARK_SYNC uS
    digitalWrite(_slinkPin, HIGH);           // release the default HIGH state
    delayMicroseconds(SLINK_MARK_DELIMITER); // Delimiter (between Sync, Ones and Zeros, the line must be released (return to high level) for SLINK_MARK_DELIMITER uS)
}

//-------------------------------------------------------------------------------------

void Slink::_writeByte(byte value)
{
    /*
     * Zero bit: holding the line low for SLINK_MARK_ZERO uS indicates a 0
     * One bit: holding the line low for SLINK_MARK_ONE uS indicates a 1
     * Delimiter between Sync, Ones and Zeros, the line must be released (return to high level) for SLINK_MARK_DELIMITER uS
     */

    pinMode(_slinkPin, OUTPUT); // Change pin mode to OUTPUT for slinkPin.
    for (int i = 7; i >= 0; i--)
    {
        if (value & 1 << i)
        {
            digitalWrite(_slinkPin, LOW);
            delayMicroseconds(SLINK_MARK_ONE);
            digitalWrite(_slinkPin, HIGH);
            delayMicroseconds(SLINK_MARK_DELIMITER);
        }
        else
        {
            digitalWrite(_slinkPin, LOW);
            delayMicroseconds(SLINK_MARK_ZERO);
            digitalWrite(_slinkPin, HIGH);
            delayMicroseconds(SLINK_MARK_DELIMITER);
        }
    }
}

//-------------------------------------------------------------------------------------

int Slink::pin()
{
    return _slinkPin;
}

//-------------------------------------------------------------------------------------

// Read, dump and debug S-Link protocol
// This function is for debugging purpose and is valid for ATMEL devices provided with serial interface,
// which is used to output the monitoring information related to S-Link timings and data, read from the input port.
#if !defined(__AVR_ATtiny85__)
void Slink::inputMonitor(int type,     // 0=measure timing, 1=decode and dump binary & hex, 2=decode and dump hex
                         boolean idle, // false(default)=measure mark (=data), true=measure idle timing (=delimiters)
                         // for S-Link protocol, marks can return sync (2400usec), zero (600usec), one (1200usec)
                         // idle=true can only be used with type=0 and timing should always return 600 usec (delimiter)
                         unsigned long uSecTimeout, // monitoring timeout; defaults to 10 seconds
                         // information read from S-Link is buffered until timeout and then dumped to the serial port
                         unsigned long serialSpeed // baud rate of the serial port (defaults to 115.2kbps)
)
{
    unsigned long value = 0;
    unsigned long Start = micros();
    int nl = 0;
    int count = 0;
    int byte = 0;
    String buffer = "";

    Serial.begin(serialSpeed);
    Serial.println("Start monitor");
    do
    {
        value = pulseIn(_slinkPin, idle, 3000UL); // timeout to 3 milliseconds=3000 uSec
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
    Serial.println(buffer);
    Serial.println("End monitor");
}

void Slink::sendLongCommand(uint8_t deviceId, const uint8_t *data, size_t length)
{
    // Make sure the bus is idle:
    pinMode(_slinkPin, INPUT);
    _lineReady();

    unsigned long start = micros();

    // Begin driving the S-Link line:
    pinMode(_slinkPin, OUTPUT);
    _writeSync();         // Start Sync
    _writeByte(deviceId); // First byte: which device we’re talking to

    // Now write all bytes in 'data':
    for (size_t i = 0; i < length; i++)
    {
        _writeByte(data[i]);
    }

    // Release the line back to INPUT:
    pinMode(_slinkPin, INPUT);

    // As per the library’s approach, pad the command up to ~45ms
    // unless the device starts talking sooner (LOW).
    do
    {
        delayMicroseconds(SLINK_LOOP_DELAY);
        if (digitalRead(_slinkPin) == LOW)
        {
            // The device is responding, so break out of the padding loop.
            break;
        }
    } while (micros() - start < SLINK_WORD_DELIMITER);
}

// Read a single bit timing from the S-Link line
unsigned long Slink::_readBitTiming()
{
    return pulseIn(_slinkPin, LOW, 5000UL); // Wait for LOW pulse, timeout 5ms
}

// Wait for sync pattern (2400us LOW pulse) - improved version
bool Slink::_waitForSync(unsigned long timeoutMs)
{
    unsigned long startTime = millis();
    
    Serial.println("DEBUG: _waitForSync starting...");
    
    while (millis() - startTime < timeoutMs)
    {
        // Use the same approach as inputMonitor - wait for LOW pulse
        unsigned long timing = pulseIn(_slinkPin, LOW, 5000UL); // 5ms timeout per pulse
        
        if (timing > 0) // Got a pulse
        {
            Serial.print("DEBUG: Detected pulse: ");
            Serial.print(timing);
            Serial.println("us");
            
            // Check if timing matches sync pattern (2400us ± tolerance)
            if (timing > (SLINK_MARK_SYNC / SLINK_MARK_RANGE) && 
                timing < (SLINK_MARK_SYNC * SLINK_MARK_RANGE))
            {
                Serial.println("DEBUG: SYNC PATTERN FOUND!");
                return true;
            }
            else
            {
                Serial.println("DEBUG: Pulse too short/long for sync");
            }
        }
        
        // Small delay if no pulse detected
        if (timing == 0)
        {
            delayMicroseconds(100);
        }
    }
    
    Serial.println("DEBUG: _waitForSync timeout");
    return false;
}

// Read a byte from S-Link line - improved version
uint8_t Slink::_readByte(unsigned long timeoutMs)
{
    uint8_t result = 0;
    unsigned long startTime = millis();
    
    Serial.print("DEBUG: Reading byte... ");
    
    for (int bit = 7; bit >= 0; bit--)
    {
        if (millis() - startTime > timeoutMs)
        {
            Serial.println("TIMEOUT");
            return 0; // Timeout
        }
            
        // Use direct pulseIn like inputMonitor
        unsigned long timing = pulseIn(_slinkPin, LOW, 5000UL);
        
        if (timing == 0)
        {
            Serial.println("NO_PULSE");
            return 0; // Timeout or error
        }
            
        // Determine if it's a 1 or 0 based on timing
        if (timing > (SLINK_MARK_ONE / SLINK_MARK_RANGE) && 
            timing < (SLINK_MARK_ONE * SLINK_MARK_RANGE))
        {
            result |= (1 << bit); // Set bit for '1'
            Serial.print("1");
        }
        else if (timing > (SLINK_MARK_ZERO / SLINK_MARK_RANGE) && 
                 timing < (SLINK_MARK_ZERO * SLINK_MARK_RANGE))
        {
            // Bit is already 0, no action needed
            Serial.print("0");
        }
        else
        {
            Serial.print("INVALID(");
            Serial.print(timing);
            Serial.print("us)");
            return 0; // Invalid timing
        }
    }
    
    Serial.print(" = 0x");
    Serial.println(result, HEX);
    return result;
}

// Receive a complete S-Link response with enhanced debugging
String Slink::receiveResponse(unsigned long timeoutMs)
{
    pinMode(_slinkPin, INPUT);
    String response = "";
    unsigned long startTime = millis();
    int byteCount = 0;
    
    Serial.print("DEBUG: Starting receive, timeout=");
    Serial.print(timeoutMs);
    Serial.println("ms");
    
    // Wait for sync pattern
    Serial.println("DEBUG: Waiting for sync pattern...");
    if (!_waitForSync(timeoutMs))
    {
        Serial.println("DEBUG: No sync pattern found");
        return ""; // No sync found
    }
    
    Serial.println("DEBUG: Sync found! Reading bytes...");
    
    // Read bytes until timeout or transmission ends
    while (millis() - startTime < timeoutMs)
    {
        uint8_t byte = _readByte(1000); // 1 second timeout per byte
        
        if (byte == 0)
        {
            Serial.print("DEBUG: Got zero byte, checking if line idle... byteCount=");
            Serial.println(byteCount);
            
            // Check if line is idle (no more data)
            unsigned long idleStart = millis();
            bool lineIdle = true;
            
            while (millis() - idleStart < 200) // 200ms idle check
            {
                if (digitalRead(_slinkPin) == LOW)
                {
                    lineIdle = false;
                    break;
                }
                delayMicroseconds(100);
            }
            
            if (lineIdle)
            {
                Serial.println("DEBUG: Line is idle, ending reception");
                break; // End of transmission
            }
            else
            {
                Serial.println("DEBUG: Line not idle, continuing...");
            }
        }
        else
        {
            byteCount++;
            Serial.print("DEBUG: Byte ");
            Serial.print(byteCount);
            Serial.print(": 0x");
            Serial.println(byte, HEX);
            
            response += String(byte, HEX) + " ";
        }
    }
    
    Serial.print("DEBUG: Received ");
    Serial.print(byteCount);
    Serial.println(" bytes total");
    
    response.trim();
    return response;
}

// Receive title response specifically (handles ASCII text after command bytes)
String Slink::receiveTitle(unsigned long timeoutMs)
{
    pinMode(_slinkPin, INPUT);
    String title = "";
    unsigned long startTime = millis();
    bool inAsciiData = false;
    int byteCount = 0;
    
    // Wait for sync pattern
    if (!_waitForSync(timeoutMs))
    {
        return ""; // No sync found
    }
    
    // Read the response
    while (millis() - startTime < timeoutMs)
    {
        uint8_t byte = _readByte(1000); // 1 second timeout per byte
        
        if (byte == 0)
        {
            // Check if line is idle
            unsigned long idleStart = millis();
            bool lineIdle = true;
            
            while (millis() - idleStart < 200) // 200ms idle check
            {
                if (digitalRead(_slinkPin) == LOW)
                {
                    lineIdle = false;
                    break;
                }
                delayMicroseconds(100);
            }
            
            if (lineIdle)
                break; // End of transmission
        }
        else
        {
            byteCount++;
            
            // First byte should be device ID (0x98-0x9A)
            // Second byte should be command (0x40)
            // Third byte should be disc number
            // From fourth byte onward should be ASCII title data
            if (byteCount >= 4)
            {
                inAsciiData = true;
            }
            
            if (inAsciiData && byte >= 0x20 && byte <= 0x7E)
            {
                // Valid ASCII printable character
                title += (char)byte;
            }
        }
    }
    
    title.trim();
    return title;
}

// Send command and immediately listen for response (no padding delay)
String Slink::sendCommandAndReceive(unsigned int deviceId, unsigned int commandId1, int commandId2, unsigned long timeoutMs)
{
    Serial.println("DEBUG: sendCommandAndReceive starting...");
    
    pinMode(_slinkPin, INPUT);
    _lineReady(); // Check line availability to write a frame
    
    Serial.println("DEBUG: Sending command...");
    pinMode(_slinkPin, OUTPUT); // Change pin mode to OUTPUT for slinkPin.
    _writeSync();               // Beginning of new transmission
    _writeByte(deviceId);       // Select device (one byte)
    _writeByte(commandId1);     // Send the actual Command-Code (one byte)
    if (commandId2 >= 0)
        _writeByte(commandId2); // Send additional Command-Code (one byte)
    
    // Immediately switch to INPUT and start listening (no padding delay)
    Serial.println("DEBUG: Command sent, switching to receive mode...");
    pinMode(_slinkPin, INPUT);
    
    // Small delay to let the device process the command
    delayMicroseconds(1000); // 1ms
    
    // Now listen for response
    Serial.println("DEBUG: Starting response reception...");
    return receiveResponse(timeoutMs);
}

// Send command and capture response using the working inputMonitor approach
String Slink::sendCommandAndCapture(unsigned int deviceId, unsigned int commandId1, int commandId2, unsigned long timeoutMs)
{
    Serial.println("DEBUG: sendCommandAndCapture starting...");
    
    // Send command first
    pinMode(_slinkPin, INPUT);
    _lineReady();
    pinMode(_slinkPin, OUTPUT);
    _writeSync();
    _writeByte(deviceId);
    _writeByte(commandId1);
    if (commandId2 >= 0)
        _writeByte(commandId2);
    pinMode(_slinkPin, INPUT);
    
    // Now capture response using same approach as inputMonitor
    Serial.println("DEBUG: Capturing response...");
    
    String response = "";
    unsigned long startTime = micros();
    int byteCount = 0;
    int byte = 0;
    bool inStartSequence = false;
    
    while (micros() - startTime < timeoutMs * 1000UL)
    {
        unsigned long timing = pulseIn(_slinkPin, LOW, 3000UL);
        
        if (timing == 0)
            continue; // No pulse, keep waiting
            
        // Use same timing logic as inputMonitor (type 2)
        if ((timing > (SLINK_MARK_SYNC / SLINK_MARK_RANGE)) && (timing < SLINK_MARK_SYNC * SLINK_MARK_RANGE))
        {
            // Sync pattern found - start of new transmission
            Serial.println("DEBUG: Found START pattern");
            response = "";
            byteCount = 0;
            byte = 0;
            inStartSequence = true;
        }
        else
        {
            if (inStartSequence)
            {
                // Process data bits using actual measured timings
                if ((timing > 500) && (timing < 700)) // ~567us = 0 bit
                {
                    // Zero bit
                }
                else if ((timing > 1000) && (timing < 1300)) // ~1130us = 1 bit  
                {
                    byte |= 128 >> byteCount;
                }
                else
                {
                    continue; // Invalid timing, skip
                }
                
                byteCount++;
                if (byteCount == 8)
                {
                    // Complete byte received
                    Serial.print("DEBUG: Captured byte: 0x");
                    Serial.println(byte, HEX);
                    response += String(byte, HEX) + " ";
                    byteCount = 0;
                    byte = 0;
                    
                    // Stop after reasonable number of bytes to avoid timeout
                    if (response.length() > 50) // ~16 bytes captured
                        break;
                }
            }
        }
    }
    
    Serial.print("DEBUG: Raw captured response: ");
    Serial.println(response);
    
    response.trim();
    return response;
}

// Send command and capture response using exact inputMonitor approach
String Slink::sendCommandAndMonitor(unsigned int deviceId, unsigned int commandId1, int commandId2)
{
    Serial.println("DEBUG: sendCommandAndMonitor starting...");
    
    // Send command first
    pinMode(_slinkPin, INPUT);
    _lineReady();
    pinMode(_slinkPin, OUTPUT);
    _writeSync();
    _writeByte(deviceId);
    _writeByte(commandId1);
    if (commandId2 >= 0)
        _writeByte(commandId2);
    pinMode(_slinkPin, INPUT);
    
    // No padding delay - immediately start monitoring
    Serial.println("DEBUG: Starting direct inputMonitor capture...");
    
    // Use the EXACT inputMonitor logic (type 2 = hex dump)
    String buffer = "";
    unsigned long value = 0;
    unsigned long startTime = micros();
    unsigned long timeoutUs = 3000000UL; // 3 seconds
    int count = 0;
    int byte = 0;
    
    while (micros() - startTime < timeoutUs)
    {
        value = pulseIn(_slinkPin, LOW, 3000UL); // timeout to 3 milliseconds
        
        if (value == 0)
        {
            // No pulse - check if we have any data collected
            if (buffer.length() > 0)
            {
                Serial.println("DEBUG: Timeout - ending capture");
                break;
            }
            continue;
        }
        
        // Use EXACT inputMonitor logic for type 2 (hex dump)
        if ((value > (SLINK_MARK_SYNC / SLINK_MARK_RANGE)) && (value < SLINK_MARK_SYNC * SLINK_MARK_RANGE))
        {
            // Sync pattern found
            Serial.println("DEBUG: SYNC found - starting byte capture");
            buffer = "";
            count = 0;
            byte = 0;
        }
        else
        {
            // Process data bits
            if ((value > SLINK_MARK_ONE / SLINK_MARK_RANGE) && (value < SLINK_MARK_ONE * SLINK_MARK_RANGE))
            {
                byte |= 128 >> count; // Set bit for '1'
            }
            // For '0' bits, no action needed (already 0)
            
            if (count++ == 7)
            {
                // Complete byte
                Serial.print("DEBUG: Complete byte: 0x");
                Serial.println(byte, HEX);
                buffer += String(byte, HEX) + ",";
                count = 0;
                byte = 0;
                
                // Stop after reasonable amount of data
                if (buffer.length() > 60)
                    break;
            }
        }
    }
    
    // Format response like inputMonitor output
    buffer.trim();
    if (buffer.endsWith(","))
        buffer.remove(buffer.length() - 1);
    buffer.replace(",", " ");
    
    Serial.print("DEBUG: sendCommandAndMonitor result: ");
    Serial.println(buffer);
    
    return buffer;
}

// Capture response using inputMonitor logic but return the result (modified from inputMonitor)
String Slink::captureInputMonitor(unsigned long uSecTimeout)
{
    unsigned long value = 0;
    unsigned long Start = micros();
    int count = 0;
    int byte = 0;
    String buffer = "";
    bool foundStart = false;
    
    Serial.println("DEBUG: captureInputMonitor starting...");
    
    do
    {
        value = pulseIn(_slinkPin, LOW, 3000UL); // timeout to 3 milliseconds
        
        if (value == 0)
        {
            // No pulse - continue waiting unless we already have data
            if (foundStart && buffer.length() > 0)
            {
                // If we found start and have some data, a timeout might mean end of transmission
                unsigned long idleTime = 0;
                unsigned long idleStart = micros();
                
                // Check for 200ms of idle time to confirm end
                while (micros() - idleStart < 200000UL)
                {
                    if (digitalRead(_slinkPin) == LOW)
                    {
                        idleTime = 0;
                        break;
                    }
                }
                
                if (micros() - idleStart >= 200000UL)
                {
                    Serial.println("DEBUG: End of transmission detected");
                    break;
                }
            }
            continue;
        }
        
        // Process pulse using exact inputMonitor logic (type 2 = hex)
        if ((value > (SLINK_MARK_SYNC / SLINK_MARK_RANGE)) && (value < SLINK_MARK_SYNC * SLINK_MARK_RANGE))
        {
            // Sync pattern found - start of new transmission
            Serial.println("DEBUG: START sync pattern found");
            buffer = "";
            count = 0;
            byte = 0;
            foundStart = true;
        }
        else if (foundStart)
        {
            // Process data bits
            if ((value > SLINK_MARK_ONE / SLINK_MARK_RANGE) && (value < SLINK_MARK_ONE * SLINK_MARK_RANGE))
            {
                byte |= 128 >> count; // Set bit for '1'
            }
            // For zero bits, no action needed (already 0)
            else if (!((value > SLINK_MARK_ZERO / SLINK_MARK_RANGE) && (value < SLINK_MARK_ZERO * SLINK_MARK_RANGE)))
            {
                // Invalid timing - might be noise, skip
                continue;
            }
            
            if (count++ == 7)
            {
                // Complete byte received
                Serial.print("DEBUG: Captured byte: 0x");
                Serial.println(byte, HEX);
                buffer += String(byte, HEX) + ",";
                count = 0;
                byte = 0;
                
                // Stop after reasonable amount of data
                if (buffer.length() > 80) // Allow more bytes for full title
                    break;
            }
        }
    } while (micros() - Start < uSecTimeout);
    
    // Clean up the response format
    buffer.trim();
    if (buffer.endsWith(","))
        buffer.remove(buffer.length() - 1);
    buffer.replace(",", " ");
    
    Serial.print("DEBUG: captureInputMonitor result: ");
    Serial.println(buffer);
    
    return buffer;
}

// Simple capture method that processes data bits directly without sync detection
String Slink::captureDataBits(unsigned long uSecTimeout)
{
    String buffer = "";
    unsigned long startTime = micros();
    int bitCount = 0;
    int byte = 0;
    int totalBits = 0;
    
    Serial.println("DEBUG: captureDataBits starting - processing bits directly...");
    
    while (micros() - startTime < uSecTimeout)
    {
        unsigned long timing = pulseIn(_slinkPin, LOW, 5000UL); // 5ms timeout per pulse
        
        if (timing == 0)
        {
            // No pulse - if we have collected some data, we might be done
            if (totalBits > 24) // At least 3 bytes (device, command, disc)
            {
                Serial.println("DEBUG: No more pulses, ending capture");
                break;
            }
            continue;
        }
        
        // Process the timing as data bits (skip sync detection)
        bool validBit = false;
        
        if (timing >= 500 && timing <= 700) // ~567us = 0 bit
        {
            // Zero bit - no action needed (byte already has 0)
            validBit = true;
            Serial.print("0");
        }
        else if (timing >= 1000 && timing <= 1300) // ~1135us = 1 bit
        {
            byte |= (1 << (7 - bitCount)); // Set bit for '1'
            validBit = true;
            Serial.print("1");
        }
        else
        {
            // Invalid timing, might be sync or noise
            Serial.print("?");
            continue;
        }
        
        if (validBit)
        {
            bitCount++;
            totalBits++;
            
            if (bitCount == 8)
            {
                // Complete byte
                Serial.print(" = 0x");
                Serial.print(byte, HEX);
                Serial.print(" ");
                
                buffer += String(byte, HEX) + " ";
                bitCount = 0;
                byte = 0;
                
                // Stop after reasonable amount of data
                if (buffer.length() > 80)
                    break;
            }
        }
    }
    
    buffer.trim();
    Serial.println();
    Serial.print("DEBUG: captureDataBits result: ");
    Serial.println(buffer);
    
    return buffer;
}

// Modified inputMonitor that returns the captured hex string instead of printing
String Slink::inputMonitorCapture(unsigned long uSecTimeout)
{
    unsigned long value = 0;
    unsigned long Start = micros();
    int count = 0;
    int byte = 0;
    String buffer = "";
    
    Serial.println("DEBUG: inputMonitorCapture starting (exact inputMonitor logic)...");
    
    do
    {
        value = pulseIn(_slinkPin, LOW, 3000UL); // timeout to 3 milliseconds=3000 uSec
        if (value == 0)
        {
            // No pulse - check if we have data and should end
            if (buffer.length() > 0)
            {
                // Add newline if we haven't added one
                if (!buffer.endsWith("\n"))
                {
                    buffer += String("\n");
                }
            }
            count = 0;
            byte = 0;
        }
        else
        {
            // Process pulse using EXACT inputMonitor logic (type 2 = hex)
            if ((value > (SLINK_MARK_SYNC / SLINK_MARK_RANGE)) && (value < SLINK_MARK_SYNC * SLINK_MARK_RANGE))
            {
                // Sync pattern found
                buffer += String("\n");
                buffer += String("START,");
                count = 0;
                byte = 0;
            }
            else
            {
                // Process data bits
                if ((value > SLINK_MARK_ONE / SLINK_MARK_RANGE) && (value < SLINK_MARK_ONE * SLINK_MARK_RANGE))
                {
                    byte |= 128 >> count; // Set bit for '1'
                }
                // For zero bits, no action needed (already 0)
                
                if (count++ == 7)
                {
                    // Complete byte - add to buffer in inputMonitor format
                    buffer += String(byte, HEX) + String(",");
                    count = 0;
                    byte = 0;
                }
            }
        }
    } while (micros() - Start < uSecTimeout);
    
    Serial.print("DEBUG: inputMonitorCapture result: ");
    Serial.println(buffer);
    
    return buffer;
}

// Version of inputMonitor that returns the buffer instead of printing it
String Slink::inputMonitorWithReturn(int type, boolean idle, unsigned long uSecTimeout)
{
    unsigned long value = 0;
    unsigned long Start = micros();
    int nl = 0;
    int count = 0;
    int byte = 0;
    String buffer = "";

    Serial.println("DEBUG: inputMonitorWithReturn starting (EXACT inputMonitor logic)...");
    do
    {
        value = pulseIn(_slinkPin, idle, 3000UL); // timeout to 3 milliseconds=3000 uSec
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
    
    Serial.print("DEBUG: inputMonitorWithReturn result: ");
    Serial.println(buffer);
    
    return buffer;
}

#endif