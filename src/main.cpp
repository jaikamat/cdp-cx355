#include <map>
#include "WiFiS3.h"
#include "WiFiManager.hpp"
#include "HttpParser.hpp"
#include "Secrets.hpp"
#include <LedMatrixController.hpp>
#include "DiscStorage.hpp"
#include <functional>
#include <map>
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

void handleSelectDiscCommand(const String &args)
{
  int idx = args.indexOf("disc=");
  if (idx < 0)
    return;
  int end = args.indexOf('&', idx);
  if (end < 0)
    end = args.length();
  int discNumber = urlDecode(args.substring(idx + 5, end)).toInt();
  if (discNumber <= 0)
  {
    Serial.println("Invalid disc param");
    return;
  }
  Serial.print("Selecting disc #");
  Serial.println(discNumber);
  slinkSelectDisc(discNumber);
}

void handleSelectDiscAndMemoCommand(const String &args)
{
  // Extract `disc` and `memo` parameters
  int discIndex = args.indexOf("disc=");
  int memoIndex = args.indexOf("memo=");
  String discValue = "";
  String memoValue = "";

  if (discIndex != -1)
  {
    int discEnd = args.indexOf("&", discIndex);
    if (discEnd == -1)
      discEnd = args.length();
    discValue = urlDecode(args.substring(discIndex + 5, discEnd));
  }

  if (memoIndex != -1)
  {
    int memoEnd = args.indexOf("&", memoIndex);
    if (memoEnd == -1)
      memoEnd = args.length();
    memoValue = urlDecode(args.substring(memoIndex + 5, memoEnd));
  }

  // Log and process
  Serial.print("Disc Value: ");
  Serial.println(discValue);
  Serial.print("Memo Value: ");
  Serial.println(memoValue);

  if (!discValue.isEmpty() && !memoValue.isEmpty())
  {
    int discNumber = discValue.toInt();

    // Save the memo locally to EEPROM
    Serial.print("Saving memo locally for Disc ");
    Serial.print(discNumber);
    Serial.println("...");
    storage.writeDiscWithNumber(discNumber, memoValue);

    Serial.println("Memo successfully set.");
  }
  else
  {
    Serial.println("Error: Missing disc or memo message.");
  }
}

void handlePlayCommand(const String &) { slinkPlay(); }
void handleStopCommand(const String &) { slinkStop(); }
void handlePauseCommand(const String &) { slinkPauseToggle(); }
void handleNextCommand(const String &) { slinkNextTrack(); }
void handlePrevCommand(const String &) { slinkPrevTrack(); }

// ------------------ SETUP COMMAND MAP ------------------
void setupCommandHandlers()
{
  commandHandlers["power"] = handlePowerCommand;
  commandHandlers["selectDisc"] = handleSelectDiscCommand;
  commandHandlers["play"] = handlePlayCommand;
  commandHandlers["stop"] = handleStopCommand;
  commandHandlers["pause"] = handlePauseCommand;
  commandHandlers["next"] = handleNextCommand;
  commandHandlers["prev"] = handlePrevCommand;
  commandHandlers["setDiscMemo"] = handleSelectDiscAndMemoCommand;
  // ... add more if needed
}

// ------------------ SETUP & LOOP ------------------
void setup()
{
  Serial.begin(115200);

  slink.init(SLINK_PIN);
  pinMode(SLINK_PIN, INPUT);

  matrix.begin(); // LED matrix
  setupCommandHandlers();

  ledController.playAnimation(MatrixAnimation::WifiSearch, true);
  wifiManager.connect();
  ledController.displayText("wifi connected");
}

//
// Minimal HTML + pagination example, but rows in batches of 5
//
void sendForm(WiFiClient &client, DiscStorage &storage, int page)
{
  // Adjust as needed
  const int discsPerPage = 20;
  const int totalDiscs = storage.getMaxDiscs();
  int totalPages = (totalDiscs + discsPerPage - 1) / discsPerPage;

  if (page >= totalPages)
    page = totalPages - 1;
  if (page < 0)
    page = 0;

  int startIdx = page * discsPerPage;
  int endIdx = startIdx + discsPerPage;
  if (endIdx > totalDiscs)
    endIdx = totalDiscs;

  // HTTP headers
  client.print(F(
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/html\r\n"
      "Connection: close\r\n"
      "\r\n"
      "<!DOCTYPE html><html><head><title>CDP-CX355</title></head>"
      "<body style='font-family:Arial,sans-serif;font-size:14px;margin:20px;'>"));

  // Title and page info
  client.print("<h1>CDP-CX355 Controller</h1>");
  client.print("<p>Showing discs ");
  client.print(startIdx);
  client.print(" to ");
  client.print(endIdx - 1);
  client.print(" of ");
  client.print(totalDiscs);
  client.print("</p>");

  // Page nav
  client.print("<p>");
  if (page > 0)
  {
    client.print("<a href='/?page=0'>First</a>");
    client.print(" <a href='/?page=");
    client.print(page - 1);
    client.print("'>Prev</a> ");
  }
  if (page < totalPages - 1)
  {
    client.print(" <a href='/?page=");
    client.print(page + 1);
    client.print("'>Next</a>");
    client.print(" <a href='/?page=");
    client.print(totalPages - 1);
    client.print("'>Last</a>");
  }
  client.print("</p>");

  // Quick command forms
  client.print(
      "<form method='POST'><input type='hidden' name='command' value='play'>"
      "<input type='submit' value='Play'></form>"
      "<form method='POST'><input type='hidden' name='command' value='stop'>"
      "<input type='submit' value='Stop'></form>"
      "<form method='POST'><input type='hidden' name='command' value='pause'>"
      "<input type='submit' value='Pause'></form>"
      "<form method='POST'><input type='hidden' name='command' value='next'>"
      "<input type='submit' value='Next'></form>"
      "<form method='POST'><input type='hidden' name='command' value='prev'>"
      "<input type='submit' value='Prev'></form>"
      "<form method='POST'><input type='hidden' name='command' value='power'>"
      "<input type='submit' value='Power Toggle'></form>");

  // Table start
  client.print("<table border='1' cellpadding='4' style='border-collapse:collapse;'>"
               "<tr><th>#</th><th>Memo</th><th>Action</th></tr>");

  // We'll collect 5 rows in a chunk
  String rowChunk;
  const int ROW_BATCH_SIZE = 5;
  int rowCount = 0;

  for (int i = startIdx; i < endIdx; i++)
  {
    DiscInfo disc = storage.readDisc(i);

    // Build one row
    String row = "<tr><td>";
    row += disc.discNumber;
    row += "</td><td>"
           "<form method='POST' style='display:inline;'>"
           "<input type='hidden' name='command' value='setDiscMemo'>"
           "<input type='hidden' name='disc' value='";
    row += disc.discNumber;
    row += "'><input type='text' name='memo' value='";
    row += disc.memo;
    row += "' maxlength='13'> "
           "<input type='submit' value='Update'></form>"
           "</td><td>"
           "<form method='POST' style='display:inline;'>"
           "<input type='hidden' name='command' value='selectDisc'>"
           "<input type='hidden' name='disc' value='";
    row += disc.discNumber;
    row += "'><input type='submit' value='Play'>"
           "</form></td></tr>";

    // Add it to the chunk
    rowChunk += row;
    rowCount++;

    // If we've hit 5 rows in this chunk, send it
    if (rowCount == ROW_BATCH_SIZE)
    {
      client.print(rowChunk);
      rowChunk = ""; // Clear the chunk
      rowCount = 0;

      // Optionally, give the WiFi stack time to send
      // delay(2);
      // client.flush();
    }
  }

  // Print any leftover rows if rowCount < 5
  if (rowChunk.length() > 0)
  {
    client.print(rowChunk);
  }

  // End table and HTML
  client.print("</table></body></html>");
}

void loop()
{
  WiFiClient client = wifiManager.getServer().available();
  if (!client)
    return;

  Serial.println("New client connected.");
  HttpRequest request = HttpParser::parse(client);

  if (request.isPost)
  {
    // Extract “command=XYZ” from POST body
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

    // Dispatch
    auto it = commandHandlers.find(cmdValue);
    if (it != commandHandlers.end())
    {
      it->second(request.body);
    }
    else
    {
      Serial.println("Unknown command");
    }

    // Return the form
    // TODO: we are setting pageNumber to 0. How can we preserve it?
    sendForm(client, storage, 0);
  }
  else
  {
    // GET request: parse "?page=N"
    int pageNumber = 0;
    int qMark = request.url.indexOf('?');
    if (qMark >= 0)
    {
      String query = request.url.substring(qMark + 1);
      int pIdx = query.indexOf("page=");
      if (pIdx >= 0)
      {
        int amp = query.indexOf('&', pIdx);
        if (amp < 0)
          amp = query.length();
        pageNumber = query.substring(pIdx + 5, amp).toInt();
        if (pageNumber < 0)
          pageNumber = 0;
      }
    }
    Serial.print("GET page = ");
    Serial.println(pageNumber);

    // Return the page
    sendForm(client, storage, pageNumber);
  }

  delay(10);
  client.stop();
  Serial.println("Client disconnected.");
}
