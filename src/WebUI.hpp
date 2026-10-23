#ifndef WEB_UI_HPP
#define WEB_UI_HPP

#include <Arduino.h>
#include "WiFiS3.h"
#include "DiscStorage.hpp"
#include "SLinkProtocol.hpp"

// Web interface for the Sony CDP-CX355 remote: the single-page HTML/JS UI
// and the paginated /discs JSON endpoint. Kept separate from main.cpp so
// application logic and presentation stay readable on their own.

// Copy `text` into `out` as a JSON string body: quotes and backslashes are
// escaped and control characters dropped, so a memo can never break the
// document. `out` needs room for 2 * strlen(text) + 1 bytes.
inline void jsonEscape(const char *text, char *out, size_t outSize)
{
  size_t n = 0;
  for (const char *p = text; *p && n + 2 < outSize; p++)
  {
    char c = *p;
    if (c == '"' || c == '\\')
      out[n++] = '\\';
    if (c >= 0x20)
      out[n++] = c;
  }
  out[n] = '\0';
}

// Print a string into a JSON value (see jsonEscape). Sized for a DiscInfo memo.
inline void printJsonEscaped(WiFiClient &client, const char *text)
{
  char buf[2 * sizeof(DiscInfo::memo) + 1];
  jsonEscape(text, buf, sizeof(buf));
  client.print(buf);
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

  int total = storage.getMaxDiscs();
  int startIdx = (page - 1) * limit;
  int endIdx = startIdx + limit;
  if (endIdx > total)
    endIdx = total;

  // Every client.print/write is a separate command to the ESP32 modem
  // (WiFiS3 has no write buffer), so the response is assembled into a
  // buffer and written in as few pieces as possible: one for the headers,
  // one per disc, one for the trailer. Printing a row field by field cost
  // about 18 modem round trips per disc and made a 25-disc page take ~1.9 s.
  client.print(F("HTTP/1.1 200 OK\r\n"
                 "Content-Type: application/json\r\n"
                 "Connection: close\r\n"
                 "\r\n"
                 "{\"discs\":["));

  char memo[2 * sizeof(DiscInfo::memo) + 1];
  char row[96];
  for (int i = startIdx; i < endIdx; i++)
  {
    DiscInfo d = storage.readDisc(i);
    jsonEscape(d.memo, memo, sizeof(memo));
    int n = snprintf(row, sizeof(row), "%s{\"d\":%d,\"m\":\"%s\"}",
                     i == startIdx ? "" : ",", d.discNumber, memo);
    client.write((const uint8_t *)row, n);
  }

  int n = snprintf(row, sizeof(row), "],\"page\":%d,\"limit\":%d,\"total\":%d,\"hasMore\":%s}",
                   page, limit, total, endIdx < total ? "true" : "false");
  client.write((const uint8_t *)row, n);
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
      "<meta name='color-scheme' content='dark'>"
      // Styling notes: graphite body and silver-gradient buttons echo the
      // CX355 fascia; title fields and the now-playing strip use amber
      // monospace on near-black like its fluorescent display. 16px form
      // controls stop iOS Safari zooming on focus. No images or web fonts.
      "<style>"
      "* { box-sizing: border-box; }"
      "body { font-family: -apple-system, Helvetica, Arial, sans-serif; background: #1c1e21; color: #d7d9dc; margin: 0; padding: 0 12px 76px; }"
      "header { display: flex; align-items: baseline; justify-content: space-between; padding: 16px 2px 10px; border-bottom: 1px solid #3a3d42; }"
      ".brand { font-weight: 900; letter-spacing: .2em; font-size: 22px; color: #f2f2f2; }"
      ".model { font-size: 12px; letter-spacing: .12em; text-transform: uppercase; color: #9a9ea5; }"
      ".model b { color: #d7d9dc; }"
      "section { margin: 14px 0; padding: 12px; background: #24272b; border: 1px solid #3a3d42; border-radius: 6px; }"
      "h2 { margin: 0 0 10px; font-size: 11px; font-weight: 600; letter-spacing: .14em; text-transform: uppercase; color: #9a9ea5; }"
      "button { font: 600 14px/1 -apple-system, Helvetica, Arial, sans-serif; color: #1a1a1a; background: linear-gradient(#f4f4f4, #c9ccd1); border: 1px solid #8c9097; border-radius: 4px; padding: 0 12px; min-height: 42px; }"
      "button:active { background: linear-gradient(#b9bcc1, #dcdfe3); }"
      "input, select { font: 16px ui-monospace, Menlo, monospace; color: #ffb000; background: #0f1113; border: 1px solid #3a3d42; border-radius: 4px; padding: 0 10px; min-height: 42px; }"
      "input::placeholder { color: #6b5a2a; }"
      "select { color: #d7d9dc; font-family: inherit; width: 100%; }"
      ".row { display: flex; flex-wrap: wrap; gap: 6px; margin-top: 6px; }"
      ".row button { flex: 1 1 30%; }"
      ".form { display: flex; gap: 6px; }"
      ".form input[type=number] { width: 5.5em; }"
      ".form input[type=text] { flex: 1; min-width: 0; }"
      ".disc { display: grid; grid-template-columns: auto 1fr; gap: 6px; margin-top: 10px; }"
      ".num { font: 600 12px ui-monospace, Menlo, monospace; letter-spacing: .06em; color: #9a9ea5; align-self: center; }"
      ".disc input { min-width: 0; }"
      ".btns { grid-column: 1 / -1; display: flex; gap: 6px; }"
      ".btns button { flex: 1; }"
      "@media (min-width: 640px) { .disc { grid-template-columns: auto 1fr auto; } .btns { grid-column: auto; } .btns button { flex: none; } }"
      ".status { color: #ffb000; font: 12px ui-monospace, Menlo, monospace; letter-spacing: .06em; text-transform: none; margin-left: 8px; }"
      ".now { position: fixed; left: 0; right: 0; bottom: 0; background: #0b0c0e; border-top: 2px solid #3a3d42; color: #ffb000; font: 16px ui-monospace, Menlo, monospace; padding: 16px 14px; text-shadow: 0 0 6px rgba(255,176,0,.45); white-space: pre; overflow: hidden; text-overflow: ellipsis; }"
      ".num.playing { color: #ffb000; text-shadow: 0 0 6px rgba(255,176,0,.45); }"
      "body.busy button, body.busy select, body.busy input { pointer-events: none; opacity: 0.5; }"
      "</style></head>"
      "<body>"
      "<header><span class='brand'>SONY</span><span class='model'><b>CDP-CX355</b> &middot; Mega Storage</span></header>"

      "<section>"
      "<h2>Transport <span id='status' class='status'></span></h2>"
      "<div class='row'>"
      "<button onclick='sendCommand(\"play\")'>&#9654; Play</button>"
      "<button onclick='sendCommand(\"pause\")'>&#9646;&#9646; Pause</button>"
      "<button onclick='sendCommand(\"stop\")'>&#9632; Stop</button>"
      "<button onclick='sendCommand(\"prev\")'>&#9664;&#9664; Prev</button>"
      "<button onclick='sendCommand(\"next\")'>&#9654;&#9654; Next</button>"
      "</div>"
      "<div class='row'>"
      "<button onclick='sendCommand(\"powerOn\")'>Power On</button>"
      "<button onclick='sendCommand(\"powerOff\")'>Power Off</button>"
      "</div>"
      "</section>"

      "<section>"
      "<h2>Set Disc Title</h2>"
      "<form class='form' onsubmit=\"event.preventDefault(); if(!lock('Setting title...'))return; var d=document.getElementById('discNum').value; var t=document.getElementById('title').value; fetch('/',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'command=saveDiscTitle&disc='+d+'&title='+encodeURIComponent(t)}).then(()=>{refreshDisc(d);setTimeout(unlock,3000);}).catch(()=>setTimeout(unlock,1000));\">"
      "<input type='number' id='discNum' placeholder='Disc' min='1'>"
      "<input type='text' id='title' placeholder='Title (13 chars)' maxlength='13'>"
      "<button type='submit'>Set</button>"
      "</form>"
      "</section>"

      "<section>"
      "<h2>Disc Collection</h2>"
      "<select id='rangeSelect' onchange='loadRange()'>"
      "<option value=''>-- Jump to Range --</option>"
      "</select>"
      "<div id='discList'></div>"
      "<div id='more' style='height:1px'></div>"
      "</section>"
      "<div id='now' class='now' hidden></div>"

      "<script>"));

  // Inject device constants so the page stays in sync with DiscStorage
  client.print("const MAX_DISCS = ");
  client.print(maxDiscs);
  client.print(";const PAGE_SIZE = 25;");

  client.print(F(
      "let nextPage = 1;"
      "let loading = false;"
      "let listGen = 0;"
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
      "  }).then(() => setTimeout(unlock, 500))"
      "  .catch(() => setTimeout(unlock, 500));"
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
      "    refreshDisc(num);"
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
      "    refreshDisc(num);"
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

      // Disc list: infinite scroll. Rows are appended 25 at a time as the
      // sentinel div below the list scrolls into view, so the device never
      // serves more than one page per request while the browser keeps every
      // loaded row (300 rows is a small DOM). The dropdown clears the list
      // and restarts scrolling from the chosen range. listGen discards a
      // page that was in flight when the list was cleared.
      "function discRow(item) {"
      "  return '<div class=\"disc\">'"
      "    + '<span class=\"num\" data-disc=\"' + item.d + '\">DISC ' + String(item.d).padStart(3, '0') + '</span>'"
      "    + '<input type=\"text\" data-disc=\"' + item.d + '\" maxlength=\"13\">'"
      "    + '<div class=\"btns\">'"
      "    + '<button type=\"button\" onclick=\"playDisc(' + item.d + ')\">&#9654; Play</button>'"
      "    + '<button type=\"button\" onclick=\"saveDiscTitle(' + item.d + ')\">Save</button>'"
      "    + '<button type=\"button\" onclick=\"discoverTitle(' + item.d + ')\">Discover</button>'"
      "    + '</div></div>';"
      "}"

      // Assign titles via the DOM so quotes in a memo can't break the markup
      "function setTitle(item) {"
      "  const input = document.querySelector('input[data-disc=\"' + item.d + '\"]');"
      "  if (input) input.value = item.m;"
      "}"

      "function loadPage(page) {"
      "  const gen = listGen;"
      "  loading = true;"
      "  fetch('/discs?page=' + page + '&limit=' + PAGE_SIZE)"
      "    .then(response => response.json())"
      "    .then(data => {"
      "      if (gen !== listGen) return;"
      "      document.getElementById('discList').insertAdjacentHTML('beforeend', data.discs.map(discRow).join(''));"
      "      data.discs.forEach(setTitle);"
      "      nextPage = data.hasMore ? page + 1 : 0;"
      "    })"
      "    .catch(err => console.error('Error fetching discs:', err))"
      "    .finally(() => { loading = false; watchSentinel(); });"
      "}"

      // Fires when the sentinel enters the viewport (200px early so the next
      // page is usually there before the user reaches the bottom)
      "const observer = new IntersectionObserver(entries => {"
      "  if (entries[0].isIntersecting && nextPage && !loading) loadPage(nextPage);"
      "}, { rootMargin: '200px' });"

      // Re-arm so the observer reports the current state even if it has not
      // changed, e.g. a tall window that shows a whole page with room to spare
      "function watchSentinel() {"
      "  const more = document.getElementById('more');"
      "  observer.unobserve(more);"
      "  observer.observe(more);"
      "}"

      "function loadRange() {"
      "  const page = +document.getElementById('rangeSelect').value;"
      "  if (!page) return;"
      "  listGen++;"
      "  document.getElementById('discList').innerHTML = '';"
      "  nextPage = page;"
      "  watchSentinel();"
      "}"

      // After a Save or Discover, re-read just that disc once the bus has
      // had time to answer. page=N&limit=1 addresses exactly disc N.
      "function refreshDisc(num) {"
      "  setTimeout(() => {"
      "    fetch('/discs?page=' + num + '&limit=1')"
      "      .then(response => response.json())"
      "      .then(data => data.discs.forEach(setTitle))"
      "      .catch(() => {});"
      "  }, 3000);"
      "}"

      // Now-playing banner. State comes only from what the player reported
      // over S-Link (see /status); the page never infers it from button presses.
      "function pollStatus() {"
      "  fetch('/status').then(r => r.json()).then(s => {"
      "    const el = document.getElementById('now');"
      "    if (s.state === 'playing' || s.state === 'paused') {"
      "      const parts = [s.state === 'playing' ? '\\u25B6' : '\\u25AE\\u25AE'];"
      "      if (s.disc) parts.push('DISC ' + String(s.disc).padStart(3, '0'));"
      "      if (s.track) parts.push('TRK ' + String(s.track).padStart(2, '0') + '  ' + s.len);"
      "      if (s.title) parts.push(s.title);"
      "      el.textContent = parts.join('  ');"
      "      el.hidden = false;"
      "    } else {"
      "      el.hidden = true;"
      "    }"
      // Light up the disc label of the disc the player says is loaded
      "    const active = s.state === 'playing' || s.state === 'paused' ? s.disc : 0;"
      "    document.querySelectorAll('.num').forEach(n => n.classList.toggle('playing', +n.dataset.disc === active));"
      "  }).catch(() => {}).finally(() => setTimeout(pollStatus, 3000));"
      "}"

      "document.addEventListener('DOMContentLoaded', () => {"
      "  document.getElementById('discNum').max = MAX_DISCS;"
      "  populateRangeDropdown();"
      "  watchSentinel();"
      "  pollStatus();"
      "});"
      "</script>"
      "</body></html>"));
}

#endif // WEB_UI_HPP
