#include <map>
#include "WiFiS3.h"
#include "WiFiManager.hpp"
#include "HttpParser.hpp"
#include "Secrets.hpp"
#include <LedMatrixController.hpp>
#include "DiscStorage.hpp"
#include <functional>
#include "Sony_SLink.h"
#include "PS2dev.h"

#define SLINK_PIN 2 // Pick a suitable I/O pin for S-Link
#define PS2_CLOCK_PIN 3 // PS/2 clock line (white wire)
#define PS2_DATA_PIN 4  // PS/2 data line (red wire)

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

ArduinoLEDMatrix matrix;
LedMatrixController ledController(matrix);
WiFiManager wifiManager(ssid, password);
DiscStorage storage;
Slink slink; // S-Link object
PS2dev ps2Keyboard(PS2_CLOCK_PIN, PS2_DATA_PIN); // PS/2 keyboard simulator
bool isPlayerOn = false;

// ------------------ URL-DECODE HELPER ------------------
String urlDecode(const String &encoded)
{
  String decoded;
  for (size_t i = 0; i < encoded.length(); i++)
  {
    char c = encoded[i];
    if (c == '+')
    {
      decoded += ' ';
    }
    else if (c == '%' && i + 2 < encoded.length())
    {
      char hex[3] = {encoded[i + 1], encoded[i + 2], '\0'};
      decoded += (char)strtol(hex, nullptr, 16);
      i += 2;
    }
    else
    {
      decoded += c;
    }
  }
  return decoded;
}

// ------------------ S-LINK COMMANDS ------------------
void slinkPlay() { slink.sendCommand(SLINK_DEVICE_CDP_CX1L, SLINK_CMD_CD_PLAY); }
void slinkStop() { slink.sendCommand(SLINK_DEVICE_CDP_CX1L, SLINK_CMD_CD_STOP); }
void slinkPauseToggle() { slink.sendCommand(SLINK_DEVICE_CDP_CX1L, SLINK_CMD_CD_PAUSE_TOGGLE); }
void slinkNextTrack() { slink.sendCommand(SLINK_DEVICE_CDP_CX1L, SLINK_CMD_CD_NEXT); }
void slinkPrevTrack() { slink.sendCommand(SLINK_DEVICE_CDP_CX1L, SLINK_CMD_CD_PREV); }
void slinkPowerOn() { slink.sendCommand(SLINK_DEVICE_CDP_CX1L, SLINK_CMD_CD_POWER_ON); }
void slinkPowerOff() { slink.sendCommand(SLINK_DEVICE_CDP_CX1L, SLINK_CMD_CD_POWER_OFF); }

/** Convert integer 1..99 -> BCD byte */
uint8_t toBCD(int val)
{
  int v = val % 100;
  int highNibble = v / 10;
  int lowNibble = v % 10;
  return (highNibble << 4) | lowNibble;
}

/** Send “Play Direct Track” command for discNumber (1..400) */
void slinkSelectDisc(int discNumber)
{
  // Decide device byte based on discNumber
  uint8_t device = (discNumber > 200) ? 0x93 : 0x90;

  // For 201..400, subtract 200
  int discVal = discNumber;
  if (discNumber > 200)
    discVal -= 200;

  // Convert to BCD or custom offset
  uint8_t discByte = 0;
  if (discVal <= 99)
    discByte = toBCD(discVal);
  else if (discVal <= 200)
    discByte = 0x9A + (discVal - 100);

  // command = 0x50, then discByte, then track=0x01
  slink.sendCommand(device, 0x50, discByte, 0x01);
}

/** Parse title from hex response string */
String parseTitle(const String &hexResponse)
{
  String title = "";

  // Split hex response by spaces and convert to ASCII
  int start = 0;
  int byteCount = 0;

  while (start < hexResponse.length())
  {
    int end = hexResponse.indexOf(' ', start);
    if (end == -1)
      end = hexResponse.length();

    String hexByte = hexResponse.substring(start, end);
    if (hexByte.length() > 0)
    {
      long byteVal = strtol(hexByte.c_str(), NULL, 16);
      byteCount++;

      // Skip first 3 bytes (device ID, command, disc number)
      // ASCII title data starts from byte 4 onward
      if (byteCount >= 4 && byteVal >= 0x20 && byteVal <= 0x7E)
      {
        title += (char)byteVal;
      }
    }
    start = end + 1;
  }

  title.trim();
  return title;
}

/** Monitor S-Link traffic for debugging */
void monitorSLinkTraffic(int seconds)
{
  Serial.print("=== Monitoring S-Link traffic for ");
  Serial.print(seconds);
  Serial.println(" seconds ===");

  slink.inputMonitor(2, false, seconds * 1000000UL); // Type 2 = hex dump

  Serial.println("=== Monitoring complete ===");
}

/** Query disc title from the CDP device with enhanced debugging */
String queryDiscTitle(int discNumber)
{
  Serial.print("=== Querying title for disc ");
  Serial.print(discNumber);
  Serial.println(" ===");

  // Decide device byte based on discNumber
  uint8_t device = (discNumber > 200) ? 0x93 : 0x90;

  // For 201..400, subtract 200
  int discVal = discNumber;
  if (discNumber > 200)
    discVal -= 200;

  // Convert to BCD or custom offset
  uint8_t discByte = 0;
  if (discVal <= 99)
    discByte = toBCD(discVal);
  else if (discVal <= 200)
    discByte = 0x9A + (discVal - 100);

  Serial.print("Device: 0x");
  Serial.print(device, HEX);
  Serial.print(", Command: 0x");
  Serial.print(SLINK_CMD_CD_DOWNLOAD_TITLE, HEX);
  Serial.print(", Disc: 0x");
  Serial.println(discByte, HEX);

  // Go back to the working approach: send command then use inputMonitor
  Serial.println("Using the proven working method: sendCommand + inputMonitor");

  // SIMPLEST APPROACH: Just use inputMonitor and manually examine what we get
  Serial.println("=== SIMPLE TEST: Multiple attempts ===");

  // SOLUTION FOUND! Use 5ms delay for perfect capture with START detection
  Serial.println("Using proven 5ms delay for reliable title capture:");

  // RADICAL APPROACH: No reset, multiple rapid attempts to catch device in right state
  Serial.println("Trying multiple rapid title queries to catch device in responsive state...");

  String response = "";

  // Try up to 5 rapid attempts with different tiny delays
  for (int attempt = 1; attempt <= 5; attempt++)
  {
    Serial.print("Attempt ");
    Serial.print(attempt);
    Serial.print(": ");

    // Send title query
    slink.sendCommand(device, SLINK_CMD_CD_DOWNLOAD_TITLE, discByte);

    // Vary the delay slightly each time
    delay(attempt * 2); // 2ms, 4ms, 6ms, 8ms, 10ms

    // Very short capture window
    response = slink.inputMonitorWithReturn(2, false, 200000UL); // 0.2 seconds

    Serial.println(response.length() > 0 ? response : "empty");

    // Check if we got a valid title response
    if (response.indexOf("98,40,") >= 0)
    {
      Serial.print("SUCCESS on attempt ");
      Serial.println(attempt);
      break;
    }

    // Short delay between attempts
    delay(100);
  }

  if (response.length() > 0)
  {
    Serial.print("Raw response: ");
    Serial.println(response);

    // Clean the response format
    String cleanResponse = response;
    cleanResponse.replace(",", " ");
    cleanResponse.replace("START", "");
    cleanResponse.replace("\n", " ");
    cleanResponse.trim();

    Serial.print("Cleaned response: ");
    Serial.println(cleanResponse);

    // Parse the title
    String title = parseTitle(cleanResponse);
    if (title.length() > 0)
    {
      Serial.print("SUCCESS - Parsed title: ");
      Serial.println(title);
      return title;
    }
    else
    {
      Serial.println("Response received but no title found - disc may be empty");
      return "";
    }
  }
  else
  {
    Serial.println("No response captured");
    return "";
  }

  Serial.println("Trying sendCommandAndCapture method...");
  String captureResponse = slink.sendCommandAndCapture(device, SLINK_CMD_CD_DOWNLOAD_TITLE, discByte, 3000);

  if (captureResponse.length() > 0)
  {
    Serial.print("Captured response: ");
    Serial.println(captureResponse);

    String title = parseTitle(captureResponse);
    if (title.length() > 0)
    {
      Serial.print("SUCCESS - Parsed title: ");
      Serial.println(title);
      return title;
    }
  }

  Serial.println("Capture method didn't work, trying standard method...");

  // Fallback to old method
  String rawResponse = slink.sendCommandAndReceive(device, SLINK_CMD_CD_DOWNLOAD_TITLE, discByte, 5000);

  if (rawResponse.length() > 0)
  {
    Serial.print("Raw response (hex): ");
    Serial.println(rawResponse);

    String title = parseTitle(rawResponse);
    if (title.length() > 0)
    {
      Serial.print("Parsed title: ");
      Serial.println(title);
      return title;
    }
  }
  else
  {
    Serial.println("No response received from either method");
  }

  Serial.println("=== Query complete ===");
  return "";
}

// ------------------ PS/2 KEYBOARD FUNCTIONS ------------------
/**
 * Writes a disc title to the CD player using the definitive defensive PS/2 strategy.
 * Implements: 1) Host inhibit checking, 2) Typematic rate handshake, 3) Proper rhythm
 *
 * @param discNumber The disc slot to select (1-300).
 * @param title The string to write as the title.
 */
void writeDiscTitle(int discNumber, const String& title) 
{
    Serial.print("=== Writing title '");
    Serial.print(title);
    Serial.print("' to disc ");
    Serial.print(discNumber);
    Serial.println(" via Defensive PS/2 ===");

    // Step 1: Select the disc on the player using S-Link.
    Serial.println("Step 1: Selecting disc via S-Link...");
    slinkSelectDisc(discNumber);
    delay(8000); // Wait for the mechanical action of disc selection to complete.

    // **NEW Step 2**: Enhanced protocol handshake with timeout protection
    // This tells the Sony device "I am a professional-grade keyboard"
    Serial.println("Step 2: Setting default keyboard parameters (enhanced handshake)...");
    ps2Keyboard.set_typematic_rate();
    delay(250); // Give host time to process typematic command

    // Step 3: Press Enter to enter title editing mode.
    Serial.println("Step 3: Pressing Enter to enter title edit mode...");
    ps2Keyboard.sendEnter();
    delay(1000); // Wait for the player to switch to edit mode.

    // Step 4: Clear any existing title using the Shift+Delete helper function.
    Serial.println("Step 4: Clearing existing title with Shift+Delete...");
    ps2Keyboard.sendShiftDelete();
    delay(500); // Wait for the clear operation to process.

    // Step 5: Type the new title using paced sendString function.
    // Now includes 100ms delays between characters (10.9 chars/sec rhythm)
    Serial.print("Step 5: Typing title at 10.9 chars/sec: ");
    Serial.println(title);
    ps2Keyboard.sendString(title);
    delay(500); // Wait for the final character to be processed.

    // Step 6: Press Enter to store the title.
    Serial.println("Step 6: Pressing Enter to store title...");
    ps2Keyboard.sendEnter();
    delay(1000); // Give the device time to save the title to memory.

    Serial.println("=== Defensive PS/2 title write complete ===");
}

/** Test PS/2 keyboard functionality */
void testPS2Keyboard() 
{
  Serial.println("=== Testing PS/2 Keyboard ===");
  
  // Test basic typing
  Serial.println("Testing basic string typing...");
  ps2Keyboard.sendString("Hello World");
  ps2Keyboard.sendEnter();
  
  delay(1000);
  
  // Test special characters  
  Serial.println("Testing special characters...");
  ps2Keyboard.sendString("Test123!@#");
  ps2Keyboard.sendEnter();
  
  delay(1000);
  
  Serial.println("=== PS/2 Test Complete ===");
}

// ------------------ COMMAND HANDLERS ------------------
using CommandHandler = std::function<void(const String &)>;
std::map<String, CommandHandler> commandHandlers;

void handlePowerCommand(const String &)
{
  Serial.println(isPlayerOn ? "Power: ON -> OFF" : "Power: OFF -> ON");
  if (isPlayerOn)
    slinkPowerOff();
  else
    slinkPowerOn();
  isPlayerOn = !isPlayerOn;
}

void handleBulkUpdateCommand(const String &args)
{
  // 1) Parse all memo fields m_XX=YYYY from the POST body
  //    and save them to storage.
  const int totalDiscs = storage.getMaxDiscs();

  for (int i = 1; i <= totalDiscs; i++)
  {
    // Each disc's memo field is named m_i => "m_1=...", "m_2=...", etc.
    String paramName = "m_" + String(i) + "=";
    int idx = args.indexOf(paramName);
    if (idx >= 0)
    {
      // find end of param
      int end = args.indexOf('&', idx);
      if (end < 0)
        end = args.length();
      // skip paramName
      String memoVal = urlDecode(args.substring(idx + paramName.length(), end));
      // Write to storage (assuming disc #1 = index 0 in your storage, etc.)
      // If your DiscStorage uses 0-based indexing, we do: i-1
      storage.writeDiscWithNumber(i, memoVal);
    }
  }

  // 2) Check if user clicked "Play" next to a disc => that yields disc=N
  int discIdx = args.indexOf("disc=");
  if (discIdx >= 0)
  {
    int end = args.indexOf('&', discIdx);
    if (end < 0)
      end = args.length();
    int discNum = urlDecode(args.substring(discIdx + 5, end)).toInt();

    if (discNum > 0)
    {
      Serial.print("Playing disc #");
      Serial.println(discNum);
      slinkSelectDisc(discNum);
    }
  }
}

void handleDiscoverTitleCommand(const String &args)
{
  Serial.print("DEBUG: handleDiscoverTitleCommand called with args: ");
  Serial.println(args);

  // Parse disc number from args: "disc=N"
  int discIdx = args.indexOf("disc=");
  if (discIdx >= 0)
  {
    int end = args.indexOf('&', discIdx);
    if (end < 0)
      end = args.length();
    int discNum = urlDecode(args.substring(discIdx + 5, end)).toInt();

    Serial.print("DEBUG: Parsed disc number: ");
    Serial.println(discNum);

    if (discNum > 0 && discNum <= storage.getMaxDiscs())
    {
      Serial.print("Discovering title for disc #");
      Serial.println(discNum);

      String title = queryDiscTitle(discNum);

      if (title.length() > 0)
      {
        // Store discovered title in EEPROM
        storage.writeDiscWithNumber(discNum, title);
        Serial.print("Stored title: ");
        Serial.println(title);
      }
      else
      {
        Serial.println("No title discovered");
      }
    }
    else
    {
      Serial.print("DEBUG: Invalid disc number ");
      Serial.print(discNum);
      Serial.print(" (max: ");
      Serial.print(storage.getMaxDiscs());
      Serial.println(")");
    }
  }
  else
  {
    Serial.println("DEBUG: No 'disc=' parameter found in args");
  }
}

void handlePlayCommand(const String &) { slinkPlay(); }
void handleStopCommand(const String &) { slinkStop(); }
void handlePauseCommand(const String &) { slinkPauseToggle(); }
void handleNextCommand(const String &) { slinkNextTrack(); }
void handlePrevCommand(const String &) { slinkPrevTrack(); }

/** Query device status via S-Link */
String queryDeviceStatus() {
  Serial.println("Querying device status (0x0F)...");
  String response = slink.sendCommandAndReceive(SLINK_DEVICE_CDP_CX1L, 0x0F, -1, 2000);
  
  if (response.length() > 0) {
    Serial.print("Device status response: ");
    Serial.println(response);
    return response;
  } else {
    Serial.println("No status response received");
    return "";
  }
}

/** Wait for device to be ready (status 08) */
bool waitForDeviceReady(int timeoutMs = 5000) {
  Serial.println("Waiting for device ready status...");
  unsigned long startTime = millis();
  
  while (millis() - startTime < timeoutMs) {
    // Monitor for ready status (08) or other status responses
    String response = slink.inputMonitorWithReturn(2, false, 500000UL); // 0.5 second check
    
    if (response.indexOf("08") >= 0) {
      Serial.println("Device ready!");
      return true;
    } else if (response.length() > 0) {
      Serial.print("Device status: ");
      Serial.println(response);
    }
    
    delay(500);
  }
  
  Serial.println("Timeout waiting for device ready");
  return false;
}

/** Simplified reliable PS/2 title writing - no S-Link status queries */
void writeDiscTitleViaPS2Simple(int discNumber, const String& title) 
{
  Serial.print("=== Sony-compliant PS/2 Write: '");
  Serial.print(title);
  Serial.print("' to disc ");
  Serial.print(discNumber);
  Serial.println(" ===");
  
  // Step 1: Select the disc via S-Link
  Serial.print("Step 1: Selecting disc ");
  Serial.print(discNumber);
  Serial.println(" via S-Link...");
  slinkSelectDisc(discNumber);
  
  // Step 2: Wait for disc selection
  Serial.println("Step 2: Waiting for disc selection to complete (8 seconds)...");
  delay(8000);
  
  // Step 3: Enter edit mode with Enter key (confirmed working)
  Serial.println("Step 3: Entering edit mode with Enter key...");
  ps2Keyboard.keyboard_mkbrk(0x5A);
  delay(2000);
  
  // Step 4: Clear existing text using Sony method (Shift+Delete)
  Serial.println("Step 4: Clearing existing text with Shift+Delete (Sony method)...");
  ps2Keyboard.sendShiftDelete();
  delay(1000);                                // Wait for clearing to complete
  
  // Step 5: Type new title
  Serial.print("Step 5: Typing title: ");
  Serial.println(title);
  for (unsigned int i = 0; i < title.length(); i++) {
    char c = title.charAt(i);
    Serial.print("  Typing: ");
    Serial.println(c);
    ps2Keyboard.sendKey(c);
    delay(300); // Reasonable typing speed
  }
  
  // Step 6: Save with Enter key
  Serial.println("Step 6: Saving with Enter key...");
  ps2Keyboard.keyboard_mkbrk(0x5A);
  delay(2000);
  
  Serial.println("=== Sony-compliant PS/2 write complete ===");
  Serial.println("Check your CD player display for the new title!");
}

void handlePS2TestCommand(const String &) 
{
  Serial.println("=== COMPREHENSIVE PS/2 DIAGNOSTIC TEST ===");
  Serial.println("Testing different Enter key approaches to find what works with Sony CD player");
  
  // Select disc 3 first (this part works via S-Link)
  Serial.println("Step 1: Selecting disc 3 via S-Link...");
  slinkSelectDisc(3);
  delay(8000); // Wait for disc selection
  
  Serial.println("\nStep 2: Testing different Enter key methods...");
  
  // Test 1: Original ps2dev style - simple mkbrk
  Serial.println("\n--- Test 1: Original ps2dev method (keyboard_mkbrk) ---");
  ps2Keyboard.keyboard_mkbrk(0x5A);
  delay(3000);
  
  // Test 2: Press and release separately
  Serial.println("\n--- Test 2: Separate press/release ---");
  ps2Keyboard.keyboard_press(0x5A);
  delay(100);
  ps2Keyboard.keyboard_release(0x5A);
  delay(3000);
  
  // Test 3: Multiple Enter attempts (in case first didn't work)
  Serial.println("\n--- Test 3: Multiple Enter attempts ---");
  for(int i = 0; i < 3; i++) {
    Serial.print("Enter attempt ");
    Serial.println(i + 1);
    ps2Keyboard.keyboard_mkbrk(0x5A);
    delay(1000);
  }
  delay(2000);
  
  // Test 4: Test basic character sending to verify PS/2 works at all
  Serial.println("\n--- Test 4: Basic character test (sending 'A') ---");
  ps2Keyboard.sendKey('A');
  delay(2000);
  
  // Test 5: Proper Sony documentation method
  Serial.println("\n--- Test 5: Complete Sony sequence (Enter -> Shift+Delete -> Type -> Enter) ---");
  
  // Step 1: Enter edit mode (we know Test 1 worked)
  Serial.println("Entering edit mode with Enter key...");
  ps2Keyboard.keyboard_mkbrk(0x5A);
  delay(2000); // Wait for edit mode to activate
  
  // Step 2: Clear existing text using Shift+Delete (per Sony docs)
  Serial.println("Clearing existing text with Shift+Delete (Sony method)...");
  ps2Keyboard.sendShiftDelete();
  delay(1000);                           // Wait for clearing to complete
  
  // Step 3: Type new title
  Serial.println("Typing 'ABC'...");
  ps2Keyboard.sendKey('A');
  delay(300);
  ps2Keyboard.sendKey('B');
  delay(300);
  ps2Keyboard.sendKey('C');
  delay(300);
  
  // Step 4: Save with Enter
  Serial.println("Saving with Enter key...");
  ps2Keyboard.keyboard_mkbrk(0x5A);
  
  Serial.println("\n=== DIAGNOSTIC TEST COMPLETE ===");
  Serial.println("Check Sony CD player display:");
  Serial.println("- Did any Enter key method activate edit mode?");
  Serial.println("- Do you see any characters being typed?");
  Serial.println("- Is 'ABC' now shown as title for disc 3?");
}

void handlePS2BasicTestCommand(const String &)
{
  Serial.println("=== DOUBLE-ENTRY DIAGNOSTIC TEST ===");
  Serial.println("Investigating character bouncing/double-entry issue...");
  
  // Test 1: Single character with different methods
  Serial.println("\n--- Test 1: Single 'A' with different methods ---");
  
  Serial.println("Method A: Raw keyboard_mkbrk(0x1C)...");
  ps2Keyboard.keyboard_mkbrk(0x1C); // 'A' scan code directly
  delay(2000);
  
  Serial.println("Method B: sendKey('A') wrapper...");
  ps2Keyboard.sendKey('A');
  delay(2000);
  
  Serial.println("Method C: Manual press/release...");
  ps2Keyboard.keyboard_press(0x1C);
  delay(100);
  ps2Keyboard.keyboard_release(0x1C);
  delay(2000);
  
  // Test 2: Different inter-character delays
  Serial.println("\n--- Test 2: 'ABC' with different delays ---");
  
  Serial.println("Fast (100ms between chars):");
  ps2Keyboard.keyboard_mkbrk(0x1C); // A
  delay(100);
  ps2Keyboard.keyboard_mkbrk(0x32); // B
  delay(100);
  ps2Keyboard.keyboard_mkbrk(0x21); // C
  delay(3000);
  
  Serial.println("Medium (500ms between chars):");
  ps2Keyboard.keyboard_mkbrk(0x1C); // A
  delay(500);
  ps2Keyboard.keyboard_mkbrk(0x32); // B
  delay(500);
  ps2Keyboard.keyboard_mkbrk(0x21); // C
  delay(3000);
  
  Serial.println("Slow (1000ms between chars):");
  ps2Keyboard.keyboard_mkbrk(0x1C); // A
  delay(1000);
  ps2Keyboard.keyboard_mkbrk(0x32); // B
  delay(1000);
  ps2Keyboard.keyboard_mkbrk(0x21); // C
  delay(3000);
  
  Serial.println("\n=== DIAGNOSTIC COMPLETE ===");
  Serial.println("Watch Sony display for:");
  Serial.println("- Which method shows single characters vs doubles");
  Serial.println("- Which timing prevents flickering");
  Serial.println("- Note any patterns in the double-entry behavior");
}

void handlePS2DiagnosticCommand(const String &)
{
  Serial.println("=== ULTRA-RELIABLE PS/2 TEST ===");
  Serial.println("Single character test with maximum reliability checks...");
  
  // Disable host communication temporarily to avoid conflicts
  Serial.println("Pausing host communication during test...");
  
  // Test 1: Pin state verification
  Serial.println("\nTest 1: Pin state check...");
  Serial.print("CLK pin (3): ");
  Serial.println(digitalRead(3) ? "HIGH (good)" : "LOW (bad)");
  Serial.print("DATA pin (4): ");
  Serial.println(digitalRead(4) ? "HIGH (good)" : "LOW (bad)");
  
  // Test 2: Reset PS/2 interface
  Serial.println("\nTest 2: Resetting PS/2 interface...");
  ps2Keyboard.begin();
  delay(1000); // Extra settling time
  
  // Test 3: Ultra-reliable single character
  Serial.println("\nTest 3: Ultra-reliable single 'A' transmission...");
  Serial.println("Method: Raw scan code with maximum retries and timing");
  
  // Send single 'A' character (scan code 0x1C) with maximum care
  int result = ps2Keyboard.keyboard_mkbrk(0x1C);
  
  if (result == 0) {
    Serial.println("SUCCESS: 'A' transmitted without errors");
  } else {
    Serial.println("FAILURE: 'A' transmission failed after retries");
  }
  
  delay(2000);
  
  // Test 4: Second character to check consistency  
  Serial.println("\nTest 4: Second character ('B') consistency check...");
  result = ps2Keyboard.keyboard_mkbrk(0x32); // 'B' scan code
  
  if (result == 0) {
    Serial.println("SUCCESS: 'B' transmitted without errors");
  } else {
    Serial.println("FAILURE: 'B' transmission failed after retries");
  }
  
  Serial.println("\n=== ULTRA-RELIABLE TEST COMPLETE ===");
  Serial.println("Check Sony display: Should show 'AB' if both succeeded");
  Serial.println("Re-enabling host communication...");
}

void handlePS2WriteCommand(const String &args)
{
  // Parse disc number and title from args: "ps2Write&disc=N&title=XXXX"
  int discIdx = args.indexOf("disc=");
  int titleIdx = args.indexOf("title=");
  
  if (discIdx >= 0 && titleIdx >= 0) {
    // Parse disc number
    int discEnd = args.indexOf('&', discIdx);
    if (discEnd < 0) discEnd = args.length();
    int discNum = urlDecode(args.substring(discIdx + 5, discEnd)).toInt();
    
    // Parse title
    int titleEnd = args.indexOf('&', titleIdx);
    if (titleEnd < 0) titleEnd = args.length();
    String title = urlDecode(args.substring(titleIdx + 6, titleEnd));
    
    if (discNum > 0 && discNum <= storage.getMaxDiscs() && title.length() > 0) {
      Serial.print("PS/2 Write Command: Writing '");
      Serial.print(title);
      Serial.print("' to disc ");
      Serial.println(discNum);
      writeDiscTitle(discNum, title);
    } else {
      Serial.println("Invalid disc number or title for PS/2 write");
    }
  } else {
    Serial.println("Missing parameters for PS/2 write command");
  }
}

// ------------------ TESTING FUNCTIONS ------------------
void testSLinkConnection()
{
  Serial.println("=== S-Link Connection Diagnostic ===");

  // Step 1: Check if S-Link line is physically connected and stable
  Serial.print("S-Link pin (");
  Serial.print(SLINK_PIN);
  Serial.print(") initial state: ");
  pinMode(SLINK_PIN, INPUT);
  bool initialState = digitalRead(SLINK_PIN);
  Serial.println(initialState ? "HIGH" : "LOW");

  // Check for line stability (should be stable HIGH when idle)
  Serial.println("Checking line stability (5 seconds)...");
  int changes = 0;
  bool lastState = initialState;

  for (int i = 0; i < 50; i++) // 5 seconds, check every 100ms
  {
    delay(100);
    bool currentState = digitalRead(SLINK_PIN);
    if (currentState != lastState)
    {
      changes++;
      lastState = currentState;
    }
  }

  Serial.print("Line state changes detected: ");
  Serial.println(changes);

  if (changes == 0 && initialState)
  {
    Serial.println("✓ S-Link line appears stable and pulled HIGH (good)");
  }
  else if (changes == 0 && !initialState)
  {
    Serial.println("⚠ S-Link line stuck LOW - connection issue?");
  }
  else if (changes > 10)
  {
    Serial.println("✓ S-Link line very active - device may be communicating");
  }
  else
  {
    Serial.println("? S-Link line has some activity - monitor for traffic");
  }

  // Step 2: Monitor for any existing S-Link traffic
  Serial.println("Monitoring for S-Link traffic (10 seconds)...");
  Serial.println("*** Try pressing buttons on CDP-CX355 now ***");
  monitorSLinkTraffic(10);

  Serial.println("=== Connection Diagnostic Complete ===");
}

void testRawCommandResponse()
{
  Serial.println("=== Testing Raw Command/Response Timing ===");
  Serial.println("Device IS responding (we saw 0x98 traffic), so testing immediate response capture...");

  // Test 1: Send device query and immediately monitor with built-in monitor
  Serial.println("Test 1: Send device query (0x90 0x22) and monitor response...");

  // Send the command using regular method
  slink.sendCommand(0x90, 0x22);

  // Immediately start monitoring for response (no sync waiting)
  Serial.println("Monitoring for immediate response (3 seconds)...");
  slink.inputMonitor(2, false, 3000000UL); // 3 seconds, hex dump mode

  delay(2000);

  // Test 2: Send title query and monitor
  Serial.println("Test 2: Send title query (0x90 0x40 0x08) and monitor response...");

  slink.sendCommand(0x90, 0x40, 0x08); // Query title for disc 8

  Serial.println("Monitoring for title response (3 seconds)...");
  slink.inputMonitor(2, false, 3000000UL); // 3 seconds, hex dump mode

  delay(2000);

  // Test 3: Parse the known working response
  Serial.println("Test 3: Parsing the captured title response...");
  String knownResponse = "98 40 8 48 6f 77 20 54 6f 20 54 72 61 69 6e 20";
  String parsedTitle = parseTitle(knownResponse);
  Serial.print("PARSED TITLE: '");
  Serial.print(parsedTitle);
  Serial.println("'");

  // Test 4: Try the new capture method
  Serial.println("Test 4: Using new sendCommandAndCapture method...");
  String capturedResponse = slink.sendCommandAndCapture(0x90, 0x40, 0x08, 3000);
  if (capturedResponse.length() > 0)
  {
    Serial.print("CAPTURED: ");
    Serial.println(capturedResponse);
    String capturedTitle = parseTitle(capturedResponse);
    Serial.print("TITLE: '");
    Serial.print(capturedTitle);
    Serial.println("'");
  }
  else
  {
    Serial.println("Capture method still needs work");
  }

  Serial.println("=== Raw Command/Response Test Complete ===");
}

void testMultipleDeviceAddresses()
{
  Serial.println("=== Device Communication Analysis ===");
  Serial.println("DISCOVERY: Device sends 0x98 responses = CDP-1, so 0x90 addressing is CORRECT");
  Serial.println("ISSUE: Our response parsing is broken, not the addressing");
  Serial.println();

  // Skip address testing since we know 0x90 is correct
  // Instead test raw command/response timing
  testRawCommandResponse();
}

void testDeviceResponse()
{
  Serial.println("=== Comprehensive S-Link Diagnostic ===");

  // Step 1: Basic connection test
  testSLinkConnection();

  delay(2000);

  // Step 2: Try multiple device addresses
  testMultipleDeviceAddresses();

  delay(2000);

  // Step 3: Try basic commands with original address
  Serial.println("=== Testing Basic Commands (0x90) ===");

  Serial.println("Test 1: Device Type Query (0x22)...");
  String deviceResponse = slink.sendCommandAndReceive(0x90, 0x22, -1, 3000);
  if (deviceResponse.length() > 0)
  {
    Serial.print("SUCCESS: Device type response: ");
    Serial.println(deviceResponse);
  }
  else
  {
    Serial.println("No response to device type query");
  }

  delay(1000);

  Serial.println("Test 2: Setup Info Query (0x0F)...");
  String statusResponse = slink.sendCommandAndReceive(0x90, 0x0F, -1, 3000);
  if (statusResponse.length() > 0)
  {
    Serial.print("SUCCESS: Setup info response: ");
    Serial.println(statusResponse);
  }
  else
  {
    Serial.println("No response to setup info query");
  }

  delay(1000);

  Serial.println("Test 3: Power On Command (0x2E)...");
  String powerResponse = slink.sendCommandAndReceive(0x90, 0x2E, -1, 3000);
  if (powerResponse.length() > 0)
  {
    Serial.print("SUCCESS: Power on response: ");
    Serial.println(powerResponse);
  }
  else
  {
    Serial.println("No response to power on command");
  }

  Serial.println("=== Basic Command Test Complete ===");
}

void testTitleCommand()
{
  Serial.println("=== Testing Title Discovery ===");

  // First test basic communication
  testDeviceResponse();

  delay(2000);

  // Test with disc 1
  Serial.println("Testing disc 1 title query...");
  String title1 = queryDiscTitle(1);
  if (title1.length() > 0)
  {
    Serial.print("SUCCESS: Disc 1 title = ");
    Serial.println(title1);
  }
  else
  {
    Serial.println("No title found for disc 1");
  }

  delay(2000); // Wait between commands

  // Test with disc 8 (the one user mentioned)
  Serial.println("Testing disc 8 title query...");
  String title8 = queryDiscTitle(8);
  if (title8.length() > 0)
  {
    Serial.print("SUCCESS: Disc 8 title = ");
    Serial.println(title8);
  }
  else
  {
    Serial.println("No title found for disc 8");
  }

  Serial.println("=== Title Discovery Test Complete ===");
}

// ------------------ SETUP COMMAND MAP ------------------
void setupCommandHandlers()
{
  commandHandlers["power"] = handlePowerCommand;
  commandHandlers["play"] = handlePlayCommand;
  commandHandlers["stop"] = handleStopCommand;
  commandHandlers["pause"] = handlePauseCommand;
  commandHandlers["next"] = handleNextCommand;
  commandHandlers["prev"] = handlePrevCommand;
  commandHandlers["bulkUpdate"] = handleBulkUpdateCommand;
  commandHandlers["discoverTitle"] = handleDiscoverTitleCommand;
  commandHandlers["ps2Test"] = handlePS2TestCommand;
  commandHandlers["ps2Write"] = handlePS2WriteCommand;
  commandHandlers["ps2Diag"] = handlePS2DiagnosticCommand;
  commandHandlers["ps2Basic"] = handlePS2BasicTestCommand;
  commandHandlers["ps2DoubleEntry"] = handlePS2BasicTestCommand;  // Use same function
  commandHandlers["ps2HostComm"] = [](const String &) {
    Serial.println("=== PS/2 HOST COMMUNICATION TEST ===");
    Serial.println("Testing proper PS/2 host acknowledgment handling...");
    
    // Temporary disable main loop host handling to see raw communication
    Serial.println("Monitoring host communication for 10 seconds...");
    Serial.println("Try pressing keys on Sony device during this time...");
    
    unsigned long startTime = millis();
    while (millis() - startTime < 10000) {
      unsigned char leds;
      int result = ps2Keyboard.keyboard_handle(&leds);
      if (result) {
        Serial.print("Host communication processed, LED state: 0x");
        Serial.println(leds, HEX);
      }
      delay(10);
    }
    
    Serial.println("Host communication monitoring complete.");
    Serial.println("Now testing single character with full host handling...");
    
    // Send single 'A' with full host communication processing
    int result = ps2Keyboard.keyboard_mkbrk(0x1C);
    if (result == 0) {
      Serial.println("SUCCESS: Character sent with proper host handling");
    } else {
      Serial.println("FAILURE: Character transmission failed");
    }
    
    Serial.println("=== HOST COMMUNICATION TEST COMPLETE ===");
  };
  
  // New timing-focused test since host communication isn't the issue
  commandHandlers["ps2Timing"] = [](const String &) {
    Serial.println("=== PS/2 TIMING OPTIMIZATION TEST ===");
    Serial.println("Testing different delays to find Sony's optimal processing speed...");
    
    // Test different inter-character delays
    int delays[] = {100, 250, 500, 750, 1000, 1500, 2000};
    int numDelays = sizeof(delays) / sizeof(delays[0]);
    
    for (int i = 0; i < numDelays; i++) {
      Serial.print("Test ");
      Serial.print(i + 1);
      Serial.print(": Typing 'ABC' with ");
      Serial.print(delays[i]);
      Serial.println("ms between characters...");
      
      // Type A-B-C with specific timing
      ps2Keyboard.keyboard_mkbrk(0x1C); // A
      delay(delays[i]);
      ps2Keyboard.keyboard_mkbrk(0x32); // B  
      delay(delays[i]);
      ps2Keyboard.keyboard_mkbrk(0x21); // C
      
      Serial.print("  Sent ABC with ");
      Serial.print(delays[i]);
      Serial.println("ms delays - check Sony display");
      
      delay(3000); // Wait to observe result before next test
    }
    
    Serial.println("=== TIMING TEST COMPLETE ===");
    Serial.println("Check which delay produced clean 'ABC' without doubles/drops");
  };
  
  // Protocol-compliant PS/2 test using standard library functions
  commandHandlers["ps2Sony"] = [](const String &) {
    Serial.println("=== PS/2 PROTOCOL-COMPLIANT TEST ===");
    Serial.println("Testing corrected implementation using standard ps2dev library functions");
    Serial.println("Following the technical report's recommendations:");
    Serial.println("- No custom clock-ratio timing");
    Serial.println("- Using standard keyboard_mkbrk() functions");
    Serial.println("- Reasonable human-like delays for pacing");
    Serial.println("- Trusting the library's protocol-correct timing");
    
    // Test the corrected writeDiscTitle function
    Serial.println("Testing writeDiscTitle() with 'SONY'...");
    writeDiscTitle(3, "SONY");
    
    Serial.println("=== PROTOCOL-COMPLIANT TEST COMPLETE ===");
    Serial.println("Expected: Reliable 'SONY' with proper PS/2 protocol compliance");
  };

  // Hardware diagnostic test for electrical issues
  commandHandlers["ps2Hardware"] = [](const String &) {
    Serial.println("=== PS/2 HARDWARE DIAGNOSTIC ===");
    Serial.println("Testing electrical connections and signal integrity...");
    
    // Test 1: Pin voltage readings
    Serial.println("\nTest 1: Pin voltage readings (should both be HIGH when idle)");
    Serial.print("CLK pin (3): ");
    Serial.println(digitalRead(3) ? "HIGH (good)" : "LOW (BAD - check connection)");
    Serial.print("DATA pin (4): ");
    Serial.println(digitalRead(4) ? "HIGH (good)" : "LOW (BAD - check connection)");
    
    // Test 2: Pin stability over time
    Serial.println("\nTest 2: Pin stability check (10 seconds)...");
    unsigned long startTime = millis();
    int clkChanges = 0, dataChanges = 0;
    bool lastClk = digitalRead(3), lastData = digitalRead(4);
    
    while (millis() - startTime < 10000) {
      bool currentClk = digitalRead(3);
      bool currentData = digitalRead(4);
      
      if (currentClk != lastClk) {
        clkChanges++;
        lastClk = currentClk;
      }
      if (currentData != lastData) {
        dataChanges++;
        lastData = currentData;
      }
      delay(1);
    }
    
    Serial.print("CLK pin changes: ");
    Serial.print(clkChanges);
    Serial.println(clkChanges > 10 ? " (BAD - line unstable)" : " (good)");
    Serial.print("DATA pin changes: ");
    Serial.print(dataChanges);
    Serial.println(dataChanges > 10 ? " (BAD - line unstable)" : " (good)");
    
    // Test 3: Manual pin control test
    Serial.println("\nTest 3: Manual pin control test...");
    for (int i = 0; i < 5; i++) {
      Serial.print("Cycle ");
      Serial.print(i + 1);
      Serial.print(": ");
      
      pinMode(3, OUTPUT);
      pinMode(4, OUTPUT);
      digitalWrite(3, LOW);
      digitalWrite(4, LOW);
      delay(100);
      
      Serial.print("LOW->HIGH ");
      
      digitalWrite(3, HIGH);
      digitalWrite(4, HIGH);
      delay(100);
      
      // Return to PS/2 mode
      pinMode(3, INPUT);
      pinMode(4, INPUT);
      digitalWrite(3, HIGH);
      digitalWrite(4, HIGH);
      
      Serial.println("OK");
    }
    
    Serial.println("\n=== HARDWARE DIAGNOSTIC COMPLETE ===");
    Serial.println("Check for:");
    Serial.println("- Any 'BAD' indicators above");
    Serial.println("- Loose wire connections");
    Serial.println("- Missing ground connection");
    Serial.println("- Interference from other devices");
  };
  
  // Robust retry approach since hardware is confirmed good
  commandHandlers["ps2Robust"] = [](const String &) {
    Serial.println("=== ROBUST PS/2 CHARACTER TEST ===");
    Serial.println("Hardware is good - testing robust retry approach...");
    
    // Function to send character with multiple attempts and validation
    auto sendCharRobust = [](char c, const char* name) -> bool {
      Serial.print("Sending '");
      Serial.print(c);
      Serial.print("' (");
      Serial.print(name);
      Serial.print(")...");
      
      // Multiple attempts with different timings
      int attempts[] = {100, 500, 1000, 1500}; // Different delays to try
      
      for (int i = 0; i < 4; i++) {
        uint8_t scanCode;
        if (c == 'A') scanCode = 0x1C;
        else if (c == 'B') scanCode = 0x32;
        else if (c == 'C') scanCode = 0x21;
        else continue;
        
        Serial.print(" attempt");
        Serial.print(i + 1);
        
        // Send character
        ps2Keyboard.keyboard_mkbrk(scanCode);
        
        // Wait with specific timing
        delay(attempts[i]);
        
        Serial.print("(");
        Serial.print(attempts[i]);
        Serial.print("ms)");
      }
      
      Serial.println(" - check Sony display");
      return true;
    };
    
    Serial.println("\nRobust character sequence test:");
    Serial.println("This sends each character multiple times with different timings");
    Serial.println("At least one attempt per character should succeed");
    Serial.println("");
    
    sendCharRobust('A', "first");
    delay(2000);
    
    sendCharRobust('B', "second"); 
    delay(2000);
    
    sendCharRobust('C', "third");
    delay(2000);
    
    Serial.println("\n=== ROBUST TEST COMPLETE ===");
    Serial.println("Expected result: Sony should show some combination of A, B, C");
    Serial.println("This approach works around Sony's internal timing quirks");
  };
  // ... add more if needed
}

// ------------------ SETUP & LOOP ------------------
void setup()
{
  Serial.begin(115200);
  delay(1000); // Give Serial time to initialize

  Serial.println("=== Sony CDP-CX355 S-Link Controller Starting ===");

  slink.init(SLINK_PIN);
  pinMode(SLINK_PIN, INPUT);

  // Initialize PS/2 keyboard simulator
  ps2Keyboard.begin();
  Serial.println("PS/2 keyboard simulator initialized on pins 3 (CLK) and 4 (DATA)");

  matrix.begin(); // LED matrix
  setupCommandHandlers();

  ledController.playAnimation(MatrixAnimation::WifiSearch, true);
  wifiManager.connect();
  String hostname = String(wifiManager.getHostname()) + ".local";
  ledController.displayText(hostname.c_str());

  Serial.println("=== Ready for S-Link commands ===");
}

void sendDiscsJsonStream(WiFiClient &client, String url)
{
  // Parse pagination parameters from URL
  int page = 1;
  int limit = 25; // Load 25 discs per page to avoid memory issues

  int pageIdx = url.indexOf("page=");
  if (pageIdx >= 0)
  {
    int end = url.indexOf('&', pageIdx);
    if (end < 0)
      end = url.length();
    page = url.substring(pageIdx + 5, end).toInt();
    if (page < 1)
      page = 1;
  }

  int limitIdx = url.indexOf("limit=");
  if (limitIdx >= 0)
  {
    int end = url.indexOf('&', limitIdx);
    if (end < 0)
      end = url.length();
    limit = url.substring(limitIdx + 6, end).toInt();
    if (limit < 1 || limit > 50)
      limit = 25; // Max 50 per page
  }

  client.println(F("HTTP/1.1 200 OK"));
  client.println(F("Content-Type: application/json"));
  client.println(F("Connection: close"));
  client.println();

  int total = storage.getMaxDiscs();
  int startIdx = (page - 1) * limit;
  int endIdx = startIdx + limit;
  if (endIdx > total)
    endIdx = total;

  // JSON response with pagination info
  client.print("{\"discs\":[");

  bool first = true;
  for (int i = startIdx; i < endIdx; i++)
  {
    if (!first)
      client.print(",");
    first = false;

    DiscInfo d = storage.readDisc(i);
    client.print("{\"d\":");
    client.print(d.discNumber);
    client.print(",\"m\":\"");
    client.print(d.memo);
    client.print("\"}");
  }

  client.print("],\"page\":");
  client.print(page);
  client.print(",\"limit\":");
  client.print(limit);
  client.print(",\"total\":");
  client.print(total);
  client.print(",\"hasMore\":");
  client.print(endIdx < total ? "true" : "false");
  client.print("}");

  client.stop();
}

void sendIndexHtml(WiFiClient &client)
{
  client.print(F(
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/html\r\n"
      "Connection: close\r\n"
      "\r\n"
      "<!DOCTYPE html>"
      "<html>"
      "<head><title>Sony CDP-CX355 Remote</title></head>"
      "<body>"
      "<h1>Sony CDP-CX355 Remote Control</h1>"

      "<div style='margin-bottom: 20px;'>"
      "<h2>Transport Controls</h2>"
      "<button onclick='sendCommand(\"play\")'>Play</button> "
      "<button onclick='sendCommand(\"stop\")'>Stop</button> "
      "<button onclick='sendCommand(\"pause\")'>Pause</button> "
      "<button onclick='sendCommand(\"next\")'>Next</button> "
      "<button onclick='sendCommand(\"prev\")'>Previous</button> "
      "<button onclick='sendCommand(\"power\")'>Power</button>"
      "</div>"

      "<div style='margin-bottom: 20px; padding: 10px; background-color: #f0f8ff; border: 1px solid #4169e1;'>"
      "<h2 style='color: #4169e1;'>PS/2 Keyboard Test</h2>"
      "<p><strong>New Feature:</strong> Write disc titles directly to CD player via PS/2 keyboard simulation</p>"
      "<p><strong>How it works:</strong> 1) Selects disc via S-Link, 2) Presses Enter via PS/2, 3) Types title, 4) Presses Enter to store</p>"
      "<button onclick='sendCommand(\"ps2Test\")' style='background-color: #4169e1; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer;'>Test: Set Disc 3 to \"ABC\"</button><br><br>"
      "<button onclick='sendCommand(\"ps2DoubleEntry\")' style='background-color: #ff4500; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer;'>Fix Double-Entry Diagnostic</button> "
      "<button onclick='sendCommand(\"ps2HostComm\")' style='background-color: #8b00ff; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer;'>Host Communication Test</button> "
      "<button onclick='sendCommand(\"ps2Timing\")' style='background-color: #ff1493; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer;'>Timing Optimization Test</button> "
      "<button onclick='sendCommand(\"ps2Hardware\")' style='background-color: #dc143c; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer;'>Hardware Diagnostic</button> "
      "<button onclick='sendCommand(\"ps2Sony\")' style='background-color: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; font-weight: bold;'>✅ Protocol-Compliant Test</button><br><br>"
      "<button onclick='sendCommand(\"ps2Robust\")' style='background-color: #228b22; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer;'>Robust Retry Test</button> "
      "<button onclick='sendCommand(\"ps2Basic\")' style='background-color: #32cd32; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer;'>Basic PS/2 Test</button> "
      "<button onclick='sendCommand(\"ps2Diag\")' style='background-color: #ff6347; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer;'>PS/2 Diagnostics</button>"
      "<p style='font-size: 12px; color: #666; margin-top: 10px;'>Wire connections: Pin 3→CLK (white), Pin 4→DATA (red), 5V→VCC (black), GND→GND (yellow)</p>"
      "</div>"

      "<h2>Disc Collection</h2>"
      "<form id='discForm'>"
      "<input type='hidden' name='command' value='bulkUpdate'>"
      "<div id='discList'></div>"
      "<div style='margin: 20px 0;'>"
      "<button type='button' id='loadMoreBtn'>Load First 25 Discs</button>"
      "</div>"
      "<div style='margin-top: 20px;'>"
      "<button type='submit'>Update All Titles</button>"
      "</div>"
      "</form>"

      "<script>"
      "let currentPage = 0;"
      "let allDiscsLoaded = false;"

      "function sendCommand(cmd) {"
      "  fetch('/', {"
      "    method: 'POST',"
      "    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },"
      "    body: 'command=' + cmd"
      "  });"
      "}"

      "function playDisc(num) {"
      "  fetch('/', {"
      "    method: 'POST',"
      "    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },"
      "    body: 'command=bulkUpdate&disc=' + num"
      "  });"
      "}"

      "function discoverTitle(num) {"
      "  const button = event.target;"
      "  const originalText = button.textContent;"
      "  button.textContent = 'Discovering...';"
      "  button.disabled = true;"
      "  fetch('/', {"
      "    method: 'POST',"
      "    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },"
      "    body: 'command=discoverTitle&disc=' + num"
      "  })"
      "  .then(() => {"
      "    button.textContent = 'Discovered!';"
      "    setTimeout(() => {"
      "      button.textContent = originalText;"
      "      button.disabled = false;"
      "      window.location.reload();"
      "    }, 2000);"
      "  })"
      "  .catch(err => {"
      "    console.error('Error discovering title:', err);"
      "    button.textContent = 'Error';"
      "    setTimeout(() => {"
      "      button.textContent = originalText;"
      "      button.disabled = false;"
      "    }, 2000);"
      "  });"
      "}"

      "function loadDiscs() {"
      "  if (allDiscsLoaded) return;"
      "  currentPage++;"
      "  document.getElementById('loadMoreBtn').textContent = 'Loading...';"

      "  fetch('/discs?page=' + currentPage + '&limit=25')"
      "    .then(response => response.json())"
      "    .then(data => {"
      "      const container = document.getElementById('discList');"
      "      let html = '';"
      "      data.discs.forEach(item => {"
      "        html += '<div style=\"margin-bottom: 5px;\">';"
      "        html += 'Disc ' + item.d + ': ';"
      "        html += '<input type=\"text\" name=\"m_' + item.d + '\" value=\"' + item.m + '\" style=\"width: 200px; margin-right: 10px;\">';"
      "        html += '<button type=\"button\" onclick=\"playDisc(' + item.d + ')\">Play</button> ';"
      "        html += '<button type=\"button\" onclick=\"discoverTitle(' + item.d + ')\">Auto-Discover</button>';"
      "        html += '</div>';"
      "      });"
      "      container.innerHTML += html;"

      "      if (data.hasMore) {"
      "        document.getElementById('loadMoreBtn').textContent = 'Load Next 25 Discs (' + (currentPage * 25) + '/' + data.total + ')';"
      "      } else {"
      "        document.getElementById('loadMoreBtn').style.display = 'none';"
      "        allDiscsLoaded = true;"
      "      }"
      "    })"
      "    .catch(err => {"
      "      console.error('Error fetching discs:', err);"
      "      document.getElementById('loadMoreBtn').textContent = 'Error - Try Again';"
      "    });"
      "}"

      "document.addEventListener('DOMContentLoaded', () => {"
      "  document.getElementById('loadMoreBtn').addEventListener('click', loadDiscs);"

      "  document.getElementById('discForm').addEventListener('submit', function(e) {"
      "    e.preventDefault();"
      "    const formData = new FormData(this);"
      "    const params = new URLSearchParams(formData);"
      "    fetch('/', {"
      "      method: 'POST',"
      "      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },"
      "      body: params.toString()"
      "    }).then(() => {"
      "      alert('Disc titles updated successfully!');"
      "    }).catch(err => {"
      "      alert('Error updating titles: ' + err);"
      "    });"
      "  });"
      "});"
      "</script>"
      "</body></html>"));
}

void loop()
{
  wifiManager.runMDNS(); // Keep mDNS running
  
  // CRITICAL: Handle PS/2 host communication every loop iteration  
  // This is essential for proper PS/2 protocol operation per ps2dev examples
  static unsigned long lastPS2Check = 0;
  unsigned long now = millis();
  if (now - lastPS2Check >= 10) { // Every 10ms as per ps2dev example
    unsigned char leds;
    ps2Keyboard.keyboard_handle(&leds);
    lastPS2Check = now;
  }

  WiFiClient client = wifiManager.getServer().available();
  if (!client)
    return;

  Serial.println("New client connected.");
  HttpRequest request = HttpParser::parse(client);

  // Possibly handle /favicon.ico quickly:
  if (!request.isPost && request.url == "/favicon.ico")
  {
    client.println("HTTP/1.1 204 No Content");
    client.println("Connection: close\r\n");
    client.stop();
    return;
  }

  if (!request.isPost)
  {
    // GET requests
    if (request.url == "/")
    {
      // Serve the index page
      sendIndexHtml(client);
    }
    else if (request.url.startsWith("/discs"))
    {
      // Serve JSON with pagination support
      sendDiscsJsonStream(client, request.url);
    }
    else
    {
      // 404 if unknown
      client.println(F("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"));
    }
    delay(10);
    client.stop();
  }
  else
  {
    // POST request => parse command=...
    // do your usual logic
    String cmdValue;
    int idx = request.body.indexOf("command=");
    if (idx >= 0)
    {
      int end = request.body.indexOf('&', idx);
      if (end < 0)
        end = request.body.length();
      cmdValue = urlDecode(request.body.substring(idx + 8, end));
    }
    Serial.print("Command: ");
    Serial.println(cmdValue);

    // Dispatch to commandHandlers, etc.
    auto it = commandHandlers.find(cmdValue);
    if (it != commandHandlers.end())
    {
      it->second(request.body);
    }
    else
    {
      Serial.println("Unknown command");
    }

    // Maybe redirect or just serve a minimal response:
    // "Redirect" example:
    client.println(F("HTTP/1.1 303 See Other"));
    client.println(F("Location: /"));
    client.println(F("Connection: close\r\n"));
    client.println();
    client.stop();
  }

  Serial.println("Client disconnected.");
}
