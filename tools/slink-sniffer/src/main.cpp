// S-Link bus sniffer for protocol research. Not part of the product firmware.
// - Pin-change interrupt on D8 timestamps every edge into a ring buffer.
// - loop() decodes LOW pulse widths into frames and prints them with a ms
//   timestamp: "[12345] 98 50 CA 03 03 48".
// - A line of hex bytes on Serial ("90 00") is transmitted on the bus.
#include <Arduino.h>

static const uint8_t PIN = 8;
#define MARK_SYNC 2400
#define MARK_ONE 1200
#define MARK_ZERO 600
#define MARK_DELIM 600
#define WORD_DELIM 30000
#define LINE_READY 3000
#define LOOP_TIMEOUT 500000

// ---- edge capture ----
struct Edge { uint32_t t; uint8_t level; };
static volatile Edge ring[512];
static volatile uint16_t head = 0, tail = 0;
static volatile bool txActive = false;
static volatile uint32_t edgeCount = 0;

void onEdge() {
  if (txActive) return;
  edgeCount++;
  uint16_t n = (head + 1) & 511;
  if (n == tail) return; // overflow: drop
  ring[head].t = micros();
  ring[head].level = digitalRead(PIN);
  head = n;
}

// ---- decoder ----
static uint8_t frame[32]; static int frameLen = 0;
static uint8_t cur = 0; static int bits = 0;
static bool inFrame = false;
static uint32_t lowStart = 0; static bool haveLow = false;
static uint32_t lastEdge = 0;

void emitFrame() {
  if (frameLen == 0 && bits == 0) return;
  Serial.print('['); Serial.print(millis()); Serial.print("] ");
  for (int i = 0; i < frameLen; i++) { if (frame[i] < 16) Serial.print('0'); Serial.print(frame[i], HEX); Serial.print(' '); }
  if (bits) { Serial.print("(+"); Serial.print(bits); Serial.print(" bits)"); }
  Serial.println();
  frameLen = 0; bits = 0; cur = 0; inFrame = false;
}

void decode() {
  while (tail != head) {
    Edge e; e.t = ring[tail].t; e.level = ring[tail].level; tail = (tail + 1) & 511;
    lastEdge = e.t;
    if (e.level == LOW) { lowStart = e.t; haveLow = true; continue; }
    if (!haveLow) continue;
    haveLow = false;
    uint32_t w = e.t - lowStart;
    if (w > MARK_SYNC / 1.3 && w < MARK_SYNC * 1.3) { emitFrame(); inFrame = true; }
    else if (!inFrame) { /* noise or mid-frame join */ }
    else if (w > MARK_ONE / 1.3 && w < MARK_ONE * 1.3) { cur = (cur << 1) | 1; bits++; }
    else if (w > MARK_ZERO / 1.4 && w < MARK_ZERO * 1.4) { cur = (cur << 1); bits++; }
    else { Serial.print("  ?pulse "); Serial.println(w); }
    if (bits == 8) { if (frameLen < 32) frame[frameLen++] = cur; cur = 0; bits = 0; }
  }
  // Frame ends after the line has been quiet for a while
  if (inFrame && micros() - lastEdge > 5000) emitFrame();
}

// ---- transmitter (same timings as the product firmware) ----
void lineReady() {
  uint32_t s = micros(), b = s;
  do { delayMicroseconds(25); if (digitalRead(PIN) == LOW) s = micros(); }
  while ((micros() - s < LINE_READY) && (micros() - b < LOOP_TIMEOUT));
}
void writeByte(uint8_t v) {
  for (int i = 7; i >= 0; i--) {
    digitalWrite(PIN, LOW); delayMicroseconds((v & (1 << i)) ? MARK_ONE : MARK_ZERO);
    digitalWrite(PIN, HIGH); delayMicroseconds(MARK_DELIM);
  }
}
void sendFrame(const uint8_t *c, int n) {
  lineReady();
  txActive = true;
  detachInterrupt(digitalPinToInterrupt(PIN));
  uint32_t start = micros();
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN, LOW); delayMicroseconds(MARK_SYNC); digitalWrite(PIN, HIGH); delayMicroseconds(MARK_DELIM);
  for (int i = 0; i < n; i++) writeByte(c[i]);
  pinMode(PIN, INPUT);
  do { delayMicroseconds(25); if (digitalRead(PIN) == LOW) break; } while (micros() - start < WORD_DELIM);
  head = tail = 0; haveLow = false;
  attachInterrupt(digitalPinToInterrupt(PIN), onEdge, CHANGE);
  txActive = false;
  Serial.print("[" ); Serial.print(millis()); Serial.print("] TX ");
  for (int i = 0; i < n; i++) { if (c[i] < 16) Serial.print('0'); Serial.print(c[i], HEX); Serial.print(' '); }
  Serial.println();
}

void setup() {
  Serial.begin(115200); delay(1000);
  pinMode(PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN), onEdge, CHANGE);
  Serial.println("SNIFFER READY");
}

String line;
static uint32_t lastStat = 0;
void loop() {
  decode();
  if (millis() - lastStat > 5000) { lastStat = millis(); Serial.print("  (edges so far: "); Serial.print(edgeCount); Serial.println(")"); }
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') {
      if (line.length()) {
        uint8_t c[16]; int n = 0; char *p = (char*)line.c_str();
        while (*p && n < 16) { while (*p == ' ') p++; if (!*p) break; c[n++] = strtol(p, &p, 16); }
        if (n) sendFrame(c, n);
        line = "";
      }
    } else line += ch;
  }
}
