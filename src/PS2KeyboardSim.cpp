#include "PS2KeyboardSim.hpp"

// PS/2 scan codes for ASCII characters (0x00-0x7F)
// Format: {scan_code, requires_shift}
const PS2KeyboardSim::ScanCode PS2KeyboardSim::scanCodes[128] = {
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

PS2KeyboardSim::PS2KeyboardSim(int clkPin, int datPin) : clockPin(clkPin), dataPin(datPin), initialized(false), lastHostCheck(0) {}

void PS2KeyboardSim::begin() {
    // Initialize pins as outputs with idle state (both high)
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, OUTPUT);
    digitalWrite(clockPin, HIGH);
    digitalWrite(dataPin, HIGH);
    
    // Give host time to initialize
    delay(100);
    
    initialized = true;
    lastHostCheck = millis();
}

void PS2KeyboardSim::delayMicroseconds(int us) {
    // More precise delay for timing-critical PS/2 protocol
    if (us > 16383) {
        delay(us / 1000);
        us = us % 1000;
    }
    if (us > 0) {
        ::delayMicroseconds(us); // Use global delayMicroseconds function
    }
}

bool PS2KeyboardSim::calculateParity(uint8_t data) {
    // Calculate odd parity (parity bit makes total 1-bits odd)
    int count = 0;
    for (int i = 0; i < 8; i++) {
        if (data & (1 << i)) count++;
    }
    return (count % 2) == 0; // Return 1 if even number of 1s (to make total odd)
}

void PS2KeyboardSim::sendBit(bool bit) {
    // PS/2 protocol: data is valid on falling edge of clock
    
    // Set data line
    digitalWrite(dataPin, bit ? HIGH : LOW);
    delayMicroseconds(SETUP_TIME);
    
    // Clock high to low transition
    digitalWrite(clockPin, LOW);
    delayMicroseconds(CLOCK_HALF_PERIOD);
    
    // Clock low to high transition  
    digitalWrite(clockPin, HIGH);
    delayMicroseconds(CLOCK_HALF_PERIOD);
}

void PS2KeyboardSim::sendByte(uint8_t data) {
    // PS/2 frame format: 1 start + 8 data + 1 parity + 1 stop
    
    // Start bit (always 0)
    sendBit(false);
    
    // Data bits (LSB first)
    for (int i = 0; i < 8; i++) {
        sendBit((data >> i) & 1);
    }
    
    // Parity bit (odd parity)
    sendBit(calculateParity(data));
    
    // Stop bit (always 1)
    sendBit(true);
    
    // Brief inter-byte delay
    delayMicroseconds(100);
}

void PS2KeyboardSim::sendKey(char c) {
    if (c < 0 || c >= 128) return; // Invalid character
    
    ScanCode sc = scanCodes[(uint8_t)c];
    if (sc.code == 0x00) return; // Unsupported character
    
    // Send shift key press if needed
    if (sc.shift) {
        sendByte(0x12); // Left shift make code
    }
    
    // Send character make code
    sendByte(sc.code);
    
    // Hold key down briefly (human-like press)
    delay(50);  // 50ms hold for regular characters
    
    // Send character break code (make code + 0xF0 prefix)
    sendByte(0xF0);
    sendByte(sc.code);
    
    // Send shift key release if needed
    if (sc.shift) {
        sendByte(0xF0);
        sendByte(0x12); // Left shift break code
    }
    
    // Longer inter-character delay for older devices
    delay(50);
}

void PS2KeyboardSim::sendString(const String& str) {
    for (unsigned int i = 0; i < str.length(); i++) {
        sendKey(str.charAt(i));
    }
}

void PS2KeyboardSim::sendEnter() {
    sendByte(0x5A); // Enter make (key press down)
    // No delay - instant make/break like Test 1 that worked
    sendByte(0xF0);
    sendByte(0x5A); // Enter break (key release)
    delay(100);     // Delay after key release
}

void PS2KeyboardSim::sendBackspace() {
    sendByte(0x66); // Backspace make
    sendByte(0xF0);
    sendByte(0x66); // Backspace break
    delay(10);
}

void PS2KeyboardSim::sendEscape() {
    sendByte(0x76); // Escape make
    sendByte(0xF0);
    sendByte(0x76); // Escape break
    delay(10);
}

void PS2KeyboardSim::sendBAT() {
    // Send Basic Assurance Test completion code
    // This is critical - host expects this to recognize device as valid keyboard
    delay(200); // Wait for host to be ready
    sendByte(0xAA); // BAT completion successful
    delay(100);
}

void PS2KeyboardSim::checkHostCommands() {
    // Check if host is sending commands by monitoring data line
    // This is a simplified implementation - full version would handle all commands
    // For now, just handle basic protocol requirements
    
    // Check if clock is being pulled low by host (request to send)
    if (digitalRead(clockPin) == LOW) {
        // Host wants to send - we should listen and respond
        // For now, just wait for release
        waitForClockRelease();
    }
}

bool PS2KeyboardSim::isClockInhibited() {
    return digitalRead(clockPin) == LOW;
}

void PS2KeyboardSim::waitForClockRelease() {
    // Wait for host to release clock line
    unsigned long start = millis();
    while (digitalRead(clockPin) == LOW && (millis() - start) < 1000) {
        delayMicroseconds(10);
    }
    
    // After release, wait additional time before transmitting
    if (digitalRead(clockPin) == HIGH) {
        delayMicroseconds(50);
    }
}

void PS2KeyboardSim::update() {
    // Call this regularly (every 10ms) to process host commands
    if (!initialized) return;
    
    unsigned long now = millis();
    if (now - lastHostCheck >= 10) {
        checkHostCommands();
        lastHostCheck = now;
    }
}

void PS2KeyboardSim::reset() {
    // Reinitialize the device
    initialized = false;
    digitalWrite(clockPin, HIGH);
    digitalWrite(dataPin, HIGH);
    delay(100);
    sendBAT();
    initialized = true;
    lastHostCheck = millis();
}