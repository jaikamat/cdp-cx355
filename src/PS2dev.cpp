#include "PS2dev.h"

// Debug mode - uncomment to enable serial debugging
#define _PS2DBG Serial

#define BYTE_INTERVAL_MICROS 100

// Timing constants from original ps2dev library
// Testing shows original timing works better than conservative slow timing
#define CLKFULL 40      // Original ps2dev timing
#define CLKHALF 20      // Original ps2dev timing  
#define BYTEWAIT 1000   // Original ps2dev timing
#define TIMEOUT 30

// PS/2 scan codes for ASCII characters (0x00-0x7F)
const PS2dev::ScanCode PS2dev::scanCodes[128] = {
    // 0x00-0x1F (control characters)
    {0x00, false}, {0x00, false}, {0x00, false}, {0x00, false}, {0x00, false}, {0x00, false}, {0x00, false}, {0x00, false},
    {0x66, false}, {0x0D, false}, {0x5A, false}, {0x00, false}, {0x00, false}, {0x5A, false}, {0x00, false}, {0x00, false},
    {0x00, false}, {0x00, false}, {0x00, false}, {0x00, false}, {0x00, false}, {0x00, false}, {0x00, false}, {0x00, false},
    {0x00, false}, {0x00, false}, {0x00, false}, {0x76, false}, {0x00, false}, {0x00, false}, {0x00, false}, {0x00, false},
    
    // 0x20-0x2F (space, punctuation, numbers)
    {0x29, false}, {0x16, true},  {0x52, true},  {0x26, true},  {0x25, true},  {0x2E, true},  {0x3D, true},  {0x52, false},
    {0x46, true},  {0x45, true},  {0x3E, true},  {0x55, true},  {0x41, false}, {0x4E, false}, {0x49, false}, {0x4A, false},
    {0x45, false}, {0x16, false}, {0x1E, false}, {0x26, false}, {0x25, false}, {0x2E, false}, {0x36, false}, {0x3D, false},
    {0x3E, false}, {0x46, false}, {0x4C, true},  {0x4C, false}, {0x41, true},  {0x55, false}, {0x49, true},  {0x4A, true},
    
    // 0x40-0x4F (@ and uppercase A-O)
    {0x1E, true},  {0x1C, true},  {0x32, true},  {0x21, true},  {0x23, true},  {0x24, true},  {0x2B, true},  {0x34, true},
    {0x33, true},  {0x43, true},  {0x3B, true},  {0x42, true},  {0x4B, true},  {0x3A, true},  {0x31, true},  {0x44, true},
    
    // 0x50-0x5F (uppercase P-Z, brackets, etc.)
    {0x4D, true},  {0x15, true},  {0x2D, true},  {0x1B, true},  {0x2C, true},  {0x3C, true},  {0x2A, true},  {0x1D, true},
    {0x22, true},  {0x35, true},  {0x1A, true},  {0x54, false}, {0x5D, false}, {0x5B, false}, {0x36, true},  {0x4E, true},
    
    // 0x60-0x6F (backtick, lowercase a-o)
    {0x0E, false}, {0x1C, false}, {0x32, false}, {0x21, false}, {0x23, false}, {0x24, false}, {0x2B, false}, {0x34, false},
    {0x33, false}, {0x43, false}, {0x3B, false}, {0x42, false}, {0x4B, false}, {0x3A, false}, {0x31, false}, {0x44, false},
    
    // 0x70-0x7F (lowercase p-z, braces, etc.)
    {0x4D, false}, {0x15, false}, {0x2D, false}, {0x1B, false}, {0x2C, false}, {0x3C, false}, {0x2A, false}, {0x1D, false},
    {0x22, false}, {0x35, false}, {0x1A, false}, {0x54, true},  {0x5D, true},  {0x5B, true},  {0x0E, true},  {0x66, false}
};

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
    if ((ret = write(data)) == EABORT && !handling_io_abort) {
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

    for (i = 0; i < 8; i++) {
        if (digitalRead(_ps2clk) == LOW) {
#ifdef _PS2DBG
            _PS2DBG.println(" ABORT (host interrupt during data)");
#endif
            gohi(_ps2data);
            return EABORT;
        }
        if (data & 0x01) {
            gohi(_ps2data);
        } else {
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
    if (parity) {
        gohi(_ps2data);
    } else {
        golo(_ps2data);
    }
    delayMicroseconds(CLKHALF);
    golo(_ps2clk);
    delayMicroseconds(CLKFULL);
    gohi(_ps2clk);
    delayMicroseconds(CLKHALF);
    if (digitalRead(_ps2clk) == LOW) {
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

int PS2dev::do_read(unsigned char * value)
{
    int ret;
    if ((ret = read(value)) == EABORT && !handling_io_abort) {
        handling_io_abort = true;
        keyboard_handle(&leds);
        handling_io_abort = false;
    }
    return ret;
}

int PS2dev::read(unsigned char * value)
{
    unsigned int data = 0x00;
    unsigned int bit = 0x01;
    unsigned char calculated_parity = 1;
    unsigned char received_parity = 0;

    unsigned long waiting_since = millis();
    while((digitalRead(_ps2data) != LOW) || (digitalRead(_ps2clk) != HIGH)) {
        if (!available()) return ECANCEL;
        if((millis() - waiting_since) > TIMEOUT) return ETIMEOUT;
    }

    delayMicroseconds(CLKHALF);
    golo(_ps2clk);
    delayMicroseconds(CLKFULL);
    gohi(_ps2clk);
    delayMicroseconds(CLKHALF);

    if (digitalRead(_ps2clk) == LOW) {
        return EABORT;
    }

    while (bit < 0x0100) {
        if (digitalRead(_ps2data) == HIGH) {
            data = data | bit;
            calculated_parity = calculated_parity ^ 1;
        } else {
            calculated_parity = calculated_parity ^ 0;
        }

        bit = bit << 1;

        delayMicroseconds(CLKHALF);
        golo(_ps2clk);
        delayMicroseconds(CLKFULL);
        gohi(_ps2clk);
        delayMicroseconds(CLKHALF);

        if (digitalRead(_ps2clk) == LOW) {
            return EABORT;
        }
    }

    if (digitalRead(_ps2data) == HIGH) {
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

    if (received_parity == calculated_parity) {
        return ENOERR;
    } else {
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
    switch (cmd) {
    case 0xFF: // reset
        ack();
        while (write(0xFA) != 0) delay(1);
        while (write(0xAA) != 0) delay(1);
        break;
    case 0xFE: // resend
        ack();
        break;
    case 0xF6: // set defaults
        ack();
        break;
    case 0xF5: // disable data reporting
        ack();
        break;
    case 0xF4: // enable data reporting
        ack();
        break;
    case 0xF3: // set typematic rate
        ack();
        if (!read(&val)) ack();
        break;
    case 0xF2: // get device id
        ack();
        do {
            if (do_write(0xAB) == EABORT) continue;
            if (do_write(0x83) == EABORT) continue;
            break;
        } while (!handling_io_abort);
        break;
    case 0xF0: // set scan code set
        ack();
        if (!read(&val)) ack();
        break;
    case 0xEE: // echo
        delayMicroseconds(BYTE_INTERVAL_MICROS);
        write(0xEE);
        delayMicroseconds(BYTE_INTERVAL_MICROS);
        break;
    case 0xED: // set/reset LEDs
        while (write(0xFA) != 0) delay(1);
        if (!read(&leds)) {
            while (write(0xFA) != 0) delay(1);
        }
#ifdef _PS2DBG
        _PS2DBG.print("LEDs: ");
        _PS2DBG.println(leds, HEX);
#endif
        *leds_ = leds;
        return 1;
        break;
    }
    return 0;
}

int PS2dev::keyboard_handle(unsigned char *leds_)
{
    unsigned char c;
    if (available()) {
#ifdef _PS2DBG
        _PS2DBG.println("Host communication detected - reading...");
#endif
        if (!read(&c)) {
#ifdef _PS2DBG
            _PS2DBG.print("Host sent command: 0x");
            _PS2DBG.println(c, HEX);
#endif
            return keyboard_reply(c, leds_);
        }
    }
    return 0;
}

int PS2dev::keyboard_mkbrk(unsigned char code)
{
#ifdef _PS2DBG
    _PS2DBG.print("keyboard_mkbrk: Starting make/break for code 0x");
    _PS2DBG.println(code, HEX);
#endif
    
    // Process any pending host communications before sending
    unsigned char leds;
    keyboard_handle(&leds);
    
    // Send make (key press) with original ps2dev retry logic
#ifdef _PS2DBG
    _PS2DBG.println("  Sending MAKE...");
#endif
    do {
        if (do_write(code) != EABORT) break;
#ifdef _PS2DBG
        _PS2DBG.println("  MAKE retry due to abort");
#endif
    } while (!handling_io_abort);
    
    // Process host responses after make
    keyboard_handle(&leds);
    delayMicroseconds(500);
    
    // Send break (key release) with original ps2dev retry logic
#ifdef _PS2DBG
    _PS2DBG.println("  Sending BREAK...");
#endif
    do {
        if (do_write(0xF0) != EABORT && do_write(code) != EABORT) break;
#ifdef _PS2DBG
        _PS2DBG.println("  BREAK retry due to abort");
#endif
    } while (!handling_io_abort);
    
    // Process host responses after break
    keyboard_handle(&leds);
    
#ifdef _PS2DBG
    _PS2DBG.print("keyboard_mkbrk: SUCCESS for code 0x");
    _PS2DBG.println(code, HEX);
#endif
    return 0;
}


int PS2dev::keyboard_press(unsigned char code)
{
    do {
        if (do_write(code) == EABORT) continue;
        break;
    } while (!handling_io_abort);
    return 0;
}

int PS2dev::keyboard_release(unsigned char code)
{
    do {
        if (do_write(0xf0) == EABORT) continue;
        if (do_write(code) == EABORT) continue;
        break;
    } while (!handling_io_abort);
    return 0;
}

int PS2dev::keyboard_press_special(unsigned char code)
{
    do {
        if (do_write(0xe0) == EABORT) continue;
        if (do_write(code) == EABORT) continue;
        break;
    } while (!handling_io_abort);
    return 0;
}

int PS2dev::keyboard_release_special(unsigned char code)
{
    do {
        if (do_write(0xe0) == EABORT) continue;
        if (do_write(0xf0) == EABORT) continue;
        if (do_write(code) == EABORT) continue;
        break;
    } while (!handling_io_abort);
    return 0;
}

int PS2dev::keyboard_special_mkbrk(unsigned char code)
{
    do {
        if (do_write(0xe0) == EABORT) continue;
        if (do_write(code) == EABORT) continue;
        break;
    } while (!handling_io_abort);
    do {
        if (do_write(0xe0) == EABORT) continue;
        if (do_write(0xf0) == EABORT) continue;
        if (do_write(code) == EABORT) continue;
        break;
    } while (!handling_io_abort);
    return 0;
}

int PS2dev::keyboard_press_printscreen()
{
    do {
        if (do_write(0xe0) == EABORT) continue;
        if (do_write(0x12) == EABORT) continue;
        if (do_write(0xe0) == EABORT) continue;
        if (do_write(0x7c) == EABORT) continue;
        break;
    } while (!handling_io_abort);
    return 0;
}

int PS2dev::keyboard_release_printscreen()
{
    do {
        if (do_write(0xe0) == EABORT) continue;
        if (do_write(0xf0) == EABORT) continue;
        if (do_write(0x7c) == EABORT) continue;
        if (do_write(0xe0) == EABORT) continue;
        if (do_write(0xf0) == EABORT) continue;
        if (do_write(0x12) == EABORT) continue;
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
    do {
        if (do_write(0xe1) == EABORT) continue;
        if (do_write(0x14) == EABORT) continue;
        if (do_write(0x77) == EABORT) continue;
        break;
    } while (!handling_io_abort);
    do {
        if (do_write(0xe1) == EABORT) continue;
        if (do_write(0xf0) == EABORT) continue;
        if (do_write(0x14) == EABORT) continue;
        if (do_write(0xe0) == EABORT) continue;
        if (do_write(0x77) == EABORT) continue;
        break;
    } while (!handling_io_abort);
    return 0;
}

// High-level compatibility functions
void PS2dev::begin()
{
#ifdef _PS2DBG
    _PS2DBG.println("PS2dev: Initializing...");
#endif
    pinMode(_ps2clk, OUTPUT);
    pinMode(_ps2data, OUTPUT);
    gohi(_ps2clk);
    gohi(_ps2data);
    
#ifdef _PS2DBG
    _PS2DBG.println("PS2dev: Pins configured, waiting for stabilization...");
#endif
    delay(500);  // Longer initialization delay for UNO R4 WiFi
    
#ifdef _PS2DBG
    _PS2DBG.println("PS2dev: Sending keyboard initialization...");
#endif
    keyboard_init();
    lastHostCheck = millis();
    
#ifdef _PS2DBG
    _PS2DBG.println("PS2dev: Initialization complete");
#endif
}

bool PS2dev::calculateParity(uint8_t data)
{
    int count = 0;
    for (int i = 0; i < 8; i++) {
        if (data & (1 << i)) count++;
    }
    return (count % 2) == 0;
}

void PS2dev::sendKey(char c)
{
    if (c < 0 || c >= 128) return;
    
    ScanCode sc = scanCodes[(uint8_t)c];
    if (sc.code == 0x00) return;
    
#ifdef _PS2DBG
    _PS2DBG.print("sendKey() called for: '");
    _PS2DBG.print(c);
    _PS2DBG.print("' -> scan code 0x");
    _PS2DBG.print(sc.code, HEX);
    if (sc.shift) _PS2DBG.print(" (with shift)");
    _PS2DBG.println();
#endif
    
    if (sc.shift) {
#ifdef _PS2DBG
        _PS2DBG.println("  Pressing shift...");
#endif
        keyboard_press(0x12); // Left shift make
    }
    
#ifdef _PS2DBG
    _PS2DBG.println("  Sending character make/break...");
#endif
    keyboard_mkbrk(sc.code);  // Key make/break - original ps2dev style
    
    if (sc.shift) {
#ifdef _PS2DBG
        _PS2DBG.println("  Releasing shift...");
#endif
        keyboard_release(0x12); // Left shift break
    }
    
#ifdef _PS2DBG
    _PS2DBG.print("sendKey() completed for: '");
    _PS2DBG.print(c);
    _PS2DBG.println("'");
#endif
}

void PS2dev::sendString(const String& str)
{
    for (unsigned int i = 0; i < str.length(); i++) {
        sendKey(str.charAt(i));
    }
}

void PS2dev::sendEnter()
{
#ifdef _PS2DBG
    _PS2DBG.println("Sending Enter key...");
#endif
    // Use original ps2dev method - simple make/break without delays
    keyboard_mkbrk(0x5A);
#ifdef _PS2DBG
    _PS2DBG.println("Enter key sent");
#endif
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

void PS2dev::sendShiftDelete()
{
#ifdef _PS2DBG
    _PS2DBG.println("Sending Shift+Delete to clear text...");
#endif
    // Sony documentation method: Hold Shift, press Delete, release both
    keyboard_press(0x12);           // Press Left Shift
    delay(50);                      // Brief delay to establish shift
    keyboard_press_special(0x71);   // Press Delete (special key with 0xE0 prefix)
    delay(200);                     // Hold Shift+Delete combination
    keyboard_release_special(0x71); // Release Delete  
    keyboard_release(0x12);         // Release Left Shift
#ifdef _PS2DBG
    _PS2DBG.println("Shift+Delete sent");
#endif
}

void PS2dev::sendByte(unsigned char data)
{
    write(data);
}

void PS2dev::update()
{
    unsigned long now = millis();
    if (now - lastHostCheck >= 10) {
        keyboard_handle(&leds);
        lastHostCheck = now;
    }
}

