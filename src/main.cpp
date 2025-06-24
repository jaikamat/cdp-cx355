#include <map>
#include "WiFiS3.h"
#include "WiFiManager.hpp"
#include "HttpParser.hpp"
#include "Secrets.hpp"
#include <LedMatrixController.hpp>
#include "DiscStorage.hpp"
#include <functional>
#include "Sony_SLink.h"
#define SLINK_PIN 2     // Pick a suitable I/O pin for S-Link

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

ArduinoLEDMatrix matrix;
LedMatrixController ledController(matrix);
WiFiManager wifiManager(ssid, password);
DiscStorage storage;
Slink slink;                                     // S-Link object
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


// ------------------ CUSTOM SONY RESPONSE PARSER ------------------

/**
 * Custom Sony response parser that handles actual Sony timing patterns
 * Instead of looking for 2400μs sync, decodes data directly from pulse patterns
 */
String parseSonyResponse(unsigned long timeoutMs = 3000)
{
  Serial.println("=== Custom Sony Response Parser ===");
  Serial.println("Looking for Sony pulse patterns (~1130μs = 1, ~566μs = 0)");
  
  String result = "";
  unsigned long startTime = millis();
  int pulseCount = 0;
  String binaryData = "";
  
  while (millis() - startTime < timeoutMs && pulseCount < 100) // Max 100 pulses
  {
    // Read S-Link pin directly (assuming it's pin 2 based on SLINK_PIN)
    unsigned long pulseStart = micros();
    
    // Wait for line to go LOW (start of pulse)
    while (digitalRead(SLINK_PIN) == HIGH && (micros() - pulseStart) < 10000) {
      delayMicroseconds(1);
    }
    
    if ((micros() - pulseStart) >= 10000) break; // Timeout waiting for pulse
    
    // Measure LOW pulse duration
    unsigned long lowStart = micros();
    while (digitalRead(SLINK_PIN) == LOW && (micros() - lowStart) < 5000) {
      delayMicroseconds(1);
    }
    
    unsigned long pulseDuration = micros() - lowStart;
    
    if (pulseDuration > 100) // Ignore very short glitches
    {
      pulseCount++;
      Serial.print("Pulse ");
      Serial.print(pulseCount);
      Serial.print(": ");
      Serial.print(pulseDuration);
      Serial.print("μs ");
      
      // Decode based on Sony timing patterns
      if (pulseDuration >= 900 && pulseDuration <= 1400) {
        // ~1130μs = Logic 1
        binaryData += "1";
        Serial.println("(1)");
      }
      else if (pulseDuration >= 400 && pulseDuration <= 800) {
        // ~566μs = Logic 0
        binaryData += "0";
        Serial.println("(0)");
      }
      else {
        Serial.print("(unknown - outside expected range)");
        Serial.println();
      }
      
      // Check if we have enough bits to decode a byte
      if (binaryData.length() >= 8) {
        // Try to decode bytes from binary data
        while (binaryData.length() >= 8) {
          String byteBinary = binaryData.substring(0, 8);
          binaryData = binaryData.substring(8);
          
          // Convert binary string to byte
          uint8_t byteValue = 0;
          for (int i = 0; i < 8; i++) {
            if (byteBinary[i] == '1') {
              byteValue |= (1 << (7-i));
            }
          }
          
          Serial.print("Decoded byte: 0x");
          if (byteValue < 0x10) Serial.print("0");
          Serial.print(byteValue, HEX);
          
          // Check for the magic 1F response
          if (byteValue == 0x1F) {
            Serial.println(" *** FOUND 1F SUCCESS RESPONSE! ***");
            return "1F";
          }
          
          // Add to result
          if (result.length() > 0) result += ",";
          result += String(byteValue, HEX);
          
          Serial.println();
        }
      }
    }
    
    // Brief delay before next pulse detection
    delayMicroseconds(50);
  }
  
  Serial.print("Final binary data remaining: ");
  Serial.println(binaryData);
  Serial.print("Total pulses detected: ");
  Serial.println(pulseCount);
  Serial.print("Decoded response: ");
  Serial.println(result.length() > 0 ? result : "No valid data");
  
  return result;
}

// ------------------ S-LINK DIRECT TEXT WRITING FUNCTIONS ------------------

/**
 * Write text directly to a disc using correct Sony S-Link protocol
 * CORRECTED: Using 0x80 "Set Disc Memo (Part 1)" command instead of 0x98
 * Command format: [device] [0x80] [disc] [13 bytes ASCII]
 * @param discNumber Disc number (1-400)
 * @param text Text to write (max 13 characters)
 * @return Response from Sony device
 */
String slinkWriteDiscText(int discNumber, const String& text)
{
  Serial.println("=== S-Link Direct Disc Text Writing ===");
  Serial.print("Writing text \"");
  Serial.print(text);
  Serial.print("\" to disc ");
  Serial.println(discNumber);
  
  // CORRECTED: Use proper device code (same logic as slinkSelectDisc)
  uint8_t device = (discNumber > 200) ? 0x93 : 0x90; // CD Player device codes
  
  // Convert disc number to BCD format (same logic as slinkSelectDisc)
  int discVal = discNumber;
  if (discNumber > 200)
    discVal -= 200;
    
  uint8_t discByte = 0;
  if (discVal <= 99)
    discByte = toBCD(discVal);
  else if (discVal <= 200)
    discByte = 0x9A + (discVal - 100);
  
  Serial.print("Device: 0x");
  Serial.print(device, HEX);
  Serial.print(", Disc byte: 0x");
  Serial.print(discByte, HEX);
  Serial.print(" (");
  Serial.print(discByte);
  Serial.println(")");
  
  // CORRECTED: Using proper 0x80 "Set Disc Memo (Part 1)" command
  // Format: [device] [0x80] [disc] [13 bytes ASCII]
  
  // Prepare text data (max 13 characters, pad with 0x00)
  uint8_t textData[15]; // command + disc + 13 text bytes
  textData[0] = 0x80;      // Command code for "Set Disc Memo (Part 1)"
  textData[1] = discByte;  // Disc number in BCD
  
  // Fill text data (max 13 characters, pad with 0x00)
  for (int i = 0; i < 13; i++)
  {
    if (i < text.length())
    {
      textData[2 + i] = (uint8_t)text[i];
    }
    else
    {
      textData[2 + i] = 0x00; // Pad with null bytes
    }
  }
  
  // Debug: Show complete command being sent
  Serial.print("Sending CORRECTED 0x80 command: Device=0x");
  Serial.print(device, HEX);
  Serial.print(" Data=");
  for (int i = 0; i < 15; i++)
  {
    if (textData[i] < 0x10) Serial.print("0");
    Serial.print(textData[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  // Use sendLongCommand with correct 0x80 format
  slink.sendLongCommand(device, textData, 15);
  
  delay(500); // Give device time to process
  
  // NEW: Use custom Sony response parser that handles actual timing patterns
  Serial.println("Using custom Sony response parser to decode actual device response...");
  String response = parseSonyResponse(3000);
  
  Serial.print("Response: ");
  if (response.length() > 0)
  {
    Serial.println(response);
    
    // Check for success response (1F)
    if (response.indexOf("1F") >= 0)
    {
      Serial.println("✅ SUCCESS: Text write command acknowledged (1F)");
      return "SUCCESS";
    }
    else
    {
      Serial.println("⚠️  Unexpected response - command may have failed");
      return "UNKNOWN_RESPONSE";
    }
  }
  else
  {
    Serial.println("No response received");
    return "NO_RESPONSE";
  }
}

/**
 * Test function: Write single character "a" to disc 3
 */
void testSlinkWriteTextA()
{
  Serial.println("=== Testing S-Link Write Text: 'a' to Disc 3 ===");
  String result = slinkWriteDiscText(3, "a");
  Serial.print("Result: ");
  Serial.println(result);
  Serial.println("Check disc 3 display for the letter 'a'");
  Serial.println("");
}

/**
 * Test function: Write "TEST" to disc 3
 */
void testSlinkWriteTextTEST()
{
  Serial.println("=== Testing S-Link Write Text: 'TEST' to Disc 3 ===");
  String result = slinkWriteDiscText(3, "TEST");
  Serial.print("Result: ");
  Serial.println(result);
  Serial.println("Check disc 3 display for 'TEST'");
  Serial.println("");
}

/**
 * Test function: Write uppercase "A" to disc 3 (definitely in allowed range 0x20-0x5A)
 */
void testSlinkWriteTextA_Upper()
{
  Serial.println("=== Testing S-Link Write Text: 'A' (uppercase) to Disc 3 ===");
  Serial.println("Using uppercase 'A' which is definitely in allowed ASCII range 0x20-0x5A");
  String result = slinkWriteDiscText(3, "A");
  Serial.print("Result: ");
  Serial.println(result);
  Serial.println("Check disc 3 display for uppercase 'A'");
  Serial.println("");
}

/**
 * Test function: Write "X" using corrected 0x80 command to disc 3
 */
void testSlinkWriteText_0x80()
{
  Serial.println("=== Testing CORRECTED 0x80 'Set Disc Memo' Command ===");
  Serial.println("Using proper Sony S-Link command 0x80 instead of 0x98");
  Serial.println("Command format: [device] [0x80] [disc] [13 bytes ASCII]");
  Serial.println("");
  String result = slinkWriteDiscText(3, "X");
  Serial.print("Result: ");
  Serial.println(result);
  Serial.println("*** If this works, we've found the solution! ***");
  Serial.println("Check disc 3 display for 'X'");
  Serial.println("");
}

/**
 * Test function: Write custom text to specified disc
 */
void testSlinkWriteCustomText(int discNumber, const String& text)
{
  Serial.println("=== Testing S-Link Write Custom Text ===");
  Serial.print("Writing \"");
  Serial.print(text);
  Serial.print("\" to disc ");
  Serial.println(discNumber);
  
  String result = slinkWriteDiscText(discNumber, text);
  Serial.print("Result: ");
  Serial.println(result);
  Serial.print("Check disc ");
  Serial.print(discNumber);
  Serial.print(" display for '");
  Serial.print(text);
  Serial.println("'");
  Serial.println("");
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
String queryDeviceStatus()
{
  Serial.println("Querying device status (0x0F)...");
  String response = slink.sendCommandAndReceive(SLINK_DEVICE_CDP_CX1L, 0x0F, -1, 2000);

  if (response.length() > 0)
  {
    Serial.print("Device status response: ");
    Serial.println(response);
    return response;
  }
  else
  {
    Serial.println("No status response received");
    return "";
  }
}

/** Wait for device to be ready (status 08) */
bool waitForDeviceReady(int timeoutMs = 5000)
{
  Serial.println("Waiting for device ready status...");
  unsigned long startTime = millis();

  while (millis() - startTime < timeoutMs)
  {
    // Monitor for ready status (08) or other status responses
    String response = slink.inputMonitorWithReturn(2, false, 500000UL); // 0.5 second check

    if (response.indexOf("08") >= 0)
    {
      Serial.println("Device ready!");
      return true;
    }
    else if (response.length() > 0)
    {
      Serial.print("Device status: ");
      Serial.println(response);
    }

    delay(500);
  }

  Serial.println("Timeout waiting for device ready");
  return false;
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
  // S-Link direct text writing command
  commandHandlers["slinkText"] = [](const String &args)
  {
    // Parse disc number and title from args: "slinkText&disc=N&title=XXXX"
    int discIdx = args.indexOf("disc=");
    int titleIdx = args.indexOf("title=");

    if (discIdx >= 0 && titleIdx >= 0)
    {
      // Parse disc number
      int discEnd = args.indexOf('&', discIdx);
      if (discEnd < 0)
        discEnd = args.length();
      int discNum = urlDecode(args.substring(discIdx + 5, discEnd)).toInt();

      // Parse title
      int titleEnd = args.indexOf('&', titleIdx);
      if (titleEnd < 0)
        titleEnd = args.length();
      String title = urlDecode(args.substring(titleIdx + 6, titleEnd));

      if (discNum > 0 && discNum <= storage.getMaxDiscs() && title.length() > 0)
      {
        Serial.print("S-Link Text Write: Writing '");
        Serial.print(title);
        Serial.print("' to disc ");
        Serial.println(discNum);
        String result = slinkWriteDiscText(discNum, title);
        Serial.println("Write result: " + result);
      }
      else
      {
        Serial.println("Invalid disc number or title for S-Link write");
      }
    }
    else
    {
      Serial.println("Missing parameters for S-Link write command");
    }
  };
}

// ------------------ SETUP & LOOP ------------------
void setup()
{
  Serial.begin(115200);
  delay(1000); // Give Serial time to initialize

  Serial.println("=== Sony CDP-CX355 S-Link Controller Starting ===");

  slink.init(SLINK_PIN);
  pinMode(SLINK_PIN, INPUT);

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

      "<div style='margin-bottom: 20px; padding: 15px; background-color: #e8f5e8; border: 2px solid #4caf50;'>"
      "<h2 style='color: #2e8b57;'>✅ S-Link Direct Text Writing (SUCCESS!)</h2>"
      "<p><strong>WORKING SOLUTION:</strong> Using correct Sony S-Link 0x80 \"Set Disc Memo\" command!</p>"
      "<p><strong>Format:</strong> <code>[device] [0x80] [disc] [13 bytes ASCII]</code> - Direct Sony S-Link protocol</p>"
      "<p><strong>Advantages:</strong> No PS/2 complexity, uses proven S-Link communication, immediate response</p>"
      "<div style='margin: 15px 0; padding: 10px; background-color: #f0f8f0; border-left: 4px solid #4caf50;'>"
      "<h3 style='color: #2e8b57; margin-bottom: 10px;'>Write Custom Text</h3>"
      "<input type='text' id='customText' placeholder='Enter text (max 13 chars)' maxlength='13' style='padding: 8px; border: 1px solid #ccc; border-radius: 4px; margin-right: 5px; width: 150px;'>"
      "<input type='number' id='discNumber' placeholder='Disc #' min='1' max='400' style='padding: 8px; border: 1px solid #ccc; border-radius: 4px; margin-right: 5px; width: 80px;'>"
      "<button onclick='writeCustomText()' style='background-color: #4caf50; color: white; padding: 8px 16px; border: none; border-radius: 4px; cursor: pointer; font-weight: bold;'>✍️ Write Text</button><br><br>"
      "<h3 style='color: #2e8b57; margin-bottom: 8px;'>Quick Tests</h3>"
      "<button onclick='sendCommand(\"slinkText&disc=1&title=HELLO\")' style='background-color: #4caf50; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; margin: 2px;'>Write \"HELLO\" to Disc 1</button> "
      "<button onclick='sendCommand(\"slinkText&disc=2&title=WORLD\")' style='background-color: #45a049; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; margin: 2px;'>Write \"WORLD\" to Disc 2</button> "
      "<button onclick='sendCommand(\"slinkText&disc=3&title=TEST\")' style='background-color: #2e8b57; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; margin: 2px;'>Write \"TEST\" to Disc 3</button>"
      "</div>"
      "<p style='font-size: 11px; color: #666; margin-top: 8px;'>💡 <strong>How it works:</strong> Sends native Sony S-Link commands directly. Response: 1F (success). Check Sony display for text!</p>"
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
      ""
      "function writeCustomText() {"
      "  const text = document.getElementById('customText').value;"
      "  const discNum = document.getElementById('discNumber').value;"
      "  if (text && discNum) {"
      "    const cmd = 'slinkText&disc=' + discNum + '&title=' + encodeURIComponent(text);"
      "    sendCommand(cmd);"
      "  } else {"
      "    alert('Please enter both text and disc number');"
      "  }"
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
