# Sky-Watcher Star Discovery MAK 127: sterowanie przez WiFi na ESP

10.10.2026, aktualizacja 15.10.2026 · Mariusz

Interfejs pilota, protokół i wszystkie wyniki testów są w [komunikacja_star_discovery.md](komunikacja_star_discovery.md). Ten dokument opisuje budowę: elementy, okablowanie, firmware, obudowę, uruchomienie i aplikację.

## Cel projektu

Moduł ESP z własnym firmware zastępuje pilota SynScan i pozwala sterować montażem Star Discovery z iPhone'a przez WiFi. Koszt to ok. 40-60 zł zamiast ok. 300-400 zł za oryginalny adapter Sky-Watcher SynScan WiFi.

- **Montaż:** Sky-Watcher Star Discovery (AZ GoTo) z tubą MAK 127.
- **Zasada działania:** ESP wpinasz kablem spiralnym w miejsce pilota. Montaż zasila moduł (12 V) i rozmawia z nim przez UART TTL 9600 8N1, jedną wspólną linią danych.
- **Sterowanie:** aplikacja SynScan Pro widzi ESP jak oryginalny adapter WiFi (UDP, port 11880, IP 192.168.4.1) i przejmuje kalibrację, GoTo i śledzenie.
- **Pilot:** zostaje jako zapas. Montaż ma jedno gniazdo, więc działa albo pilot, albo ESP.
- **Stan (15.10.2026):** SynScan Pro łączy się i steruje montażem na ESP8266 NodeMCU i na ESP32 NodeMCU-32S. GoTo jeszcze nietestowane.

## Lista elementów

Wszystko poza obudową kupisz na Allegro, ceny są orientacyjne.

| Element | Ilość | Uwagi | Cena (zł) |
| --- | --- | --- | --- |
| NodeMCU ESP8266 v2 (Amica) **albo** ESP32 NodeMCU-32S (WROOM-32) | 1 | Obie płytki sprawdzone z SynScan Pro. ESP32: nie WROVER, nie S2/S3/C3 | 15-35 |
| Przetwornica Mini360 | 1 | 12 V na 5 V, 17,2 x 11,2 mm | 5-10 |
| Gniazdo RJ45 8p8c na PCB | 1 | Korpus 19 x 13 mm, nóżki 2,5 mm | 2-5 |
| Rezystor 1,1 kΩ | 2 (ESP8266) / 1 (ESP32) | Na TX do linii danych. ESP8266: dwa równolegle (ok. 550 Ω). Zamiennik: 470-680 Ω | <1 |
| Przewód AWG 24-26 | ok. 0,5 m | Lutowany bezpośrednio, nie dupont | 2-5 |
| Koszulka termokurczliwa | kilka cm | Na każde lutowanie | 2 |
| Śruby M3 x 20 | 4 | Wkręcane w plastik (otwór 2,7 mm) | 2 |
| Taśma dwustronna piankowa 0,5 mm | kawałek | Na filar pod płytką | 5 |
| Obudowa (druk 3D, PETG/ASA) | 1 kpl. | Pliki obudowa\_base.stl i obudowa\_lid.stl | ok. 3 |

Potrzebne narzędzia: lutownica, multimetr, komputer z arduino-cli i kabel micro USB **z transmisją danych**. Do diagnostyki przydaje się przejściówka FT232 (zworka na 3,3 V).

## Schemat połączeń

Gniazdo RJ45 w obudowie zastępuje gniazdo pilota, więc podłączasz do niego oryginalny kabel spiralny (RJ12 w montażu, RJ45 po stronie pilota).

**Najważniejsze:** Star Discovery ma **jedną wspólną linię danych** w obie strony. Piny 5 i 6 RJ45 są zwarte w montażu. Pinout EQDIRECT (HEQ5) z osobnymi RX i TX do tego montażu nie pasuje.

| RJ45 w obudowie | Dokąd | Uwagi |
| --- | --- | --- |
| pin 7 (+12 V) | Mini360 IN+ | Zmierzone 12 V |
| pin 4 (GND) | Mini360 IN-, GND płytki | Osobny przewód. Zmierzyć ~0 Ω między GND płytki a pinem 4 |
| Mini360 OUT+ (5,0 V) | VIN płytki | Ustaw 5,0 V przed podłączeniem płytki |
| piny 5 i 6 (linia danych) | RX płytki bezpośrednio + TX płytki przez rezystor | Patrz tabela niżej |
| pin 2 | niepodłączony | Wejście bufora w montażu, funkcja nieznana |
| piny 1, 3, 8 | niepodłączone | |

```
TX płytki ──[R]──┬── RJ45 pin 5/6 (dane)
RX płytki ───────┘
GND płytki ─────── RJ45 pin 4
```

| Płytka | TX | RX | R |
| --- | --- | --- | --- |
| ESP8266 NodeMCU | **D1** (GPIO5) | **D7** (GPIO13) | 2 x 1,1 kΩ równolegle (ok. 550 Ω) |
| ESP32 NodeMCU-32S | **GPIO17** (TX2) | **GPIO16** (RX2) | 1,1 kΩ |

**ESP8266:** nie używaj pinów RX/TX płytki (siedzi na nich chip USB) i **niczego nie podłączaj do D8**. D8 musi mieć stan niski przy starcie, inaczej płytka nie uruchamia programu.

**Dlaczego taki rezystor:** linia montażu jest podciągana w montażu przez ok. 4,4 kΩ (wyliczone z pomiaru). Stan niski z ESP musi zejść wyraźnie poniżej ok. 0,66 V. Przy 1,1 kΩ na ESP8266 zmierzono 0,66 V i montaż gubił bity. Za mały rezystor (47 Ω albo brak) zagłusza odpowiedź montażu. Szczegóły: [komunikacja_star_discovery.md](komunikacja_star_discovery.md).

**Poziom napięć:** 3,3 V TTL (zmierzone). Konwerter poziomów niepotrzebny.

**Ochrona:** kabel wpinaj i wypinaj tylko przy wyłączonym montażu. Wtyk nie gwarantuje, że masa połączy się pierwsza, a RX jest podłączony bez rezystora.

**USB:** nie podłączaj USB do komputera, gdy płytka jest zasilana z montażu przez Mini360. Do testów z USB odłącz 12 V od Mini360.

**Alternatywa: gniazdo RJ12 zamiast RJ45** (bezpośrednio do gniazda w montażu, tylko kablem PROSTYM 1:1): pin 3 +12 V, pin 5 GND, piny 2 i 4 linia danych (zwarte w montażu), pin 6 nie podłączać, pin 1 wolny.

## Firmware

Firmware jest w repozytorium, kompilujesz i wgrywasz go przez arduino-cli (komendy w CLAUDE.md, sekcja "Komendy").

| Płytka | Firmware | Sieć WiFi | Diagnostyka |
| --- | --- | --- | --- |
| ESP8266 | `firmware/SkyWatcherWiFi_Debug` (`MOUNT_MODE 2`) | `SynScan_WiFi_1234`, otwarta | strona http://192.168.4.1 |
| ESP32 | `firmware/SkyWatcherWiFi_ESP32` | `SynScan_WiFi_1234`, hasło `synscan123` | log i komendy testowe przez USB 115200 |

Gotowe obrazy: `bin/SkyWatcherWiFi_Debug_nodemcuv2.bin` (ESP8266) i `bin/SkyWatcherWiFi_ESP32_nodemcu32s.merged.bin` (ESP32, adres 0x0).

Firmware ESP8266:
- filtruje echo komend (wspólna linia),
- przepuszcza do aplikacji tylko poprawne odpowiedzi (`=` + cyfry hex + `\r` albo `!` + kod), przekłamane odrzuca i aplikacja ponawia komendę,
- ma stronę debugera z logiem i licznikami ("Błędy ramki", "Odrzucone odpowiedzi", "Brak odpowiedzi").

Nie używaj `firmware/SkyWatcherWiFi_ESP8266` ani `firmware/synscan_esp32_bridge`: zakładają stary pinout z osobnymi RX i TX.

Gotowy firmware z internetu ([Eqdirect\_Bridge](https://github.com/DIYAstro/Eqdirect_Bridge), [vlaate](https://github.com/vlaate/SkyWatcherWiFi)) nie był testowany na Star Discovery. Działa tylko, jeśli obsługuje jedną wspólną linię z filtrem echa i dobierzesz rezystor jak wyżej.

## Obudowa 3D

Obudowa ma ok. 84 x 52 x 20 mm i składa się z podstawy i pokrywy przykręcanej 4 śrubami M3. Pliki: obudowa\_base.stl, obudowa\_lid.stl i parametryczne źródło obudowa\_synscan.scad (OpenSCAD). Pasuje do NodeMCU Amica i NodeMCU-32S.

| Element | Mocowanie |
| --- | --- |
| ESP | Filar pod środkiem płytki, 4 słupki narożne, docisk modułu bolcem pokrywy z wciskiem 0,4 mm |
| Mini360 | Podest 8 mm, 2 sprężyste zatrzaski na krótszych bokach |
| RJ45 | Szyny 2,8 mm, ścianki boczne, tylne ograniczniki, kołnierz w otworze ściany, docisk pokrywy z wciskiem 0,3 mm |

Pozostałe cechy: otwór micro USB 8,6 x 4,2 mm z zewnętrzną wnęką na wtyk, szczeliny wentylacyjne w dłuższych ścianach i w pokrywie.

**Najważniejsze parametry w pliku .scad**

| Parametr | Wartość | Znaczenie |
| --- | --- | --- |
| esp\_l, esp\_w | 48 / 25,5 mm | Wymiary płytki (NodeMCU-32S; Amica 48,3 x 26) |
| esp\_z | 9,8 mm | Spód płytki nad dnem (goldpiny 9 mm + 0,8 mm) |
| hold\_gap | -0,4 mm | Docisk modułu, ujemny = wcisk |
| rj\_w, rj\_h, rj\_d | 16 / 13 / 19 mm | Korpus RJ45 (szerokość 16 mm do potwierdzenia) |
| rj\_gap | -0,3 mm | Docisk RJ45 |
| screw\_pilot | 2,7 mm | 4,0 mm pod wtopki M3 |

**Druk:** PETG lub ASA (PLA mięknie w nagrzanym aucie), warstwa 0,2 mm, 3 obrysy, wypełnienie 20%, bez podpór. Pokrywa jest już obrócona do druku.

Przed pełnym wydrukiem przerwij druk podstawy po 3-4 mm i sprawdź, czy płytka mieści się między słupkami.

## Montaż i uruchomienie

Kolejność jest ważna: najpierw pomiary i ustawienie 5 V, dopiero potem ESP.

1. **Ustawienie Mini360.** Podłącz do niej samo 12 V i potencjometrem ustaw 5,0 V na wyjściu. Fabryczne ustawienie bywa dowolne.
2. **Wgranie firmware** przez USB, zanim płytka trafi do obudowy.
3. **Lutowanie płytki.** Przewody lutuj z boku goldpinów, tuż pod plastikową listwą, i prowadź płasko. Pod końcami pinów zostaje tylko 0,8 mm. Rezystory TX przylutuj w przewodzie i zaizoluj koszulką.
4. **Lutowanie RJ45.** Tak samo z boku nóżek, przewody wyprowadź środkiem z tyłu gniazda.
5. **Pomiary przed pierwszym włączeniem** (montaż wyłączony): GND płytki do pinu 4 ~0 Ω, RX do pinu 5/6 ~0 Ω, TX do pinu 5/6 ok. 550 Ω (ESP8266) albo 1,1 kΩ (ESP32), nic na D8 (ESP8266).
6. **Montaż w obudowie.** Mini360 wciśnij w zatrzaski, płytkę połóż na filarze z taśmą, przykręć pokrywę.
7. **Test.** Wepnij kabel, włącz montaż. Płytka mrugnie 3 razy (ESP8266), pojawi się sieć `SynScan_WiFi_1234`. Na stronie http://192.168.4.1 kliknij `:e1`: oczekiwane `:e1\r=0212A2\r` w wierszu MNT.
8. **SynScan Pro:** Connect, test strzałek, potem krótkie GoTo bez tuby. Dokręć sprzęgła obu osi: luźne sprzęgło daje skoki pozycji.

## Oprogramowanie iOS

Na iPhone używasz SynScan Pro. Zwykły SynScan jest przygotowany pod iPada i nie wyświetla się w wyszukiwarce na iPhonie.

| Aplikacja | Rola | Koszt |
| --- | --- | --- |
| [SynScan Pro](https://apps.apple.com/app/id1302270121) | Połączenie z montażem, kalibracja, GoTo, śledzenie. Wymagana | Darmowa, iOS 14+ |
| SkySafari Plus | Wygodna mapa nieba, GoTo z mapy. Rekomendacja | Płatna |
| Stellarium Mobile Plus | Realistyczne niebo, słabsze sterowanie | Płatna |

Aplikacje z mapami nieba łączą się z montażem przez SynScan Pro, który musi działać w tle. Kalibrację zawsze robisz w SynScan Pro.

Gdy iOS zgłosi brak internetu w sieci `SynScan_WiFi_1234`, wybierz "Pozostań w tej sieci" i wyłącz Asystenta Wi-Fi. Na czas sesji wyłącz też automatyczną blokadę ekranu, bo iPhone po uśpieniu ekranu zrywa połączenie WiFi.

Zapytania `AT+CWMODE_CUR?` w logu debugera to aplikacja szukająca oryginalnego adaptera. Brak odpowiedzi jest normalny.

## Rozwiązywanie problemów

| Objaw | Przyczyna | Rozwiązanie |
| --- | --- | --- |
| Mac nie widzi płytki | Kabel tylko do ładowania albo hub USB | Kabel z danymi, bezpośrednio do komputera |
| Płytka restartuje się w kółko | Słabe zasilanie z USB albo coś na D8 | Inny kabel, odłącz wszystko od D8 |
| ESP8266: `boot mode:(7,0)` w logu startowym | Coś podciąga D8 | Odłącz D8 |
| SynScan: "nie można wykryć trybu montażu" | Brak odpowiedzi montażu | Debuger `:e1`, potem tabela niżej |
| W logu samo echo `:e1\r`, bez odpowiedzi | Brak masy, przewód TX bez kontaktu, za duży rezystor TX | Pomiary z punktu 5 uruchomienia |
| Komendy z przekłamanymi bitami, montaż odpowiada `!0`/`!3` | Stan niski za wysoki | Mniejszy rezystor TX (nie poniżej ok. 220 Ω) |
| Licznik "Odrzucone odpowiedzi" rośnie | Przekłamania na linii | Krótsze przewody, lutowanie zamiast płytki stykowej, zasilanie z montażu bez USB |
| Skoki pozycji jednej osi | Luźne sprzęgło | Dokręć sprzęgło |
| Telefon traci połączenie | iOS przechodzi na LTE | "Pozostań w tej sieci", wyłącz Asystenta Wi-Fi |
| Pokrywa się wybrzusza | Za duży docisk | hold\_gap na -0,2 albo zeszlifuj bolec |

Pełna tabela diagnostyczna: [komunikacja_star_discovery.md](komunikacja_star_discovery.md).

**Ryzyka**

- 12 V na pinie danych lub 5 V na VIN bez Mini360 niszczy płytkę. Zawsze mierz przed podłączeniem.
- Część odpowiedzi montażu przychodzi przekłamana (ok. 12% na ESP8266 w teście z USB i FT232). Firmware odrzuca przekłamane odpowiedzi, a aplikacja ponawia komendę. Przyczyna nieznana, do sprawdzenia w układzie docelowym.
- Wyładowany telefon kończy sesję. Pilot trzymaj w zestawie.

## Źródła

- [IceInSpace: pinout kabla RJ12-RJ45](https://www.iceinspace.com.au/forum/showthread.php?t=169962): mapowanie pinów, RX i TX zwarte w Virtuoso
- [emard/esp32synscan](https://github.com/emard/esp32synscan): pinout RJ12, 9600 8N1, 3,3 V
- [vlaate/SkyWatcherWiFi](https://github.com/vlaate/SkyWatcherWiFi): pierwowzór firmware
- [Cloudy Nights: DIY WiFi adapter](https://www.cloudynights.com/forums/topic/656206-diy-wifi-adapter-for-skywatcher-telescope-mounts/page/3/): pomiary oscyloskopem na Virtuoso i HEQ5
- [DIYAstro Eqdirect\_Bridge](https://github.com/DIYAstro/Eqdirect_Bridge): alternatywny firmware ESP32, nietestowany na Star Discovery
- [Instrukcja SynScan V4 (Sky-Watcher, PDF)](https://inter-static.skywatcher.com/downloads/Synscan_V4_Hand_Control_Manual_SSHCV4-F-161208V1-EN.pdf)
- [SynScan Pro w App Store](https://apps.apple.com/app/id1302270121)
