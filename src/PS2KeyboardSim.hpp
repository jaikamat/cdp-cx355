#pragma once

#include <Arduino.h>

// PS/2 Keyboard Simulator for Arduino Uno R4 WiFi
// Simulates a PS/2 keyboard to send keystrokes to devices expecting PS/2 input
class PS2KeyboardSim {
private:
    int clockPin;
    int dataPin;
    bool initialized;
    unsigned long lastHostCheck;
    
    // PS/2 timing constants (microseconds) - Conservative timing for older devices
    static const int CLOCK_HALF_PERIOD = 50;  // 10 kHz clock rate (was working)
    static const int SETUP_TIME = 10;         // Data setup time (original)
    static const int HOLD_TIME = 5;           // Data hold time
    
    // PS/2 scan codes for common characters
    struct ScanCode {
        uint8_t code;
        bool shift;
    };
    
    // Scan code lookup table for basic ASCII characters
    static const ScanCode scanCodes[128];
    
    void sendBit(bool bit);
    bool calculateParity(uint8_t data);
    void delayMicroseconds(int us);
    void sendBAT();
    void checkHostCommands();
    bool isClockInhibited();
    void waitForClockRelease();
    
public:
    PS2KeyboardSim(int clkPin, int datPin);
    void begin();
    void update();  // Call regularly to process host commands
    void sendKey(char c);
    void sendString(const String& str);
    void sendEnter();
    void sendBackspace();
    void sendEscape();
    void sendByte(uint8_t data);
    void reset();  // Reinitialize device
};