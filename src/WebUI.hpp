#ifndef WEB_UI_HPP
#define WEB_UI_HPP

#include <Arduino.h>
#include "WiFiS3.h"
#include "DiscStorage.hpp"
#include "SLinkProtocol.hpp"

// Web interface for the Sony CDP-CX355 remote: the single-page HTML/JS UI
// and the paginated /discs JSON endpoint. Kept separate from main.cpp so
// application logic and presentation stay readable on their own.

// Print a string into a JSON value, escaping quotes/backslashes and
// dropping control characters so a memo can never break the document
inline void printJsonEscaped(WiFiClient &client, const char *text)
{
  for (const char *p = text; *p; p++)
  {
    char c = *p;
    if (c == '"' || c == '\\')
    {
      client.print('\\');
      client.print(c);
    }
    else if (c >= 0x20)
    {
      client.print(c);
    }
  }
}

inline void sendDiscsJsonStream(WiFiClient &client, const String &url, DiscStorage &storage)
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
    printJsonEscaped(client, d.memo);
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

// GET /status: what the player last reported about itself. Polled by the
// page every few seconds to drive the now-playing banner. "title" is the
// EEPROM memo for the current disc, or "" if none.
inline void sendStatusJson(WiFiClient &client, const PlaybackStatus &status, DiscStorage &storage)
{
  static const char *names[] = {"unknown", "off", "stopped", "paused", "playing"};

  client.println(F("HTTP/1.1 200 OK"));
  client.println(F("Content-Type: application/json"));
  client.println(F("Cache-Control: no-store"));
  client.println(F("Connection: close"));
  client.println();

  client.print("{\"state\":\"");
  client.print(names[(int)status.state]);
  client.print("\",\"disc\":");
  client.print(status.disc);
  client.print(",\"track\":");
  client.print(status.track);
  client.print(",\"len\":\"");
  client.print(status.lengthMin);
  client.print(':');
  if (status.lengthSec < 10)
    client.print('0');
  client.print(status.lengthSec);
  client.print("\",\"title\":\"");
  if (status.disc >= 1 && status.disc <= storage.getMaxDiscs())
  {
    DiscInfo d = storage.readDisc(status.disc - 1);
    printJsonEscaped(client, d.memo);
  }
  client.print("\"}");
  client.stop();
}

inline void sendIndexHtml(WiFiClient &client, int maxDiscs)
{
  client.print(F(
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/html\r\n"
      "Connection: close\r\n"
      "\r\n"
      "<!DOCTYPE html>"
      "<html>"
      "<head><title>Sony CDP-CX355 Remote</title>"
      "<meta name='viewport' content='width=device-width, initial-scale=1'>"
      // Mobile notes: 16px on form controls stops iOS Safari zooming in on
      // focus; .disc rows wrap so the title field and buttons fit a phone.
      "<style>"
      "body { font-family: sans-serif; margin: 12px; padding-bottom: 56px; }"
      ".now { position: fixed; left: 0; right: 0; bottom: 0; background: #222; color: #fff; padding: 12px; font-size: 16px; }"
      "button, input, select { font-size: 16px; padding: 8px; margin: 2px 0; }"
      ".disc { display: flex; flex-wrap: wrap; align-items: center; gap: 4px; margin-bottom: 8px; }"
      ".disc input { flex: 1; min-width: 120px; }"
      "body.busy button, body.busy select, body.busy input { pointer-events: none; opacity: 0.5; }"
      ".status { font-weight: bold; margin-left: 10px; }"
      "</style></head>"
      "<body>"
      "<h1>Sony CDP-CX355 Remote Control</h1>"

      "<div style='margin-bottom: 20px;'>"
      "<h2>Transport Controls <span id='status' class='status'></span></h2>"
      "<button onclick='sendCommand(\"play\")'>Play</button> "
      "<button onclick='sendCommand(\"stop\")'>Stop</button> "
      "<button onclick='sendCommand(\"pause\")'>Pause</button> "
      "<button onclick='sendCommand(\"next\")'>Next</button> "
      "<button onclick='sendCommand(\"prev\")'>Previous</button> "
      "<button onclick='sendCommand(\"powerOn\")'>Power On</button> "
      "<button onclick='sendCommand(\"powerOff\")'>Power Off</button>"
      "</div>"

      "<div style='margin-bottom: 20px;'>"
      "<h2>Set Disc Title</h2>"
      "<form onsubmit=\"event.preventDefault(); if(!lock('Setting title...'))return; var d=document.getElementById('discNum').value; var t=document.getElementById('title').value; fetch('/',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'command=saveDiscTitle&disc='+d+'&title='+encodeURIComponent(t)}).then(()=>{refreshCurrentRange();setTimeout(unlock,3000);}).catch(()=>setTimeout(unlock,1000));\">"
      "  <input type='number' id='discNum' placeholder='Disc #' min='1'>"
      "  <input type='text' id='title' placeholder='Title (13 chars)' maxlength='13'>"
      "  <button type='submit'>Set Title</button>"
      "</form>"
      "</div>"

      "<h2>Disc Collection</h2>"
      "<div style='margin-bottom: 10px;'>"
      "<select id='rangeSelect' onchange='loadRange()'>"
      "<option value=''>-- Select Disc Range --</option>"
      "</select>"
      "</div>"
      "<div id='discList'></div>"
      "<div id='now' class='now' hidden></div>"

      "<script>"));

  // Inject device constants so the page stays in sync with DiscStorage
  client.print("const MAX_DISCS = ");
  client.print(maxDiscs);
  client.print(";const PAGE_SIZE = 25;");

  client.print(F(
      "let currentRange = null;"
      "let busy = false;"

      "function lock(msg) {"
      "  if (busy) return false;"
      "  busy = true;"
      "  document.body.classList.add('busy');"
      "  document.getElementById('status').textContent = msg || 'Working...';"
      "  return true;"
      "}"

      "function unlock() {"
      "  busy = false;"
      "  document.body.classList.remove('busy');"
      "  document.getElementById('status').textContent = '';"
      "}"

      "function sendCommand(cmd) {"
      "  if (!lock('Sending...')) return;"
      "  fetch('/', {"
      "    method: 'POST',"
      "    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },"
      "    body: 'command=' + cmd"
      "  }).then(() => {"
      "    refreshCurrentRange();"
      "    setTimeout(unlock, 500);"
      "  }).catch(() => setTimeout(unlock, 500));"
      "}"

      "function playDisc(num) {"
      "  if (!lock('Playing disc ' + num + '...')) return;"
      "  fetch('/', {"
      "    method: 'POST',"
      "    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },"
      "    body: 'command=playDisc&disc=' + num"
      "  }).then(() => setTimeout(unlock, 500))"
      "  .catch(() => setTimeout(unlock, 500));"
      "}"

      "function saveDiscTitle(num) {"
      "  const input = document.querySelector('input[data-disc=\"' + num + '\"]');"
      "  if (!input || !lock('Saving disc ' + num + '...')) return;"
      "  fetch('/', {"
      "    method: 'POST',"
      "    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },"
      "    body: 'command=saveDiscTitle&disc=' + num + '&title=' + encodeURIComponent(input.value)"
      "  }).then(() => {"
      "    refreshCurrentRange();"
      "    setTimeout(unlock, 3000);"
      "  }).catch(() => setTimeout(unlock, 1000));"
      "}"

      "function discoverTitle(num) {"
      "  if (!lock('Discovering disc ' + num + '...')) return;"
      "  fetch('/', {"
      "    method: 'POST',"
      "    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },"
      "    body: 'command=discoverTitle&disc=' + num"
      "  }).then(() => {"
      "    refreshCurrentRange();"
      "    setTimeout(unlock, 5000);"
      "  }).catch(() => setTimeout(unlock, 2000));"
      "}"

      "function populateRangeDropdown() {"
      "  const select = document.getElementById('rangeSelect');"
      "  for (let i = 0; i < MAX_DISCS; i += PAGE_SIZE) {"
      "    const option = document.createElement('option');"
      "    option.value = (i / PAGE_SIZE + 1);"
      "    option.textContent = 'Discs ' + (i + 1) + '-' + Math.min(i + PAGE_SIZE, MAX_DISCS);"
      "    select.appendChild(option);"
      "  }"
      "}"

      "function loadRange() {"
      "  const page = document.getElementById('rangeSelect').value;"
      "  if (!page) return;"
      "  currentRange = page;"
      "  fetch('/discs?page=' + page + '&limit=' + PAGE_SIZE)"
      "    .then(response => response.json())"
      "    .then(data => {"
      "      const container = document.getElementById('discList');"
      "      let html = '';"
      "      data.discs.forEach(item => {"
      "        html += '<div class=\"disc\">';"
      "        html += '<span>Disc ' + item.d + '</span>';"
      "        html += '<input type=\"text\" data-disc=\"' + item.d + '\" maxlength=\"13\">';"
      "        html += '<button type=\"button\" onclick=\"playDisc(' + item.d + ')\">Play</button> ';"
      "        html += '<button type=\"button\" onclick=\"saveDiscTitle(' + item.d + ')\">Save</button> ';"
      "        html += '<button type=\"button\" onclick=\"discoverTitle(' + item.d + ')\">Auto-Discover</button>';"
      "        html += '</div>';"
      "      });"
      "      container.innerHTML = html;"
      // Assign titles via the DOM so quotes in a memo can't break the markup
      "      data.discs.forEach(item => {"
      "        const input = container.querySelector('input[data-disc=\"' + item.d + '\"]');"
      "        if (input) input.value = item.m;"
      "      });"
      "    })"
      "    .catch(err => {"
      "      console.error('Error fetching discs:', err);"
      "    });"
      "}"

      "function refreshCurrentRange() {"
      "  if (currentRange) {"
      "    setTimeout(loadRange, 3000);"
      "  }"
      "}"

      // Now-playing banner. State comes only from what the player reported
      // over S-Link (see /status); the page never infers it from button presses.
      "function pollStatus() {"
      "  fetch('/status').then(r => r.json()).then(s => {"
      "    const el = document.getElementById('now');"
      "    if (s.state === 'playing' || s.state === 'paused') {"
      "      const parts = [s.state === 'playing' ? 'Playing' : 'Paused'];"
      "      if (s.disc) parts.push('Disc ' + s.disc + (s.title ? ' (' + s.title + ')' : ''));"
      "      if (s.track) parts.push('Track ' + s.track + ' - ' + s.len);"
      "      el.textContent = parts.join(' - ');"
      "      el.hidden = false;"
      "    } else {"
      "      el.hidden = true;"
      "    }"
      "  }).catch(() => {}).finally(() => setTimeout(pollStatus, 3000));"
      "}"

      "document.addEventListener('DOMContentLoaded', () => {"
      "  document.getElementById('discNum').max = MAX_DISCS;"
      "  populateRangeDropdown();"
      "  pollStatus();"
      "});"
      "</script>"
      "</body></html>"));
}

#endif // WEB_UI_HPP
