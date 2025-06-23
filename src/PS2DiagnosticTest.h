#pragma once

#include "PS2dev.h"

class PS2DiagnosticTest {
public:
    PS2DiagnosticTest(PS2dev& ps2device) : ps2(ps2device) {}
    
    // Comprehensive Enter key test with multiple timing approaches
    void runEnterKeyDiagnostics();
    
    // Test different timing parameters
    void runTimingTests(); 
    
    // Test host communication
    void runHostCommTests();
    
    // Run full diagnostic suite
    void runFullDiagnostics();
    
private:
    PS2dev& ps2;
    
    // Test different Enter key methods
    void testEnterMethod1_OriginalMkbrk();
    void testEnterMethod2_PressRelease();
    void testEnterMethod3_WithDelays();
    void testEnterMethod4_MultipleAttempts();
    
    // Test with different timing constants
    void testWithTiming(int clkfull, int clkhalf, int bytewait);
    
    // Test basic communication
    void testBasicKeypress(char key);
    
    // Monitor for host responses
    void monitorHostCommunication(int durationMs);
};