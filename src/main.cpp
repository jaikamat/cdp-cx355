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
#include <WDT.h>

// Hardware watchdog period. If loop() stalls for longer than this (hung
// modem, stuck socket, bus fault) the RA4M1 resets itself instead of needing
// a power cycle. The hardware maximum is about 5.6 s, so there is no room to
// raise this; instead loop() refreshes the watchdog between its phases, and
// every phase is bounded well below this value:
//   - WiFiManager::ensureConnected   <= WIFI_JOIN_TIMEOUT_MS (4 s) when down
//   - SLinkProtocol::process         <= 5 retries x ~0.85 s  (~4.2 s)
//   - HttpParser::parse              <= REQUEST_DEADLINE_MS  (2 s)
//   - command handler (setDiscTitle) <= 5 retries x ~0.85 s  (~4.2 s)
// Anything added to loop() must either fit in that budget or refresh the
// watchdog itself.
const uint32_t WATCHDOG_TIMEOUT_MS = 5000;

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

  // Start the watchdog only now: setup() legitimately blocks for longer than
  // the period while joining WiFi and scrolling the hostname on the matrix.
  if (WDT.begin(WATCHDOG_TIMEOUT_MS))
  {
    Serial.print("Watchdog armed: ");
    Serial.print(WDT.getTimeout());
    Serial.println(" ms");
  }
  else
  {
    Serial.println("Watchdog failed to start!");
  }

  Serial.println("=== Ready for S-Link commands ===");
}

// Each phase of loop() is followed by a watchdog refresh. See the note on
// WATCHDOG_TIMEOUT_MS for the per-phase time budget this relies on.
void loop()
{
  WDT.refresh();
  wifiManager.ensureConnected();

  WDT.refresh();
  slink.process();
  wifiManager.runMDNS();

  WiFiClient client = wifiManager.getServer().available();
  if (!client)
    return;

  WDT.refresh();
  HttpRequest request = HttpParser::parse(client);
  WDT.refresh();

  // /status is polled every few seconds by the page; keep it out of the log
  // so the serial monitor stays readable.
  bool isStatusPoll = !request.isPost && request.url.startsWith("/status");
  if (!isStatusPoll)
    Serial.println("New client connected.");

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
    else if (isStatusPoll)
    {
      sendStatusJson(client, slink.getPlaybackStatus(), storage);
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

    // The page's JavaScript issues every command with fetch() and ignores the
    // response, so there is nothing to send back. A redirect here would make
    // fetch() re-download the whole page after every button press.
    client.println(F("HTTP/1.1 204 No Content"));
    client.println(F("Connection: close\r\n"));
    client.stop();
  }

  if (!isStatusPoll)
    Serial.println("Client disconnected.");
}
