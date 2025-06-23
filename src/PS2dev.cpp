#include "PS2dev.h"

// Debug mode - uncomment to enable serial debugging
#define _PS2DBG Serial

#define BYTE_INTERVAL_MICROS 100

// Timing constants from original ps2dev library
// Testing shows original timing works better than conservative slow timing
#define CLKFULL 40    // Original ps2dev timing
#define CLKHALF 20    // Original ps2dev timing
#define BYTEWAIT 1000 // Original ps2dev timing
#define TIMEOUT 30

// PS/2 scan codes for ASCII characters (0x00-0x7F)
const PS2dev::ScanCode PS2dev::scanCodes[128] = {
    // 0x00-0x1F (control characters)
    {0x00, false},
    {0x00, false},
    {0x00, false},
    {0x00, false},
    {0x00, false},
    {0x00, false},
    {0x00, false},
    {0x00, false},
    {0x66, false},
    {0x0D, false},
    {0x5A, false},
    {0x00, false},
    {0x00, false},
    {0x5A, false},
    {0x00, false},
    {0x00, false},
    {0x00, false},
    {0x00, false},
    {0x00, false},
    {0x00, false},
    {0x00, false},
    {0x00, false},
    {0x00, false},
    {0x00, false},
    {0x00, false},
    {0x00, false},
    {0x00, false},
    {0x76, false},
    {0x00, false},
    {0x00, false},
    {0x00, false},
    {0x00, false},

    // 0x20-0x2F (space, punctuation, numbers)
    {0x29, false},
    {0x16, true},
    {0x52, true},
    {0x26, true},
    {0x25, true},
    {0x2E, true},
    {0x3D, true},
    {0x52, false},
    {0x46, true},
    {0x45, true},
    {0x3E, true},
    {0x55, true},
    {0x41, false},
    {0x4E, false},
    {0x49, false},
    {0x4A, false},
    {0x45, false},
    {0x16, false},
    {0x1E, false},
    {0x26, false},
    {0x25, false},
    {0x2E, false},
    {0x36, false},
    {0x3D, false},
    {0x3E, false},
    {0x46, false},
    {0x4C, true},
    {0x4C, false},
    {0x41, true},
    {0x55, false},
    {0x49, true},
    {0x4A, true},

    // 0x40-0x4F (@ and uppercase A-O)
    {0x1E, true},
    {0x1C, true},
    {0x32, true},
    {0x21, true},
    {0x23, true},
    {0x24, true},
    {0x2B, true},
    {0x34, true},
    {0x33, true},
    {0x43, true},
    {0x3B, true},
    {0x42, true},
    {0x4B, true},
    {0x3A, true},
    {0x31, true},
    {0x44, true},

    // 0x50-0x5F (uppercase P-Z, brackets, etc.)
    {0x4D, true},
    {0x15, true},
    {0x2D, true},
    {0x1B, true},
    {0x2C, true},
    {0x3C, true},
    {0x2A, true},
    {0x1D, true},
    {0x22, true},
    {0x35, true},
    {0x1A, true},
    {0x54, false},
    {0x5D, false},
    {0x5B, false},
    {0x36, true},
    {0x4E, true},

    // 0x60-0x6F (backtick, lowercase a-o)
    {0x0E, false},
    {0x1C, false},
    {0x32, false},
    {0x21, false},
    {0x23, false},
    {0x24, false},
    {0x2B, false},
    {0x34, false},
    {0x33, false},
    {0x43, false},
    {0x3B, false},
    {0x42, false},
    {0x4B, false},
    {0x3A, false},
    {0x31, false},
    {0x44, false},

    // 0x70-0x7F (lowercase p-z, braces, etc.)
    {0x4D, false},
    {0x15, false},
    {0x2D, false},
    {0x1B, false},
    {0x2C, false},
    {0x3C, false},
    {0x2A, false},
    {0x1D, false},
    {0x22, false},
    {0x35, false},
    {0x1A, false},
    {0x54, true},
    {0x5D, true},
    {0x5B, true},
    {0x0E, true},
    {0x66, false}};

PS2dev::PS2dev(int clk, int data)
{
    _ps2clk = clk;
    _ps2data = data;
    gohi(_ps2clk);
    gohi(_ps2data);
    leds = 0;
    handling_io_abort = false;
    lastHostCheck = 0;
}

void PS2dev::gohi(int pin)
{
    pinMode(pin, INPUT);
    digitalWrite(pin, HIGH);
}

void PS2dev::golo(int pin)
{
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
}

int PS2dev::do_write(unsigned char data)
{
    int ret;
    if ((ret = write(data)) == EABORT && !handling_io_abort)
    {
#ifdef _PS2DBG
        _PS2DBG.print("ABORT detected during write of 0x");
        _PS2DBG.print(data, HEX);
        _PS2DBG.println(" - handling host communication");
#endif
        handling_io_abort = true;
        keyboard_handle(&leds);
        handling_io_abort = false;
#ifdef _PS2DBG
        _PS2DBG.println("Retrying after abort...");
#endif
    }
    return ret;
}

int PS2dev::write(unsigned char data)
{
    unsigned char i;
    unsigned char parity = 1;
    unsigned char original_data = data; // Store for debugging

#ifdef _PS2DBG
    _PS2DBG.print("sending 0x");
    _PS2DBG.print(original_data, HEX);
    _PS2DBG.print("...");
#endif

    // Ensure lines are in proper state before transmission
    gohi(_ps2data);
    gohi(_ps2clk);
    delayMicroseconds(100); // Extra settling time for Arduino UNO R4 WiFi

    golo(_ps2data);
    delayMicroseconds(CLKHALF);
    golo(_ps2clk);
    delayMicroseconds(CLKFULL);
    gohi(_ps2clk);
    delayMicroseconds(CLKHALF);

    for (i = 0; i < 8; i++)
    {
        if (digitalRead(_ps2clk) == LOW)
        {
#ifdef _PS2DBG
            _PS2DBG.println(" ABORT (host interrupt during data)");
#endif
            gohi(_ps2data);
            return EABORT;
        }
        if (data & 0x01)
        {
            gohi(_ps2data);
        }
        else
        {
            golo(_ps2data);
        }
        delayMicroseconds(CLKHALF);
        golo(_ps2clk);
        delayMicroseconds(CLKFULL);
        gohi(_ps2clk);
        delayMicroseconds(CLKHALF);

        parity = parity ^ (data & 0x01);
        data = data >> 1;
    }

    // Parity bit
    if (parity)
    {
        gohi(_ps2data);
    }
    else
    {
        golo(_ps2data);
    }
    delayMicroseconds(CLKHALF);
    golo(_ps2clk);
    delayMicroseconds(CLKFULL);
    gohi(_ps2clk);
    delayMicroseconds(CLKHALF);
    if (digitalRead(_ps2clk) == LOW)
    {
#ifdef _PS2DBG
        _PS2DBG.println(" ABORT (host interrupt during parity)");
#endif
        gohi(_ps2data);
        return EABORT;
    }

    // Stop bit
    gohi(_ps2data);
    delayMicroseconds(CLKHALF);
    golo(_ps2clk);
    delayMicroseconds(CLKFULL);
    gohi(_ps2clk);
    delayMicroseconds(CLKHALF);

    // Extended wait for Arduino UNO R4 WiFi - allow host to process
    delayMicroseconds(BYTEWAIT + 500); // Extra 500µs for reliability

#ifdef _PS2DBG
    _PS2DBG.print(" SUCCESS (0x");
    _PS2DBG.print(original_data, HEX);
    _PS2DBG.println(")");
#endif

    return ENOERR;
}

int PS2dev::available()
{
    return ((digitalRead(_ps2data) == LOW) || (digitalRead(_ps2clk) == LOW));
}

int PS2dev::do_read(unsigned char *value)
{
    int ret;
    if ((ret = read(value)) == EABORT && !handling_io_abort)
    {
        handling_io_abort = true;
        keyboard_handle(&leds);
        handling_io_abort = false;
    }
    return ret;
}

int PS2dev::read(unsigned char *value)
{
    unsigned int data = 0x00;
    unsigned int bit = 0x01;
    unsigned char calculated_parity = 1;
    unsigned char received_parity = 0;

    unsigned long waiting_since = millis();
    while ((digitalRead(_ps2data) != LOW) || (digitalRead(_ps2clk) != HIGH))
    {
        if (!available())
            return ECANCEL;
        if ((millis() - waiting_since) > TIMEOUT)
            return ETIMEOUT;
    }

    delayMicroseconds(CLKHALF);
    golo(_ps2clk);
    delayMicroseconds(CLKFULL);
    gohi(_ps2clk);
    delayMicroseconds(CLKHALF);

    if (digitalRead(_ps2clk) == LOW)
    {
        return EABORT;
    }

    while (bit < 0x0100)
    {
        if (digitalRead(_ps2data) == HIGH)
        {
            data = data | bit;
            calculated_parity = calculated_parity ^ 1;
        }
        else
        {
            calculated_parity = calculated_parity ^ 0;
        }

        bit = bit << 1;

        delayMicroseconds(CLKHALF);
        golo(_ps2clk);
        delayMicroseconds(CLKFULL);
        gohi(_ps2clk);
        delayMicroseconds(CLKHALF);

        if (digitalRead(_ps2clk) == LOW)
        {
            return EABORT;
        }
    }

    if (digitalRead(_ps2data) == HIGH)
    {
        received_parity = 1;
    }

    delayMicroseconds(CLKHALF);
    golo(_ps2clk);
    delayMicroseconds(CLKFULL);
    gohi(_ps2clk);
    delayMicroseconds(CLKHALF);

    delayMicroseconds(CLKHALF);
    golo(_ps2data);
    golo(_ps2clk);
    delayMicroseconds(CLKFULL);
    gohi(_ps2clk);
    delayMicroseconds(CLKHALF);
    gohi(_ps2data);

    *value = data & 0x00FF;

#ifdef _PS2DBG
    _PS2DBG.print("received data ");
    _PS2DBG.println(*value, HEX);
    _PS2DBG.print("received parity ");
    _PS2DBG.print(received_parity, BIN);
    _PS2DBG.print(" calculated parity ");
    _PS2DBG.println(calculated_parity, BIN);
#endif

    if (received_parity == calculated_parity)
    {
        return ENOERR;
    }
    else
    {
        return ECANCEL;
    }
}

void PS2dev::keyboard_init()
{
    delay(200);
    write(0xAA);
    return;
}

void PS2dev::ack()
{
    delayMicroseconds(BYTE_INTERVAL_MICROS);
    write(0xFA);
    delayMicroseconds(BYTE_INTERVAL_MICROS);
    return;
}

int PS2dev::keyboard_reply(unsigned char cmd, unsigned char *leds_)
{
    unsigned char val;

#ifdef _PS2DBG
    _PS2DBG.print("Host command received: 0x");
    _PS2DBG.print(cmd, HEX);
    _PS2DBG.println();
#endif

    switch (cmd)
    {
    case 0xFF: // Reset - Complete keyboard reset
#ifdef _PS2DBG
        _PS2DBG.println("Processing RESET command");
#endif
        ack();
        // Reset internal state
        leds = 0;
        // Scanning always enabled in defensive mode
        // Send ACK then BAT success
        while (write(0xFA) != 0)
            delay(1);
        while (write(0xAA) != 0)
            delay(1);
        break;

    case 0xFE: // Resend last byte
#ifdef _PS2DBG
        _PS2DBG.println("Processing RESEND command");
#endif
        ack();
        break;

    case 0xF6: // Set defaults
#ifdef _PS2DBG
        _PS2DBG.println("Processing SET DEFAULTS command");
#endif
        // Reset to power-on defaults
        leds = 0;
        // Scanning always enabled in defensive mode
        ack();
        break;

    case 0xF5: // Disable data reporting
#ifdef _PS2DBG
        _PS2DBG.println("Processing DISABLE command");
#endif
        // Scanning always enabled in defensive mode
        ack();
        break;

    case 0xF4: // Enable data reporting
#ifdef _PS2DBG
        _PS2DBG.println("Processing ENABLE command");
#endif
        // Scanning always enabled in defensive mode
        ack();
        break;

    case 0xF3: // Set typematic rate/delay
#ifdef _PS2DBG
        _PS2DBG.println("Processing SET TYPEMATIC RATE command");
#endif
        ack();
        if (!read(&val))
        {
#ifdef _PS2DBG
            _PS2DBG.print("Typematic parameter: 0x");
            _PS2DBG.println(val, HEX);
#endif
            ack();
        }
        break;

    case 0xF2: // Read ID - Identify keyboard type
#ifdef _PS2DBG
        _PS2DBG.println("Processing READ ID command");
#endif
        ack();
        // Send standard keyboard ID: 0xAB 0x83
        do
        {
            if (do_write(0xAB) == EABORT)
                continue;
            if (do_write(0x83) == EABORT)
                continue;
            break;
        } while (!handling_io_abort);
        break;

    case 0xF0: // Set scan code set
#ifdef _PS2DBG
        _PS2DBG.println("Processing SET SCAN CODE SET command");
#endif
        ack();
        if (!read(&val))
        {
#ifdef _PS2DBG
            _PS2DBG.print("Scan code set: ");
            _PS2DBG.println(val, HEX);
#endif
            ack();
        }
        break;

    case 0xEE: // Echo - Simple ping test
#ifdef _PS2DBG
        _PS2DBG.println("Processing ECHO command");
#endif
        delayMicroseconds(BYTE_INTERVAL_MICROS);
        write(0xEE);
        delayMicroseconds(BYTE_INTERVAL_MICROS);
        break;

    case 0xED: // Set/Reset LEDs - Critical for Caps Lock behavior
#ifdef _PS2DBG
        _PS2DBG.println("Processing SET LEDs command");
#endif
        while (write(0xFA) != 0)
            delay(1);
        if (!read(&leds))
        {
            while (write(0xFA) != 0)
                delay(1);
#ifdef _PS2DBG
            _PS2DBG.print("LED state updated: ");
            _PS2DBG.print("Scroll=");
            _PS2DBG.print((leds & 0x01) ? "ON" : "OFF");
            _PS2DBG.print(" Num=");
            _PS2DBG.print((leds & 0x02) ? "ON" : "OFF");
            _PS2DBG.print(" Caps=");
            _PS2DBG.println((leds & 0x04) ? "ON" : "OFF");
#endif
        }
        *leds_ = leds;
        return 1;
        break;

    default:
        // Unknown command - send RESEND (0xFE) to indicate error
#ifdef _PS2DBG
        _PS2DBG.print("Unknown command: 0x");
        _PS2DBG.println(cmd, HEX);
#endif
        write(0xFE);
        break;
    }
    return 0;
}

int PS2dev::keyboard_handle(unsigned char *leds_)
{
    unsigned char c;
    if (available())
    {
#ifdef _PS2DBG
        _PS2DBG.println("Host communication detected - reading...");
#endif
        if (!read(&c))
        {
#ifdef _PS2DBG
            _PS2DBG.print("Host sent command: 0x");
            _PS2DBG.println(c, HEX);
#endif
            return keyboard_reply(c, leds_);
        }
    }
    return 0;
}

/**
 * @brief  Sends a "make" (press) and "break" (release) scancode sequence.
 * @note   This function is updated with defensive timing to prevent double-presses
 * and ensure the host has time to process the event.
 * @param  code The scancode of the key to send.
 * @return 0 on success.
 */
int PS2dev::keyboard_mkbrk(unsigned char code)
{
    bus_idle();
    keyboard_press(code);
    delay(40); // Key hold time: A more deliberate press.
    bus_idle();
    keyboard_release(code);
    delay(20); // Recovery time: Prevents key bounce and gives host time to recover.
    return 0;
}

/**
 * @brief  Sends a key press scancode, respecting host inhibit.
 */
int PS2dev::keyboard_press(unsigned char code)
{
    bus_idle(); // Ensure bus is idle before starting.
    return do_write(code);
}

/**
 * @brief  Sends a key release scancode, respecting host inhibit.
 */
int PS2dev::keyboard_release(unsigned char code)
{
    bus_idle(); // Ensure bus is idle before starting.
    do_write(0xF0);
    return do_write(code);
}

int PS2dev::keyboard_press_special(unsigned char code)
{
    // **CRITICAL**: Wait for host to be ready before transmitting
    bus_idle();

    do
    {
        if (do_write(0xe0) == EABORT)
            continue;
        if (do_write(code) == EABORT)
            continue;
        break;
    } while (!handling_io_abort);
    return 0;
}

int PS2dev::keyboard_release_special(unsigned char code)
{
    // **CRITICAL**: Wait for host to be ready before transmitting
    bus_idle();

    do
    {
        if (do_write(0xe0) == EABORT)
            continue;
        if (do_write(0xf0) == EABORT)
            continue;
        if (do_write(code) == EABORT)
            continue;
        break;
    } while (!handling_io_abort);
    return 0;
}

int PS2dev::keyboard_special_mkbrk(unsigned char code)
{
    do
    {
        if (do_write(0xe0) == EABORT)
            continue;
        if (do_write(code) == EABORT)
            continue;
        break;
    } while (!handling_io_abort);
    do
    {
        if (do_write(0xe0) == EABORT)
            continue;
        if (do_write(0xf0) == EABORT)
            continue;
        if (do_write(code) == EABORT)
            continue;
        break;
    } while (!handling_io_abort);
    return 0;
}

int PS2dev::keyboard_press_printscreen()
{
    do
    {
        if (do_write(0xe0) == EABORT)
            continue;
        if (do_write(0x12) == EABORT)
            continue;
        if (do_write(0xe0) == EABORT)
            continue;
        if (do_write(0x7c) == EABORT)
            continue;
        break;
    } while (!handling_io_abort);
    return 0;
}

int PS2dev::keyboard_release_printscreen()
{
    do
    {
        if (do_write(0xe0) == EABORT)
            continue;
        if (do_write(0xf0) == EABORT)
            continue;
        if (do_write(0x7c) == EABORT)
            continue;
        if (do_write(0xe0) == EABORT)
            continue;
        if (do_write(0xf0) == EABORT)
            continue;
        if (do_write(0x12) == EABORT)
            continue;
        break;
    } while (!handling_io_abort);
    return 0;
}

int PS2dev::keyboard_mkbrk_printscreen()
{
    keyboard_press_printscreen();
    keyboard_release_printscreen();
    return 0;
}

int PS2dev::keyboard_pausebreak()
{
    do
    {
        if (do_write(0xe1) == EABORT)
            continue;
        if (do_write(0x14) == EABORT)
            continue;
        if (do_write(0x77) == EABORT)
            continue;
        break;
    } while (!handling_io_abort);
    do
    {
        if (do_write(0xe1) == EABORT)
            continue;
        if (do_write(0xf0) == EABORT)
            continue;
        if (do_write(0x14) == EABORT)
            continue;
        if (do_write(0xe0) == EABORT)
            continue;
        if (do_write(0x77) == EABORT)
            continue;
        break;
    } while (!handling_io_abort);
    return 0;
}

// High-level compatibility functions
/**
 * @brief  Initializes the PS/2 device, now including the BAT and typematic handshake.
 */
void PS2dev::begin()
{
#ifdef _PS2DBG
    _PS2DBG.println("PS2dev: Initializing...");
#endif

    pinMode(_ps2clk, INPUT_PULLUP);
    pinMode(_ps2data, INPUT_PULLUP);

    // Per BAT specification, wait before sending completion code.
    delay(750);

#ifdef _PS2DBG
    _PS2DBG.println("PS2dev: Sending BAT completion code...");
#endif
    write(0xAA); // Send BAT success code

#ifdef _PS2DBG
    _PS2DBG.println("PS2dev: Setting default typematic rate (handshake)...");
#endif
    set_typematic_rate(); // Perform the handshake

    lastHostCheck = millis();
#ifdef _PS2DBG
    _PS2DBG.println("PS2dev: Initialization complete and ready.");
#endif
}

bool PS2dev::calculateParity(uint8_t data)
{
    int count = 0;
    for (int i = 0; i < 8; i++)
    {
        if (data & (1 << i))
            count++;
    }
    return (count % 2) == 0;
}

/**
 * @brief  High-level function to send an ASCII character.
 * @note   Updated with more generous delays for modifier key stability.
 * @param  c The character to send.
 */
void PS2dev::sendKey(char c)
{
    if (c < 0 || c >= 128)
        return;
    ScanCode sc = scanCodes[(uint8_t)c];
    if (sc.code == 0x00)
        return;

    if (sc.shift)
    {
        keyboard_press(LEFT_SHIFT);
        delay(50); // Increased delay to ensure host recognizes shift state.
    }

    keyboard_mkbrk(sc.code);

    if (sc.shift)
    {
        keyboard_release(LEFT_SHIFT);
        delay(50); // Increased delay to ensure host recognizes shift release.
    }
}

/**
 * @brief  Sends a string of characters with proper pacing.
 * @note   The total delay per character is the delay in this function plus the
 * recovery delay in keyboard_mkbrk().
 */
void PS2dev::sendString(const String &str)
{
    for (unsigned int i = 0; i < str.length(); i++)
    {
        sendKey(str.charAt(i));
        delay(80); // Inter-character pacing. Total delay is now ~100ms.
    }
}

/**
 * @brief Sends the Enter key.
 */
void PS2dev::sendEnter()
{
    keyboard_mkbrk(ENTER);
}

void PS2dev::sendBackspace()
{
    keyboard_mkbrk(0x66);
    delay(10);
}

void PS2dev::sendEscape()
{
    keyboard_mkbrk(0x76);
    delay(10);
}

/**
 * @brief Sends Shift+Delete to clear text fields on the Sony device.
 */
void PS2dev::sendShiftDelete()
{
    keyboard_press(LEFT_SHIFT);
    delay(50);
    keyboard_press_special(DELETE);
    delay(100); // Hold down duration
    keyboard_release_special(DELETE);
    delay(50);
    keyboard_release(LEFT_SHIFT);
}

void PS2dev::sendByte(unsigned char data)
{
    write(data);
}

// *** ENHANCED HOST INHIBIT FUNCTION ***
// Robust host inhibit check with timeout protection to prevent infinite loops

// *** ENHANCED TYPEMATIC RATE FUNCTION ***
// Sets the typematic rate and delay to match standard keyboard defaults.
/**
 * @brief  Sets the keyboard's typematic rate and delay to standard defaults.
 * @note   This acts as a crucial handshake with the host, ensuring both sides
 * are synchronized to standard keyboard behavior.
 */
void PS2dev::set_typematic_rate()
{
    unsigned char ack_response;

#ifdef _PS2DBG
    _PS2DBG.println("Setting default typematic rate...");
#endif

    bus_idle();
    write(0xF3);         // Command: Set Typematic Rate
    read(&ack_response); // Wait for and read the ACK (0xFA)

    bus_idle();
    // Parameter: 0b00101011 -> 0x2B
    // Delay: 500ms, Rate: 10.9 chars/sec
    write(0x2B);
    read(&ack_response); // Wait for and read the final ACK

#ifdef _PS2DBG
    if (ack_response == 0xFA)
    {
        _PS2DBG.println("Typematic rate handshake successful.");
    }
    else
    {
        _PS2DBG.print("Typematic rate handshake failed, received: 0x");
        _PS2DBG.println(ack_response, HEX);
    }
#endif
}

void PS2dev::update()
{
    // Simple polling-based update for host communication
    unsigned long now = millis();
    if (now - lastHostCheck >= 10)
    {
        unsigned char leds_temp;
        keyboard_handle(&leds_temp);
        lastHostCheck = now;
    }
}

// *** DEFENSIVE TIMING IMPLEMENTATION ***

/**
 * @brief  Waits for the PS/2 bus to be in a true idle state.
 * @note   A compliant device must only transmit when both CLK and DATA
 * lines have been continuously high for at least 50 microseconds.
 */
void PS2dev::bus_idle()
{
    unsigned long start_time = micros();
    while (micros() - start_time < 50)
    {
        if (digitalRead(_ps2clk) == LOW || digitalRead(_ps2data) == LOW)
        {
            // If a line drops, reset the timer and start waiting again.
            start_time = micros();
        }
    }
}
