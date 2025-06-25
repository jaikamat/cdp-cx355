#include <map>
#include "WiFiS3.h"
#include "WiFiManager.hpp"
#include "HttpParser.hpp"
#include "Secrets.hpp"
#include <LedMatrixController.hpp>
#include "DiscStorage.hpp"
#include <functional>
#include "SLinkProtocol.hpp"

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

ArduinoLEDMatrix matrix;
LedMatrixController ledController(matrix);
WiFiManager wifiManager(ssid, password);
DiscStorage storage;
SLinkProtocol slink(8); // Use Digital Pin 8 for S-Link
bool isPlayerOn = true; // Assume player is ON as per user feedback

void onDiscTitleRetrieved(SLinkProtocol* protocol, void* userData) {
    int discNum = (int)userData;
    String title = protocol->getTitle();
    Serial.print("Callback received for disc ");
    Serial.print(discNum);
    Serial.print(": ");
    Serial.println(title);
    if (title.length() > 0 && title != "No Disc" && title != "No Title" && title != "Timeout") {
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

void slinkPlay() { slink.play(); }
void slinkStop() { slink.stop(); }
void slinkPauseToggle() { slink.pause(); }
void slinkNextTrack() { slink.nextTrack(); }
void slinkPrevTrack() { slink.prevTrack(); }
void slinkSelectDisc(int discNumber) { slink.selectDisc(discNumber); }

using CommandHandler = std::function<void(const String &)>;
std::map<String, CommandHandler> commandHandlers;

void handlePowerCommand(const String &)
{
  if (isPlayerOn)
  {
    slink.powerOff();
  }
  else
  {
    slink.powerOn();
  }
  isPlayerOn = !isPlayerOn;
  Serial.println(isPlayerOn ? "Power: OFF -> ON" : "Power: ON -> OFF");
}

void handleBulkUpdateCommand(const String &args)
{
  const int totalDiscs = storage.getMaxDiscs();

  for (int i = 1; i <= totalDiscs; i++)
  {
    String paramName = "m_" + String(i) + "=";
    int idx = args.indexOf(paramName);
    if (idx >= 0)
    {
      int end = args.indexOf('&', idx);
      if (end < 0)
        end = args.length();
      String memoVal = urlDecode(args.substring(idx + paramName.length(), end));
      storage.writeDiscWithNumber(i, memoVal);
    }
  }

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
      Serial.print("Queuing title discovery for disc #");
      Serial.println(discNum);
      slink.getDiscTitle(discNum, onDiscTitleRetrieved, (void *)discNum);
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

void handleQueryDiscMemoryCommand(const String &args) {
    int discIdx = args.indexOf("disc=");
    if (discIdx >= 0) {
        int discNum = urlDecode(args.substring(discIdx + 5)).toInt();
        slink.queryDiscMemoryInfo(discNum);
    }
}

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
  commandHandlers["queryDiscMemory"] = handleQueryDiscMemoryCommand;
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

void sendDiscsJsonStream(WiFiClient &client, String url)
{
  int page = 1;
  int limit = 25;

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
      limit = 25;
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

      "<div style='margin-bottom: 20px;'>"
      "<h2>Disc Memory Info</h2>"
      "<button onclick=\"sendCommand('queryDiscMemory&disc=3')\">Query Disc 3</button> "
      "<button onclick=\"sendCommand('queryDiscMemory&disc=8')\">Query Disc 8</button>"
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
      sendIndexHtml(client);
    }
    else if (request.url.startsWith("/discs"))
    {
      sendDiscsJsonStream(client, request.url);
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