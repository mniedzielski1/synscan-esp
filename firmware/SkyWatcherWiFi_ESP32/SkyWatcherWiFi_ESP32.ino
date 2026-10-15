/*
  SkyWatcherWiFi dla ESP32 (port projektu vlaate/SkyWatcherWiFi)
  https://github.com/vlaate/SkyWatcherWiFi

  Zmiany wzgledem oryginalu (ESP8266):
  - ESP32: UART2 na GPIO16 (RX) i GPIO17 (TX), wiec USB i Serial Monitor dzialaja normalnie
  - usuniety zbedny Serial.read(), ktory gubil pierwszy bajt (poprawka warwound, Cloudy Nights)
  - serialIndex jako uint16_t (w oryginale uint8_t przepelnial sie przy 256 bajtach)
  - haslo do sieci WiFi (w oryginale siec byla otwarta)
  - logi w Serial Monitorze (115200) do diagnostyki

  Podlaczenie (RJ45 w obudowie). Star Discovery ma JEDNA wspolna linie danych:
    pin 5/6 (zwarte w montazu) -> GPIO16 (RX2) bezposrednio
    pin 5/6                    -> 1,1 kOhm -> GPIO17 (TX2)
    pin 4 GND -> GND, pin 7 +12V -> Mini360 (5V) -> VIN
    pin 2: niepodlaczony

  Diagnostyka przez USB (115200): wpisz np. ":e1" + Enter, odpowiedz montazu
  (surowa, z echem) pojawi sie jako "MNT".
*/

#include <WiFi.h>
#include <WiFiUdp.h>

// ---------- KONFIGURACJA ----------
#define AP_NAME       "SynScan_WiFi_1234"  // SynScan Pro wykrywa siec po prefiksie "SynScan"
#define AP_PASSWORD   "synscan123"         // min. 8 znakow; "" = siec otwarta jak w oryginale
#define MOUNT_RX_PIN  16
#define MOUNT_TX_PIN  17
#define MOUNT_BAUD    9600
#define UDP_PORT      11880                // port oczekiwany przez SynScan
#define TIMEOUT_MS    20                   // koniec odpowiedzi, gdy przez tyle ms nic nie przyjdzie
#define BUFFER_SIZE   1024
#define DEBUG         true                 // logi na USB (115200)

WiFiUDP udp;
IPAddress remoteIp;
uint16_t remotePort = 0;

uint8_t udpBuffer[BUFFER_SIZE];
uint8_t serialBuffer[BUFFER_SIZE];
uint16_t serialIndex = 0;

// Montaz moze miec TX i RX na wspolnej linii, wiec odsyla echo komendy.
// Ignorujemy wszystko do znaku '=' lub '!' (poczatek prawdziwej odpowiedzi).
bool ignoreEcho = true;
bool fromUsb = false;    // komenda testowa z USB: odpowiedzi nie wysylamy do aplikacji

void logRaw(const char* dir, const uint8_t* d, size_t len) {
  if (!DEBUG) return;
  Serial.printf("%8.2f  %-4s ", millis() / 1000.0, dir);
  for (size_t i = 0; i < len; i++) {
    uint8_t c = d[i];
    if (c == '\r') Serial.print("\\r");
    else if (c == '\n') Serial.print("\\n");
    else if (c >= 32 && c < 127) Serial.write(c);
    else Serial.printf("\\x%02X", c);
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(MOUNT_BAUD, SERIAL_8N1, MOUNT_RX_PIN, MOUNT_TX_PIN);

  WiFi.mode(WIFI_AP);
  IPAddress ip(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(ip, ip, subnet);
  WiFi.softAP(AP_NAME, strlen(AP_PASSWORD) >= 8 ? AP_PASSWORD : nullptr);
  udp.begin(UDP_PORT);

  if (DEBUG) Serial.printf("\nAP: %s  IP: 192.168.4.1  UDP: %d\n", AP_NAME, UDP_PORT);
}

void loop() {
  // 0. Komenda testowa z USB: linia zaczynajaca sie od ':' idzie do montazu
  static char usbLine[40]; static uint8_t usbLen = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r' || c == '\n') {
      if (usbLen > 1 && usbLine[0] == ':') {
        usbLine[usbLen++] = '\r';
        while (Serial2.available()) Serial2.read();
        Serial2.write((const uint8_t*)usbLine, usbLen);
        ignoreEcho = true; fromUsb = true;
        logRaw("TEST", (const uint8_t*)usbLine, usbLen);
      }
      usbLen = 0;
    } else if (usbLen < sizeof(usbLine) - 2) usbLine[usbLen++] = c;
  }

  // 1. Komenda z aplikacji (WiFi) -> montaz (UART)
  int packetSize = udp.parsePacket();
  if (packetSize > 0) {
    remoteIp = udp.remoteIP();
    remotePort = udp.remotePort();
    int len = udp.read(udpBuffer, BUFFER_SIZE);
    if (len > 0) {
      while (Serial2.available()) Serial2.read();
      Serial2.write(udpBuffer, len);
      ignoreEcho = true; fromUsb = false;
      logRaw("APP", udpBuffer, len);
    }
  }

  // 2. Odpowiedz z montazu (UART) -> aplikacja (WiFi)
  if (Serial2.available()) {
    uint8_t raw[BUFFER_SIZE]; uint16_t rawLen = 0;
    while (true) {
      if (Serial2.available()) {
        uint8_t c = Serial2.read();
        if (rawLen < BUFFER_SIZE) raw[rawLen++] = c;
        if (c == '=' || c == '!') ignoreEcho = false;
        if (!ignoreEcho && serialIndex < BUFFER_SIZE) serialBuffer[serialIndex++] = c;
      } else {
        delay(TIMEOUT_MS);
        if (!Serial2.available()) break;
      }
    }
    logRaw("MNT", raw, rawLen);

    if (serialIndex > 0 && remotePort != 0 && !fromUsb) {
      udp.beginPacket(remoteIp, remotePort);
      udp.write(serialBuffer, serialIndex);
      udp.endPacket();
    }
    serialIndex = 0;
  }
}
