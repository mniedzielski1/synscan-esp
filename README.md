# synscan-esp

Sterowanie montażem **Sky-Watcher Star Discovery** (AZ GoTo) z iPhone'a przez WiFi, aplikacją **SynScan Pro**. Moduł ESP wpięty w gniazdo pilota udaje oryginalny adapter Sky-Watcher SynScan WiFi (UDP 11880, IP 192.168.4.1).

**Stan (15.10.2026):** SynScan Pro łączy się i steruje montażem na ESP8266 NodeMCU i na ESP32 NodeMCU-32S. GoTo jeszcze nietestowane.

## Najważniejsze

- Star Discovery ma **jedną wspólną linię danych** w obie strony (piny RJ45 5 i 6 zwarte w montażu). Pinout EQDIRECT (HEQ5) z osobnymi RX/TX do niego nie pasuje.
- 9600 8N1, 3,3 V TTL. Montaż odsyła echo każdej komendy: `:e1\r=0212A2\r`.
- RX płytki idzie na linię bezpośrednio, TX przez rezystor. Wspólna masa jest konieczna.

## Okablowanie

```
TX płytki ──[R]──┬── RJ45 pin 5/6 (dane)
RX płytki ───────┘
GND płytki ─────── RJ45 pin 4
RJ45 pin 7 (+12 V) ── Mini360 (5,0 V) ── VIN
RJ45 pin 2: niepodłączony
```

| Płytka | TX | RX | R | Firmware |
| --- | --- | --- | --- | --- |
| ESP8266 NodeMCU (Amica) | D1 (GPIO5) | D7 (GPIO13) | 2 x 1,1 kΩ równolegle (ok. 550 Ω) | `firmware/SkyWatcherWiFi_Debug` |
| ESP32 NodeMCU-32S | GPIO17 | GPIO16 | 1,1 kΩ | `firmware/SkyWatcherWiFi_ESP32` |

ESP8266: nie używać pinów RX/TX płytki (chip USB) i nic nie podłączać do D8 (blokuje start).

## Szybki start

```bash
brew install arduino-cli
arduino-cli config init
arduino-cli config add board_manager.additional_urls \
  https://arduino.esp8266.com/stable/package_esp8266com_index.json \
  https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp8266:esp8266      # albo esp32:esp32

# ESP8266
arduino-cli compile -b esp8266:esp8266:nodemcuv2 --output-dir build firmware/SkyWatcherWiFi_Debug
arduino-cli upload -p /dev/cu.usbserial-0001 -b esp8266:esp8266:nodemcuv2 --input-dir build firmware/SkyWatcherWiFi_Debug
```

Gotowe obrazy są w `bin/`. Po starcie pojawia się sieć `SynScan_WiFi_1234` (ESP8266 otwarta, ESP32 hasło `synscan123`). Debuger ESP8266: http://192.168.4.1, przycisk `:e1` powinien dać `:e1\r=0212A2\r`.

## Zawartość

| Ścieżka | Opis |
| --- | --- |
| `firmware/SkyWatcherWiFi_Debug/` | ESP8266: most UDP, debuger HTTP, filtr echa i przekłamanych odpowiedzi |
| `firmware/SkyWatcherWiFi_ESP32/` | ESP32: most na UART2, log i komendy testowe przez USB |
| `firmware/SkyWatcherWiFi_ESP8266/`, `firmware/synscan_esp32_bridge/` | stare wersje ze starym pinoutem, nie działają ze Star Discovery |
| `tools/serial_test.py` | test montażu z komputera przez FT232 (tylko odczyty, skan prędkości, podsłuch) |
| `tools/udp_test.py` | test mostu przez WiFi z komputera |
| `enclosure/` | obudowa 3D (OpenSCAD + STL), ok. 84 x 52 x 20 mm |
| `bin/` | skompilowany firmware |
| `docs/komunikacja_star_discovery.md` | interfejs pilota, płyta sterownika, protokół, wyniki wszystkich testów, diagnostyka |
| `docs/dokumentacja_synscan_esp32.md` | budowa: elementy, okablowanie, obudowa, uruchomienie, iOS |

## Bezpieczeństwo

- Kabel do montażu wpinać i wypinać tylko przy wyłączonym montażu (12 V na wtyku).
- Nie podłączać USB do komputera, gdy płytka jest zasilana z montażu przez Mini360.
- Mini360 ustawić na 5,0 V przed podłączeniem płytki.
- Pierwsze testy ruchu bez tuby.

## Źródła

- [vlaate/SkyWatcherWiFi](https://github.com/vlaate/SkyWatcherWiFi): pierwowzór firmware
- [emard/esp32synscan](https://github.com/emard/esp32synscan): pinout RJ12, parametry linii
- [IceInSpace: kabel RJ12-RJ45](https://www.iceinspace.com.au/forum/showthread.php?t=169962): mapowanie pinów
- [Cloudy Nights: DIY WiFi adapter](https://www.cloudynights.com/forums/topic/656206-diy-wifi-adapter-for-skywatcher-telescope-mounts/page/3/)

## Licencja

[GPL-3.0](LICENSE). Firmware jest oparty na [vlaate/SkyWatcherWiFi](https://github.com/vlaate/SkyWatcherWiFi) (GPL-3.0), z poprawkami z wątku na Cloudy Nights i zmianami dla Star Discovery.
