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
  commandHandlers["bulkUpdate"] = handleBulkUpdateCommand;
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
// Minimal HTML + pagination example, but rows in batches
//
void sendForm(WiFiClient &client, DiscStorage &storage, int page)
{
  // For now, ignore 'page' or remove it if you don't want pagination.
  // We'll just show ALL discs. Or you can keep your chunking if you have 300 discs.
  // If you want pagination, you can still keep page logic, but let's show everything.

  const int discsPerPage = storage.getMaxDiscs(); // show them all on one page
  int startIdx = 0;
  int endIdx = storage.getMaxDiscs();

  // Minimal HTTP headers
  client.print(F(
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/html\r\n"
      "Connection: close\r\n"
      "\r\n"
      "<html><body>"));

  // Some short command forms (play, stop, etc.) in separate forms, if you like
  // Or you can skip these entirely for an even smaller page
  client.print(
      "<form method=POST><input type=hidden name=command value=play>"
      "<input type=submit value=Play></form>"
      "<form method=POST><input type=hidden name=command value=stop>"
      "<input type=submit value=Stop></form>"
      "<form method=POST><input type=hidden name=command value=pause>"
      "<input type=submit value=Pause></form>"
      "<form method=POST><input type=hidden name=command value=next>"
      "<input type=submit value=Next></form>"
      "<form method=POST><input type=hidden name=command value=prev>"
      "<input type=submit value=Prev></form>"
      "<form method=POST><input type=hidden name=command value=power>"
      "<input type=submit value=Power></form>");

  // Single big form for all discs
  client.print("<form method=POST>");

  // We'll say command=bulkUpdate for everything
  client.print("<input type=hidden name='command' value='bulkUpdate'>");

  // We'll chunk the discs to avoid large strings
  const int ROW_BATCH_SIZE = 10;
  String chunk;
  int rowCount = 0;

  for (int i = startIdx; i < endIdx; i++)
  {
    DiscInfo d = storage.readDisc(i);
    int discNum = d.discNumber; // or i+1, depending on your storage
    String memoVal = d.memo;

    // Build a minimal line: "#NN: <input name=m_NN value='Memo'> <button name='disc' value='NN'>Play</button><br>"
    String line = "#";
    line += discNum;
    line += ":<input name=m_";
    line += discNum;
    line += " value=\"";
    line += memoVal;
    line += "\"> ";
    // If user clicks this button, it passes disc=NN in addition to all m_ fields
    line += "<button name=disc value=";
    line += discNum;
    line += ">Play</button><br>";

    // Add to chunk
    chunk += line;
    rowCount++;

    if (rowCount == ROW_BATCH_SIZE)
    {
      client.print(chunk);
      chunk = "";
      rowCount = 0;
    }
  }

  // Send leftover if any
  if (chunk.length() > 0)
    client.print(chunk);

  // Finally a single "UpdateAll" button to save all memo fields
  client.print("<input type=submit value='UpdateAll'></form>");

  // Close HTML
  client.print("</body></html>");
}

void loop()
{
  WiFiClient client = wifiManager.getServer().available();
  if (!client)
    return;

  Serial.println("New client connected.");
  HttpRequest request = HttpParser::parse(client);

  // Squelch favicon.ico requests to save time
  if (!request.isPost && request.url == "/favicon.ico")
  {
    // A quick “no icon” response
    client.println("HTTP/1.1 404 Not Found");
    client.println("Connection: close");
    client.println();
    client.stop();
    return; // Skip the normal page
  }

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
