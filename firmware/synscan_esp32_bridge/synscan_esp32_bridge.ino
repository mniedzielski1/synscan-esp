// NIEAKTUALNE (15.10.2026): archiwum. Zaklada osobne RX/TX montazu, a Star Discovery ma jedna
// wspolna linie danych (RJ45 pin 5/6). Uzyj firmware/SkyWatcherWiFi_ESP32.
// ESP32 jako zamiennik adaptera Sky-Watcher SynScan WiFi
// Most UDP:11880 <-> UART (protokol motor controllera Sky-Watcher, 9600 8N1)
// Podlaczenie: bezposrednio do gniazda pilota w MONTAZU (TTL), NIE do RJ12 w pilocie (RS-232).

#include <WiFi.h>
#include <WiFiUdp.h>

// ---- KONFIGURACJA ----
const char* AP_SSID = "SynScan_ESP32";
const char* AP_PASS = "synscan123";      // min. 8 znakow
const uint16_t UDP_PORT = 11880;         // port, ktorego uzywa SynScan Pro

#define RX_PIN 16                        // ESP32 RX  <- TX montazu
#define TX_PIN 17                        // ESP32 TX  -> RX montazu
#define MOUNT_BAUD 9600
#define REPLY_TIMEOUT_MS 300
#define LED_PIN 2                        // wbudowana dioda na wiekszosci plytek DevKit

WiFiUDP udp;
uint8_t inBuf[128];
uint8_t outBuf[128];

// Czyta jedna linie zakonczona '\r' z UART. Zwraca dlugosc albo 0 przy timeoucie.
size_t readLine(uint8_t* dst, size_t maxLen, uint32_t timeoutMs) {
  size_t n = 0;
  uint32_t t0 = millis();
  while (millis() - t0 < timeoutMs) {
    if (Serial2.available()) {
      uint8_t c = Serial2.read();
      if (n < maxLen) dst[n++] = c;
      if (c == '\r') return n;
    }
  }
  return 0;
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(MOUNT_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
  pinMode(LED_PIN, OUTPUT);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1),
                    IPAddress(192, 168, 4, 1),
                    IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_SSID, AP_PASS);

  udp.begin(UDP_PORT);
  Serial.printf("AP: %s  IP: 192.168.4.1  UDP: %u\n", AP_SSID, UDP_PORT);
}

void loop() {
  int pktLen = udp.parsePacket();
  if (pktLen <= 0) return;

  IPAddress rip = udp.remoteIP();
  uint16_t rport = udp.remotePort();
  int len = udp.read(inBuf, sizeof(inBuf));
  if (len <= 0) return;

  digitalWrite(LED_PIN, HIGH);

  while (Serial2.available()) Serial2.read();   // wyczysc smieci z bufora
  Serial2.write(inBuf, len);
  Serial2.flush();

  size_t n = readLine(outBuf, sizeof(outBuf), REPLY_TIMEOUT_MS);

  // Niektore plyty odsylaja echo komendy (zaczyna sie od ':'). Pomin je.
  if (n > 0 && outBuf[0] == ':') {
    n = readLine(outBuf, sizeof(outBuf), REPLY_TIMEOUT_MS);
  }

  if (n > 0) {
    udp.beginPacket(rip, rport);
    udp.write(outBuf, n);
    udp.endPacket();
  }

  // Debug: odkomentuj, zeby widziec ruch w Serial Monitorze
  // Serial.write(inBuf, len); Serial.print(" -> "); Serial.write(outBuf, n); Serial.println();

  digitalWrite(LED_PIN, LOW);
}
