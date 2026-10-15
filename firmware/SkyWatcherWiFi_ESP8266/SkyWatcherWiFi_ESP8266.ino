/*
  SkyWatcherWiFi dla NodeMCU ESP8266
  Oparte na https://github.com/vlaate/SkyWatcherWiFi z poprawkami z watku Cloudy Nights:
  - usuniety zbedny Serial.read(), ktory gubil pierwszy bajt odpowiedzi (warwound)
  - serialIndex jako uint16_t (w oryginale uint8_t przepelnial sie przy 256 bajtach)
  - opcjonalne haslo do sieci WiFi (w oryginale siec byla otwarta)

  NIEAKTUALNE (15.10.2026): ten szkic NIE dziala ze Star Discovery. Montaz ma jedna wspolna
  linie danych (pin 5/6), a UART0 na RX/TX jest zagluszany przez chip USB. Uzyj
  firmware/SkyWatcherWiFi_Debug (ESP8266) albo firmware/SkyWatcherWiFi_ESP32.

  Podlaczenie (stary pinout EQDIRECT, RJ45 w obudowie):
    pin 5 (TX montazu) -> RX (GPIO3) bezposrednio, BEZ rezystora
    pin 6 (RX montazu) -> 47 ohm -> TX (GPIO1)
    pin 4 GND -> GND
    pin 7 +12V -> Mini360 ustawione na 5V -> Vin

  UWAGA: RX/TX to ten sam UART co USB.
  - Firmware wgrywaj tylko przy odlaczonym montazu.
  - Nie podlaczaj USB do komputera, gdy plytka jest zasilana z montazu.
*/

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

// ---------- KONFIGURACJA ----------
#define AP_NAME      "SynScan_WiFi_1234"  // SynScan Pro wykrywa siec po prefiksie "SynScan"
#define AP_PASSWORD  ""                   // siec otwarta
#define MOUNT_BAUD   9600
#define UDP_PORT     11880                // port oczekiwany przez SynScan
#define TIMEOUT_MS   20                   // koniec odpowiedzi, gdy przez tyle ms nic nie przyjdzie
#define BUFFER_SIZE  1024

WiFiUDP udp;
IPAddress remoteIp;
uint16_t remotePort = 0;

uint8_t udpBuffer[BUFFER_SIZE];
uint8_t serialBuffer[BUFFER_SIZE];
uint16_t serialIndex = 0;

// Montaz ma TX i RX na wspolnej linii, wiec odsyla echo komendy.
// Ignorujemy wszystko do znaku '=' lub '!' (poczatek prawdziwej odpowiedzi).
bool ignoreEcho = true;

void setup() {
  Serial.begin(MOUNT_BAUD);

  WiFi.mode(WIFI_AP);
  IPAddress ip(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(ip, ip, subnet);
  if (strlen(AP_PASSWORD) >= 8) WiFi.softAP(AP_NAME, AP_PASSWORD);
  else                          WiFi.softAP(AP_NAME);
  udp.begin(UDP_PORT);
}

void loop() {
  // 1. Komenda z aplikacji (WiFi) -> montaz (UART)
  int packetSize = udp.parsePacket();
  if (packetSize > 0) {
    remoteIp = udp.remoteIP();
    remotePort = udp.remotePort();
    int len = udp.read(udpBuffer, BUFFER_SIZE);
    if (len > 0) {
      Serial.write(udpBuffer, len);
      ignoreEcho = true;
    }
  }

  // 2. Odpowiedz z montazu (UART) -> aplikacja (WiFi)
  if (Serial.available()) {
    while (true) {
      if (Serial.available()) {
        uint8_t c = Serial.read();
        if (c == '=' || c == '!') ignoreEcho = false;
        if (!ignoreEcho && serialIndex < BUFFER_SIZE) serialBuffer[serialIndex++] = c;
      } else {
        delay(TIMEOUT_MS);
        if (!Serial.available()) break;
      }
    }

    if (serialIndex > 0 && remotePort != 0) {
      udp.beginPacket(remoteIp, remotePort);
      udp.write(serialBuffer, serialIndex);
      udp.endPacket();
    }
    serialIndex = 0;
    yield();
  }
}
