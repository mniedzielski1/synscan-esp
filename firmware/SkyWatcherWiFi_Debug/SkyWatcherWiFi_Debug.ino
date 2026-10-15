/*
  SkyWatcherWiFi + debuger HTTP dla NodeMCU ESP8266
  Oparte na https://github.com/vlaate/SkyWatcherWiFi (z poprawkami z Cloudy Nights)

  Most: SynScan (UDP 11880) <-> montaz (UART 9600 8N1)
  Debuger: http://192.168.4.1  (podglad ruchu, liczniki, komendy testowe, filtr echa)

  Star Discovery ma JEDNA wspolna linie danych (RJ45 pin 5/6, w montazu zwarte).
  MOUNT_MODE 2 (domyslnie): odbior sprzetowym UART0 przelaczonym na D7, nadawanie programowe na D1
    D7 (GPIO13) <- linia danych, bezposrednio
    D1 (GPIO5)  -> ok. 550 Ohm (2 x 1,1 kOhm rownolegle) -> linia danych
                   D1 w trybie otwartego drenu: nie trzyma linii w stanie 1, montaz odpowiada bez zaklocen
    Log i komendy testowe przez USB NIE dzialaja (UART0 zajety). Diagnostyka przez WiFi.
  MOUNT_MODE 1: programowy UART na D1 (TX) / D2 (RX), log i komendy przez USB 115200.
    Nadawanie OK, ale odbior programowy przy montazu dawal smieci (test 14.10.2026).
  MOUNT_MODE 0: sprzetowy UART0 na RX/TX. Nie dziala przy podlaczonym USB
    (chip USB na GPIO3 zaglusza linie).
  1,1 kOhm na TX dawal stan niski 0,66 V (za wysoko dla montazu), ok. 550 Ohm daje ok. 0,37 V.
  GND -> pin 4 RJ45 (zawsze). Nic nie podlaczac do D8 (blokuje start).
*/

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include "page.h"

#define MOUNT_MODE 2
#if MOUNT_MODE == 1
#include <SoftwareSerial.h>
#define MOUNT_RX_PIN 4   // D2
#define MOUNT_TX_PIN 5   // D1
SoftwareSerial mount(MOUNT_RX_PIN, MOUNT_TX_PIN);
#elif MOUNT_MODE == 2
#include <SoftwareSerial.h>
#define MOUNT_TX_PIN 5   // D1, odbior: UART0 po Serial.swap() na GPIO13 (D7)
SoftwareSerial mountTx(-1, MOUNT_TX_PIN);
struct HybridPort {
  int available() { return Serial.available(); }
  int read() { return Serial.read(); }
  size_t write(const uint8_t* b, size_t n) { return mountTx.write(b, n); }
} mount;
#else
#define mount Serial
#endif

struct Entry;
void escapeTo(char* out, size_t outLen, const uint8_t* d, size_t len);
void addLog(const char* dir, const uint8_t* d, size_t len);
void addLogText(const char* dir, const char* s);
void handleRoot();
void handleLog();
void handleSend();
void handleEcho();
void handleClear();
void handleBaud();
void handleSniff();
void handleScan();
void setBaud(uint32_t b);
bool mountRxError();
bool replyValid(const uint8_t* d, size_t len);
void setup();
void loop();
void ledBlink();

// ---------- KONFIGURACJA ----------
#define AP_NAME          "SynScan_WiFi_1234"
#define AP_PASSWORD      ""      // "" = siec otwarta, albo min. 8 znakow
#define MOUNT_BAUD       9600
#define UDP_PORT         11880
#define QUIET_MS         20      // koniec odpowiedzi po tylu ms ciszy
#define REPLY_TIMEOUT_MS 300     // brak odpowiedzi po tylu ms
#define BUF              512
#define LOG_N            60
#define LOG_LEN          72
#define LED_PIN          2       // niebieska dioda na module ESP-12 (GPIO2), aktywna stanem niskim
#define LED_MS           40

WiFiUDP udp;
ESP8266WebServer web(80);
IPAddress remoteIp;
uint16_t remotePort = 0;

uint8_t udpBuf[BUF];
uint8_t rxBuf[BUF];
bool echoFilter = true;
uint32_t curBaud = MOUNT_BAUD;
bool sniff = false;     // tylko RX, TX montazu w stanie wysokiej impedancji

// skan predkosci: :e1 na kolejnych predkosciach (tylko odczyt wersji)
const uint32_t SCAN_BAUDS[] = {2400, 4800, 9600, 19200, 38400, 57600, 115200};
const uint8_t SCAN_N = sizeof(SCAN_BAUDS) / sizeof(SCAN_BAUDS[0]);
bool scanning = false;
uint8_t scanIdx = 0;
uint32_t scanNextAt = 0;

struct Entry { uint32_t seq; uint32_t t; char dir[6]; char txt[LOG_LEN]; };
Entry logBuf[LOG_N];
uint16_t logHead = 0, logCount = 0;
uint32_t logSeq = 0;
uint32_t cntApp = 0, cntMnt = 0, cntNoReply = 0, cntEcho = 0, cntTest = 0, cntErr = 0, cntRejected = 0;

bool waiting = false;
bool fromTest = false;
uint32_t sentAt = 0;
uint32_t ledOffAt = 0;

void ledBlink() { digitalWrite(LED_PIN, LOW); ledOffAt = millis() + LED_MS; }

// ---------- LOG ----------
void escapeTo(char* out, size_t outLen, const uint8_t* d, size_t len) {
  size_t o = 0;
  for (size_t i = 0; i < len && o + 5 < outLen; i++) {
    uint8_t c = d[i];
    if (c == '\r')      { out[o++] = '\\'; out[o++] = 'r'; }
    else if (c == '\n') { out[o++] = '\\'; out[o++] = 'n'; }
    else if (c >= 32 && c < 127 && c != '"' && c != '\\') out[o++] = c;
    else { o += snprintf(out + o, outLen - o, "\\x%02X", c); }
  }
  out[o] = 0;
}

void addLog(const char* dir, const uint8_t* d, size_t len) {
  Entry& e = logBuf[logHead];
  e.seq = ++logSeq;
  e.t = millis();
  strncpy(e.dir, dir, sizeof(e.dir) - 1); e.dir[sizeof(e.dir) - 1] = 0;
  escapeTo(e.txt, sizeof(e.txt), d, len);
  logHead = (logHead + 1) % LOG_N;
  if (logCount < LOG_N) logCount++;
#if MOUNT_MODE == 1
  Serial.printf("%8.2f  %-5s %s\n", e.t / 1000.0, e.dir, e.txt);
#endif
}

void addLogText(const char* dir, const char* s) { addLog(dir, (const uint8_t*)s, strlen(s)); }

// ---------- WWW ----------


void handleRoot() { web.send_P(200, "text/html", PAGE); }

void handleLog() {
  String s;
  s.reserve(6000);
  s += F("{\"app\":"); s += cntApp;
  s += F(",\"mnt\":"); s += cntMnt;
  s += F(",\"noreply\":"); s += cntNoReply;
  s += F(",\"echob\":"); s += cntEcho;
  s += F(",\"test\":"); s += cntTest;
  s += F(",\"clients\":"); s += WiFi.softAPgetStationNum();
  s += F(",\"echo\":"); s += echoFilter ? F("true") : F("false");
  s += F(",\"baud\":"); s += curBaud;
  s += F(",\"sniff\":"); s += sniff ? F("true") : F("false");
  s += F(",\"scan\":"); s += scanning ? F("true") : F("false");
  s += F(",\"err\":"); s += cntErr;
  s += F(",\"rej\":"); s += cntRejected;
  s += F(",\"log\":[");
  for (uint16_t i = 0; i < logCount; i++) {
    // najnowsze na gorze
    int idx = (int)logHead - 1 - i;
    if (idx < 0) idx += LOG_N;
    Entry& e = logBuf[idx];
    if (i) s += ',';
    s += F("{\"t\":"); s += e.t;
    s += F(",\"d\":\""); s += e.dir;
    s += F("\",\"x\":\"");
    for (const char* p = e.txt; *p; p++) { if (*p == '\\') s += F("\\\\"); else s += *p; }
    s += F("\"}");
  }
  s += F("]}");
  web.send(200, "application/json", s);
}

void handleSend() {
  String c = web.arg("c");
  c.trim();
  if (c.length() == 0 || c.length() > 32) { web.send(400, "text/plain", "bad"); return; }
  if (sniff || scanning) { web.send(409, "text/plain", "podsluch lub skan aktywny"); return; }
  if (!c.startsWith(":")) c = ":" + c;
  c += '\r';
  while (mount.available()) mount.read();
  mount.write((const uint8_t*)c.c_str(), c.length());
  fromTest = true; waiting = true; sentAt = millis(); cntTest++;
  addLog("TEST", (const uint8_t*)c.c_str(), c.length());
  web.send(200, "text/plain", "ok");
}

void handleEcho() {
  echoFilter = web.arg("on") != "0";
  addLogText("INFO", echoFilter ? "filtr echa WL" : "filtr echa WYL");
  web.send(200, "text/plain", "ok");
}

void handleClear() {
  logHead = logCount = 0;
  cntApp = cntMnt = cntNoReply = cntEcho = cntTest = cntErr = cntRejected = 0;
  web.send(200, "text/plain", "ok");
}

void setBaud(uint32_t b) {
#if MOUNT_MODE == 2
  Serial.end();
  Serial.begin(b, SERIAL_8N1, SERIAL_RX_ONLY);
  Serial.swap();                 // RX UART0 na GPIO13 (D7), chip USB odciety
  Serial.hasRxError();
  mountTx.end();
  if (sniff) pinMode(MOUNT_TX_PIN, INPUT);
  else {
    mountTx.begin(b, SWSERIAL_8N1, -1, MOUNT_TX_PIN);
    mountTx.enableTxGPIOOpenDrain(true);  // tylko sciaga linie w dol, w stanie 1 odlaczony (jak dioda)
  }
#elif MOUNT_MODE == 1
  mount.end();
  mount.begin(b, SWSERIAL_8N1, MOUNT_RX_PIN, sniff ? -1 : MOUNT_TX_PIN);
  if (sniff) pinMode(MOUNT_TX_PIN, INPUT);  // TX nie steruje linia montazu
#else
  Serial.flush();
  Serial.end();
  Serial.begin(b, SERIAL_8N1, sniff ? SERIAL_RX_ONLY : SERIAL_FULL);
  if (sniff) pinMode(1, INPUT);  // TX nie steruje linia montazu
  Serial.hasRxError();
#endif
  while (mount.available()) mount.read();
  curBaud = b;
}

// Poprawna odpowiedz montazu: "=" + cyfry hex (0-9, A-F) + "\r" albo "!" + cyfry + "\r"
bool replyValid(const uint8_t* d, size_t len) {
  if (len < 2 || d[len - 1] != '\r') return false;
  if (d[0] != '=' && d[0] != '!') return false;
  for (size_t i = 1; i < len - 1; i++) {
    uint8_t c = d[i];
    bool hex = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F');
    if (!hex) return false;
  }
  return true;
}

bool mountRxError() {
#if MOUNT_MODE == 1
  return mount.overflow();
#else
  return Serial.hasRxError();
#endif
}

void handleBaud() {
  uint32_t b = web.arg("b").toInt();
  if (b < 1200 || b > 230400) { web.send(400, "text/plain", "bad"); return; }
  setBaud(b);
  char t[32]; snprintf(t, sizeof(t), "predkosc %lu", (unsigned long)b);
  addLogText("INFO", t);
  web.send(200, "text/plain", "ok");
}

void handleSniff() {
  sniff = web.arg("on") == "1";
  if (sniff) scanning = false;
  setBaud(curBaud);
  addLogText("INFO", sniff ? "podsluch WL (TX odlaczony)" : "podsluch WYL");
  web.send(200, "text/plain", "ok");
}

void handleScan() {
  if (sniff) { web.send(409, "text/plain", "wylacz podsluch"); return; }
  scanning = true; scanIdx = 0; scanNextAt = millis();
  addLogText("INFO", "skan predkosci start");
  web.send(200, "text/plain", "ok");
}

// ---------- START ----------
void setup() {
#if MOUNT_MODE == 2
  setBaud(MOUNT_BAUD);
#elif MOUNT_MODE == 1
  Serial.begin(115200);
  mount.begin(MOUNT_BAUD, SWSERIAL_8N1, MOUNT_RX_PIN, MOUNT_TX_PIN);
#else
  Serial.begin(MOUNT_BAUD);
#endif
  pinMode(LED_PIN, OUTPUT);
  // 3 mrugniecia na start
  for (int i = 0; i < 3; i++) { digitalWrite(LED_PIN, LOW); delay(150); digitalWrite(LED_PIN, HIGH); delay(150); }

  WiFi.mode(WIFI_AP);
  IPAddress ip(192, 168, 4, 1), subnet(255, 255, 255, 0);
  WiFi.softAPConfig(ip, ip, subnet);
  if (strlen(AP_PASSWORD) >= 8) WiFi.softAP(AP_NAME, AP_PASSWORD);
  else                          WiFi.softAP(AP_NAME);

  udp.begin(UDP_PORT);
  web.on("/", handleRoot);
  web.on("/log", handleLog);
  web.on("/send", handleSend);
  web.on("/echo", handleEcho);
  web.on("/clear", handleClear);
  web.on("/baud", handleBaud);
  web.on("/sniff", handleSniff);
  web.on("/scan", handleScan);
  web.begin();
  addLogText("INFO", MOUNT_MODE == 2 ? "start, UDP 11880, 9600 8N1, TX D1, RX D7" : MOUNT_MODE == 1 ? "start, UDP 11880, 9600 8N1, TX D1, RX D2" : "start, UDP 11880, 9600 8N1, montaz na RX/TX");
}

void loop() {
  web.handleClient();
  if (ledOffAt && (int32_t)(millis() - ledOffAt) >= 0) { digitalWrite(LED_PIN, HIGH); ledOffAt = 0; }

#if MOUNT_MODE == 1
  // 0. Komenda testowa z USB (115200): linia zaczynajaca sie od ':' idzie do montazu
  static char usbLine[40]; static uint8_t usbLen = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r' || c == '\n') {
      if (usbLen == 1 && usbLine[0] == '0') {
        // pomiar: D1 (TX) w stanie niskim przez 15 s
        mount.end();
        pinMode(MOUNT_TX_PIN, OUTPUT); digitalWrite(MOUNT_TX_PIN, LOW);
        Serial.println("D1 = 0 przez 15 s, mierz napiecie linii");
        for (int i = 0; i < 150; i++) { delay(100); yield(); }
        digitalWrite(MOUNT_TX_PIN, HIGH);
        Serial.println("D1 = 1, koniec pomiaru");
        setBaud(curBaud);
      }
      if (usbLen == 1 && usbLine[0] == '?') {
        // diagnostyka pinow: D1 (TX) na 0 i 1, odczyt D2 (RX)
        mount.end();
        pinMode(MOUNT_TX_PIN, OUTPUT); pinMode(MOUNT_RX_PIN, INPUT);
        digitalWrite(MOUNT_TX_PIN, LOW);  delay(5); int r0 = digitalRead(MOUNT_RX_PIN);
        digitalWrite(MOUNT_TX_PIN, HIGH); delay(5); int r1 = digitalRead(MOUNT_RX_PIN);
        pinMode(MOUNT_TX_PIN, INPUT);     delay(5); int rz = digitalRead(MOUNT_RX_PIN);
        Serial.printf("PINY D1=0 -> D2=%d, D1=1 -> D2=%d, D1 wolny -> D2=%d\n", r0, r1, rz);
        setBaud(curBaud);
      }
      if (usbLen > 1 && usbLine[0] == ':' && !sniff && !scanning) {
        usbLine[usbLen++] = '\r';
        while (mount.available()) mount.read();
        mount.write((const uint8_t*)usbLine, usbLen);
        fromTest = true; waiting = true; sentAt = millis(); cntTest++;
        addLog("TEST", (const uint8_t*)usbLine, usbLen);
      }
      usbLen = 0;
    } else if (usbLen < sizeof(usbLine) - 2) usbLine[usbLen++] = c;
  }
#endif

  // 1. Aplikacja -> montaz
  int packetSize = udp.parsePacket();
  if (packetSize > 0) {
    remoteIp = udp.remoteIP();
    remotePort = udp.remotePort();
    int len = udp.read(udpBuf, BUF);
    if (len > 0 && !sniff && !scanning) {
      while (mount.available()) mount.read();
      mount.write(udpBuf, len);
      fromTest = false; waiting = true; sentAt = millis(); cntApp++;
      addLog("APP", udpBuf, len);
      ledBlink();
    }
  }

  // 2. Montaz -> aplikacja
  if (mount.available()) {
    size_t n = 0;
    while (true) {
      if (mount.available()) {
        uint8_t c = mount.read();
        if (n < BUF) rxBuf[n++] = c;
      } else {
        delay(QUIET_MS);
        if (!mount.available()) break;
      }
    }
    addLog("MNT", rxBuf, n);
    if (mountRxError()) { cntErr++; addLogText("INFO", "blad odbioru UART (ramka/przepelnienie)"); }
    ledBlink();
    cntMnt++;
    waiting = false;

    size_t start = 0;
    if (echoFilter) {
      while (start < n && rxBuf[start] != '=' && rxBuf[start] != '!') start++;
      cntEcho += start;
    }
    bool valid = replyValid(rxBuf + start, n - start);
    if (start < n && !valid && echoFilter) {
      cntRejected++;
      addLogText("INFO", "odrzucona odpowiedz (przeklamana), aplikacja ponowi komende");
    }
    if (start < n && (valid || !echoFilter) && !fromTest && !sniff && remotePort != 0) {
      udp.beginPacket(remoteIp, remotePort);
      udp.write(rxBuf + start, n - start);
      udp.endPacket();
    }
  }

  // 3. Skan predkosci
  if (scanning && !waiting && (int32_t)(millis() - scanNextAt) >= 0) {
    if (scanIdx < SCAN_N) {
      setBaud(SCAN_BAUDS[scanIdx++]);
      char t[32]; snprintf(t, sizeof(t), "skan %lu", (unsigned long)curBaud);
      addLogText("INFO", t);
      const char* c = ":e1\r";
      mount.write((const uint8_t*)c, 4);
      fromTest = true; waiting = true; sentAt = millis(); cntTest++;
      scanNextAt = millis() + REPLY_TIMEOUT_MS + 100;
    } else {
      scanning = false;
      setBaud(MOUNT_BAUD);
      addLogText("INFO", "skan koniec, powrot na 9600");
    }
  }

  // 4. Brak odpowiedzi
  if (waiting && millis() - sentAt > REPLY_TIMEOUT_MS) {
    waiting = false;
    cntNoReply++;
    addLogText("BRAK", "brak odpowiedzi montazu");
  }
}
