#include <map>
#include "WiFiS3.h"
#include "WiFiManager.hpp"
#include "HttpParser.hpp"
#include "Secrets.hpp"
#include <LedMatrixController.hpp>
#include "DiscStorage.hpp"
#include <functional>
#include "Sony_SLink.h"

#define SLINK_PIN 2 // Pick a suitable I/O pin for S-Link

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

ArduinoLEDMatrix matrix;
LedMatrixController ledController(matrix);
WiFiManager wifiManager(ssid, password);
DiscStorage storage;
Slink slink; // S-Link object
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
