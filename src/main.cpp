#include <map>
#include "WiFiS3.h"
#include "WiFiManager.hpp"
#include "HttpParser.hpp"
#include "Secrets.hpp"
#include <LedMatrixController.hpp>
#include "DiscStorage.hpp"
#include <functional>
#include "SLinkProtocol.hpp"
#include "WebUI.hpp"

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

ArduinoLEDMatrix matrix;
LedMatrixController ledController(matrix);
WiFiManager wifiManager(ssid, password);
DiscStorage storage;
SLinkProtocol slink(8); // Use Digital Pin 8 for S-Link

void onDiscTitleRetrieved(SLinkProtocol *protocol, void *userData)
{
  int discNum = (int)(intptr_t)userData;
  String title = protocol->getTitle();
  Serial.print("Callback received for disc ");
  Serial.print(discNum);
  Serial.print(": ");
  Serial.println(title.length() > 0 ? title : "(empty)");

  // Only skip writing on timeout - otherwise update EEPROM (even with empty title to clear it)
  if (title != "Timeout")
  {
    storage.writeDiscWithNumber(discNum, title);
  }
}

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

// Extract and decode a key's value from a form-encoded body ("a=1&b=2").
// Returns an empty String when the key is absent.
String getFormParam(const String &args, const char *key)
{
  String pattern = String(key) + "=";
  int start = 0;
  while (true)
  {
    int idx = args.indexOf(pattern, start);
    if (idx < 0)
      return "";
    if (idx == 0 || args[idx - 1] == '&')
    {
      start = idx + pattern.length();
      break;
    }
    start = idx + 1; // Matched inside another key's name; keep looking
  }
  int end = args.indexOf('&', start);
  if (end < 0)
    end = args.length();
  return urlDecode(args.substring(start, end));
}

using CommandHandler = std::function<void(const String &)>;
std::map<String, CommandHandler> commandHandlers;

void handlePowerOnCommand(const String &)
{
  slink.powerOn();
  Serial.println("Power: ON");
}

void handlePowerOffCommand(const String &)
{
  slink.powerOff();
  Serial.println("Power: OFF");
}

void handlePlayDiscCommand(const String &args)
{
  int discNum = getFormParam(args, "disc").toInt();
  if (discNum > 0 && discNum <= storage.getMaxDiscs())
  {
    Serial.print("Playing disc #");
    Serial.println(discNum);
    slink.selectDisc(discNum);
  }
}

void handleSaveDiscTitleCommand(const String &args)
{
  int discNum = getFormParam(args, "disc").toInt();
  String title = getFormParam(args, "title");

  if (discNum > 0 && discNum <= storage.getMaxDiscs())
  {
    Serial.print("Setting title for disc ");
    Serial.print(discNum);
    Serial.print(": ");
    Serial.println(title);

    // Send title to Sony player via S-Link
    slink.setDiscTitle(discNum, title);

    // Auto-discover after setting to sync EEPROM with player
    slink.getDiscTitle(discNum, onDiscTitleRetrieved, (void *)(intptr_t)discNum);
  }
}

void handleDiscoverTitleCommand(const String &args)
{
  int discNum = getFormParam(args, "disc").toInt();
  if (discNum > 0 && discNum <= storage.getMaxDiscs())
  {
    Serial.print("Discovering title for disc #");
    Serial.println(discNum);

    // Simple query-only approach: just get the disc title directly
    slink.getDiscTitle(discNum, onDiscTitleRetrieved, (void *)(intptr_t)discNum);
  }
}

void handlePlayCommand(const String &) { slink.play(); }
void handleStopCommand(const String &) { slink.stop(); }
void handlePauseCommand(const String &) { slink.pause(); }
void handleNextCommand(const String &) { slink.nextTrack(); }
void handlePrevCommand(const String &) { slink.prevTrack(); }

void setupCommandHandlers()
{
  commandHandlers["powerOn"] = handlePowerOnCommand;
  commandHandlers["powerOff"] = handlePowerOffCommand;
  commandHandlers["play"] = handlePlayCommand;
  commandHandlers["stop"] = handleStopCommand;
  commandHandlers["pause"] = handlePauseCommand;
  commandHandlers["next"] = handleNextCommand;
  commandHandlers["prev"] = handlePrevCommand;
  commandHandlers["playDisc"] = handlePlayDiscCommand;
  commandHandlers["saveDiscTitle"] = handleSaveDiscTitleCommand;
  commandHandlers["setDiscTitle"] = handleSaveDiscTitleCommand; // Alias kept for older clients
  commandHandlers["discoverTitle"] = handleDiscoverTitleCommand;
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== Sony CDP-CX355 S-Link Controller Starting ===");

  slink.begin();

  matrix.begin();
  setupCommandHandlers();

  ledController.playAnimation(MatrixAnimation::WifiSearch, true);
  wifiManager.connect();
  String hostname = String(wifiManager.getHostname()) + ".local";
  ledController.displayText(hostname.c_str());

  Serial.println("=== Ready for S-Link commands ===");
}

void loop()
{
  slink.process();
  wifiManager.runMDNS();

  WiFiClient client = wifiManager.getServer().available();
  if (!client)
    return;

  Serial.println("New client connected.");
  HttpRequest request = HttpParser::parse(client);

  if (!request.isPost && request.url == "/favicon.ico")
  {
    client.println("HTTP/1.1 204 No Content");
    client.println("Connection: close\r\n");
    client.stop();
    return;
  }

  if (!request.isPost)
  {
    if (request.url == "/")
    {
      sendIndexHtml(client, storage.getMaxDiscs());
    }
    else if (request.url.startsWith("/discs"))
    {
      sendDiscsJsonStream(client, request.url, storage);
    }
    else
    {
      client.println(F("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"));
    }
    delay(10);
    client.stop();
  }
  else
  {
    String cmdValue = getFormParam(request.body, "command");
    Serial.print("Command: ");
    Serial.println(cmdValue);

    auto it = commandHandlers.find(cmdValue);
    if (it != commandHandlers.end())
    {
      it->second(request.body);
    }
    else
    {
      Serial.println("Unknown command");
    }

    client.println(F("HTTP/1.1 303 See Other"));
    client.println(F("Location: /"));
    client.println(F("Connection: close\r\n"));
    client.println();
    client.stop();
  }

  Serial.println("Client disconnected.");
}
