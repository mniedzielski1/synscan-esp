# Sky-Watcher Star Discovery: interfejs pilota i protokół

14.10.2026 (aktualizacja: ESP32, ESP8266, filtr odpowiedzi) · Mariusz

Dokument opisuje, jak naprawdę wygląda komunikacja z montażem Star Discovery przez gniazdo pilota. Wszystko poniżej zostało zmierzone albo przetestowane na tym egzemplarzu, chyba że opis mówi wprost "wnioskowanie".

## Najważniejsze w skrócie

- **Jedna wspólna linia danych** w obie strony (pół-dupleks), a nie osobne RX i TX. Pinout EQDIRECT (HEQ5) do tego montażu NIE pasuje.
- **9600 8N1, poziomy 3,3 V.** Konwerter poziomów niepotrzebny.
- Montaż odsyła **echo** każdej komendy, a za nim odpowiedź: `:e1\r=0212A2\r`.
- TX adaptera idzie na linię przez **rezystor**: 1,1 kΩ dla FT232 i ESP32, około 550 Ω (2 x 1,1 kΩ równolegle) dla ESP8266. Przy 47 Ω albo bez rezystora odpowiedź montażu ginie.
- RX adaptera idzie na tę samą linię **bezpośrednio**.
- **Wspólna masa** (GND adaptera do pinu 4 RJ45) jest konieczna. Mierzyć: ma być około 0 Ω.
- Rezystor podciągający linię w montażu: około **4,4 kΩ** (wyliczone z pomiaru 0,66 V przy 1,1 kΩ). Stan niski nadawany przez adapter musi być wyraźnie poniżej około 0,66 V (próg montażu, wnioskowanie z typowych danych PIC).
- Działa z SynScan Pro na **ESP32 NodeMCU-32S** i na **ESP8266 NodeMCU** (14.10.2026).

## Płyta sterownika

Zdjęcia i pomiary z 13-14.10.2026.

| Element | Opis |
| --- | --- |
| Płyta | Synta MC006 Rev. F 2013.01, naklejka AZDBN |
| Procesor | Microchip PIC18F45K20 |
| Zasilanie logiki | U9, stabilizator 3,3 V (oznaczenie 17-33) |
| Bufor gniazda pilota | U8, 74LV125A (4 bufory z wyjściem trójstanowym) |
| R38 | 22 Ω, gniazdo HC1 do U8 pin 8 (3Y, wyjście bufora) |
| R39 | 22 Ω, gniazdo HC1 do U8 pin 2 (1A, wejście bufora) |
| Gniazdo pilota HC1 | RJ12 (6 pinów). Piny 2 i 4 zwarte ścieżką na płycie |

Wnioskowanie: montaż nadaje na wspólną linię przez bufor U8, który włącza tylko na czas odpowiedzi. Dlatego obca, silnie sterowana linia (TX adaptera przez małą rezystancję) zagłusza odpowiedź.

## Okablowanie

### Piny

| RJ45 (kabel spiralny, strona pilota) | RJ12 (gniazdo w montażu) | Funkcja | Podłączenie adaptera |
| --- | --- | --- | --- |
| 7 | 3 | +12 V | Mini360 IN+ (wyjście 5,0 V na VIN ESP) |
| 4 | 5 | GND | GND adaptera, osobnym przewodem |
| 5 i 6 (zwarte w montażu) | 4 i 2 | **dane, w obie strony** (R38) | RX bezpośrednio + TX przez rezystor |
| 2 | 6 | wejście bufora (R39), funkcja nieznana | niepodłączony |
| 1, 3, 8 | 1 | brak | niepodłączone |

Mapowanie RJ12 na RJ45 pochodzi z [IceInSpace](https://www.iceinspace.com.au/forum/showthread.php?t=169962) i zgadza się z pomiarami ciągłości.

### Schemat

```
                 R
TX adaptera ────[===]────┬──── RJ45 pin 5/6 (dane)
RX adaptera ─────────────┘
GND adaptera ─────────────────── RJ45 pin 4 (GND)
                                 RJ45 pin 2: niepodłączony
```

| Adapter | TX | RX | R | Firmware | Stan |
| --- | --- | --- | --- | --- | --- |
| FT232 (test z komputera) | TX, zworka na 3,3 V | RX | 1,1 kΩ | `tools/serial_test.py` | 200/200 odpowiedzi OK |
| ESP32 NodeMCU-32S | GPIO17 (TX2) | GPIO16 (RX2) | 1,1 kΩ | `firmware/SkyWatcherWiFi_ESP32` | SynScan Pro działa, test USB 112/120 odpowiedzi OK |
| ESP8266 NodeMCU (Amica) | D1 (GPIO5), programowy, otwarty dren | D7 (GPIO13), sprzętowy UART0 po `Serial.swap()` | 2 x 1,1 kΩ równolegle | `firmware/SkyWatcherWiFi_Debug` (`MOUNT_MODE 2`) | SynScan Pro działa, 15 błędów ramki na 127 odpowiedzi |

### ESP8266: dlaczego D1 i D7, a nie RX/TX

| Próba | Wynik |
| --- | --- |
| Sprzętowy UART0 na RX/TX (GPIO3/1) z USB | brak nawet echa: chip USB (CP2102) na GPIO3 trzyma linię |
| Programowy UART: TX D1, RX D2 | nadawanie OK, odbiór przy montażu same śmieci |
| **TX programowy D1, RX sprzętowy D7** | **działa** |
| Swap UART0 na D7/D8 z TX na D8 | odrzucone: D8 (GPIO15) musi mieć stan niski przy starcie, linia go podciąga i płytka nie startuje (`boot mode:(7,0)`) |

Do **D8 nic nie podłączać**, nawet przypadkiem przez rząd płytki prototypowej.

### Dobór rezystora na TX (wyniki testów)

| Wariant | Wynik |
| --- | --- |
| 47 Ω, TX i RX na osobnych przewodach (pinout EQDIRECT) | brak odpowiedzi albo pojedynczy bajt `\x00` |
| 2,2 kΩ, wspólna linia | odpowiedzi poprawne, echo często przekłamane |
| **1,1 kΩ, wspólna linia, FT232 / ESP32** | FT232: 200/200 odpowiedzi poprawnych, echo 195/200, ruch obu osi poprawny |
| 1,1 kΩ, ESP8266 | stan niski 0,66 V (zmierzone), montaż gubi bity, odpowiada `!0`, `!3` albo wcale |
| **2 x 1,1 kΩ równolegle (ok. 550 Ω), ESP8266** | stan niski ok. 0,37 V (wyliczone), montaż rozumie każdą komendę |
| BAT85 zamiast rezystora (katoda do TX) | nie zadziałało, brak sygnału na linii (prawdopodobnie odwrotnie wlutowana, nie potwierdzone) |

Przy otwartym drenie (ESP8266) rezystora nie zmniejszać poniżej ok. 220 Ω: przy jednoczesnym nadawaniu ESP i montażu prąd pinu rośnie.

Przekłamane echo nie przeszkadza: filtr echa w firmware odcina wszystko przed znakiem `=`.

### Zielony przewód (RJ45 pin 2)

- Idzie na wejście bufora 1A. Montaż nie reaguje na komendy podane na ten pin.
- W czasie odpowiedzi montażu widać na nim stan niski przez cały czas odpowiedzi, bez czytelnych bitów. Wnioskowanie: jest połączony z linią danych przez około 10 kΩ (zmierzone między przewodami).
- Funkcja nieznana. Nie podłączać.

## Protokół

Sterownik silników Sky-Watcher, ten sam co w innych montażach Synta.

### Format

- Komenda: `:` + litera + oś + dane + `\r`. Oś `1` to azymut, `2` to wysokość.
- Odpowiedź: `=` + dane + `\r` (OK) albo `!` + kod + `\r` (błąd).
- Na wspólnej linii najpierw wraca echo komendy, potem odpowiedź.
- Liczby 24-bitowe w hex, bajty od najmłodszego: `230280` to bajty `23 02 80`, czyli 0x800223.
- Pozycja przesunięta o 0x800000: 0x800223 to +547 kroków.

### Parametry tego montażu

| Komenda | Odpowiedź | Znaczenie |
| --- | --- | --- |
| `:e1`, `:e2` | `=0212A2` | wersja firmware sterownika |
| `:a1` | `=3BBA0F` | 1 030 715 kroków na obrót, azymut |
| `:a2` | `=4FF814` | 1 374 287 kroków na obrót, wysokość |
| `:b1`, `:b2` | `=204E00` | częstotliwość timera 20 000 Hz |
| `:j1`, `:j2` | `=000080` | pozycja po włączeniu: 0x800000 (zero) |
| `:f1`, `:f2` | `=000` | status po włączeniu |

### Komendy

| Komenda | Rodzaj | Opis |
| --- | --- | --- |
| `:e` `:a` `:b` `:f` `:j` | odczyt | wersja, kroki/obrót, timer, status, pozycja. Bezpieczne |
| `:G` + 2 cyfry | ruch | tryb ruchu, patrz niżej |
| `:I` + 6 cyfr | ruch | prędkość: liczba taktów timera na krok |
| `:J` | ruch | start |
| `:K` | ruch | stop z hamowaniem |
| `:L` | ruch | natychmiastowy stop |
| `:S` + 6 cyfr | ruch | pozycja docelowa GoTo |
| `:E` + 6 cyfr | stan | ustawienie licznika pozycji |
| `:F` | stan | inicjalizacja osi (wysyła SynScan Pro) |

Komend ruchu nie wysyłać z narzędzi testowych bez nadzoru i bez zdjętej tuby.

### Komenda `:G` (tryb ruchu)

Znaczenie cyfr z opisu protokołu Sky-Watcher (z pamięci, potwierdzone testem tylko dla `10` i `11`).

| 1. cyfra | Tryb |
| --- | --- |
| 0 | GoTo szybkie |
| 1 | ruch ciągły wolny (śledzenie, strzałki) |
| 2 | GoTo wolne |
| 3 | ruch ciągły szybki |

| 2. cyfra | Kierunek |
| --- | --- |
| 0 | zgodnie z ruchem wskazówek zegara, pozycja rośnie |
| 1 | przeciwnie, pozycja maleje |

### Status `:f`

Trzy cyfry hex. Znaczenie bitów z opisu protokołu, potwierdzone testem dla kierunku i ruchu.

| Cyfra | Bit 0 | Bit 1 | Bit 2 |
| --- | --- | --- | --- |
| 1. | 1 = ruch ciągły, 0 = GoTo | 1 = kierunek przeciwny | 1 = tryb szybki |
| 2. | 1 = oś jedzie | 1 = oś zablokowana | |
| 3. | 1 = oś zainicjalizowana | | |

Zaobserwowane: `100` po ruchu w kierunku 0, `300` po ruchu w kierunku 1, druga cyfra `0` po zatrzymaniu.

### Przykład: wolny obrót azymutu

Zapis z testu FT232, 13.10.2026.

| Wysłane | Odebrane | Opis |
| --- | --- | --- |
| `:K1\r` | `:K1\r=\r` | stop |
| `:G110\r` | `:G110\r=\r` | ruch ciągły wolny, kierunek 0 |
| `:I1070000\r` | `:I1070000\r=\r` | prędkość 7 taktów na krok |
| `:J1\r` | `:J1\r=\r` | start |
| `:j1\r` | `:j1\r=230280\r` | pozycja +547 |
| `:K1\r` | `:K1\r=\r` | stop |
| `:f1\r` | `:f1\r=100\r` | ruch ciągły, kierunek 0, stoi |

Przy `:I` = 7 wzór (20 000 / 7) daje około 2860 kroków/s, a zmierzone było około 630-730 kroków/s (1261-1468 kroków w 2 s z rozpędzaniem i hamowaniem). Wnioskowanie: montaż w trybie wolnym ogranicza prędkość albo stosuje dodatkowy dzielnik. SynScan Pro przelicza prędkości sam.

### Przykład: GoTo (nietestowane, kolejność jak w SynScan Pro)

1. `:K1`: stop.
2. `:G100`: GoTo szybkie, kierunek 0.
3. `:S1xxxxxx`: pozycja docelowa.
4. `:J1`: start. Montaż sam przyspiesza i hamuje.
5. `:f1` co chwilę, aż 2. cyfra statusu wróci na `0`.

## Testy ruchu

13-14.10.2026, bez tuby, FT232 z rezystorem 1,1 kΩ. Ruch 2 s, `:G?1x`, `:I?070000`.

| Oś | Przejechane kroki (8 prób) | Uwagi |
| --- | --- | --- |
| Azymut | ±1261 do ±1266 | powtarzalne |
| Wysokość, sprzęgło luźne | +2930, -1620, +1173, -1136 | skoki licznika do ok. 3000 kroków w ciągu 30 ms |
| Wysokość, sprzęgło dokręcone | ±1459 do ±1468 | powtarzalne, wraca na 0 ±6 kroków |

Po dokręceniu sprzęgła zostaje jedna korekta o około 110-130 kroków (ok. 0,03°) około 0,5 s po starcie. Wnioskowanie: korekta enkodera Freedom Find. Przy nieruchomym montażu 400 odczytów pozycji było identycznych, więc skoki nie wynikały z komunikacji.

**Zalecenie:** przed użyciem dokręcić sprzęgła obu osi. Luźne sprzęgło wysokości daje skoki pozycji.

## Narzędzia

### Test z komputera przez FT232

Okablowanie jak w tabeli "Schemat", zworka FT232 na 3,3 V, VCC z FT232 niepodłączone do montażu.

```bash
pip3 install pyserial
python3 tools/serial_test.py                 # odczyty :e :a :f :j obu osi
python3 tools/serial_test.py --scan          # :e1 na 2400...115200
python3 tools/serial_test.py --sniff         # tylko nasłuch
python3 tools/serial_test.py -p /dev/cu.usbserial-XXXX
```

Oczekiwany wynik:

```
:e1   ->  :e1\r=0212A2\r
:a1   ->  :a1\r=3BBA0F\r
:j1   ->  :j1\r=000080\r
```

### Debuger HTTP w firmware ESP8266

Po połączeniu z siecią `SynScan_WiFi_1234`:

| Adres | Funkcja |
| --- | --- |
| `http://192.168.4.1` | strona debugera z logiem APP/MNT |
| `/log` | log i liczniki w JSON |
| `/send?c=:e1` | wysłanie komendy testowej |
| `/echo?on=0` | wyłączenie filtra echa |
| `/scan` | `:e1` na 2400...115200, powrót na 9600 |
| `/baud?b=9600` | zmiana prędkości |
| `/sniff?on=1` | podsłuch, TX odłączony programowo |
| `/clear` | czyszczenie logu |

Liczniki na stronie:

| Licznik | Znaczenie |
| --- | --- |
| Błędy ramki | UART odebrał bajt z błędem (zła prędkość, poziomy, zakłócenia). Dotyczy też echa, więc część jest nieszkodliwa |
| Odrzucone odpowiedzi | odpowiedź montażu nie przeszła kontroli formatu i NIE poszła do aplikacji. Aplikacja ponawia komendę |
| Brak odpowiedzi | montaż milczał dłużej niż 300 ms |

**Filtr odpowiedzi** (`replyValid`): do SynScan Pro trafia tylko `=` + cyfry 0-9/A-F + `\r` albo `!` + cyfry + `\r`. Przekłamanie typu `=u01` jest odrzucane. Nie wykryje zamiany jednej cyfry hex na inną. Wyłączenie filtra echa (`/echo?on=0`) wyłącza też ten filtr.

W trybie `MOUNT_MODE 2` USB płytki nie pokazuje logu (UART0 zajęty przez montaż), diagnostyka tylko przez WiFi.

### Komendy testowe przez USB

| Firmware | Jak | Odpowiedź |
| --- | --- | --- |
| ESP32 (`SkyWatcherWiFi_ESP32`) | 115200, wpisać `:e1` + Enter | log `TEST` / `MNT` z surową odpowiedzią |
| ESP8266 `MOUNT_MODE 1` | jak wyżej, plus `?` (test D1 -> D2) i `0` (D1 = 0 przez 15 s do pomiaru napięcia) | log `TEST` / `MNT`, `PINY ...` |

## Diagnostyka: objawy i przyczyny

| Objaw | Przyczyna (potwierdzona w tym projekcie) |
| --- | --- |
| SynScan Pro: "nie można wykryć trybu montażu" | brak poprawnej odpowiedzi montażu, patrz niżej |
| na każdą komendę jeden bajt `\x00` | RX słucha pinu 2 (zielony) zamiast linii danych |
| `\x00` tylko na 9600, tylko po `\r` | komenda dochodzi, odpowiedź czytana z niewłaściwego przewodu |
| `BRAK` na wszystkich prędkościach | TX na pinie 2 (zielony), montaż nie dostaje komendy |
| GND adaptera do pinu 4: kilka kΩ | brak wspólnej masy, dolutować osobny przewód |
| samo echo `:e1\r` bez odpowiedzi | za mały rezystor na TX, zagłusza odpowiedź montażu |
| odpowiedzi OK, echo przekłamane | za duży rezystor na TX, zmniejszyć do ok. 1 kΩ |
| samo echo, na podsłuchu FT232 też tylko echo | przewód TX adaptera nie ma kontaktu z linią (płytka prototypowa, dupont) |
| ESP8266: komendy z bitami 0 zamienionymi na 1 (`:g1`, `:jr`) | stan niski za wysoki (0,66 V), zmniejszyć rezystor TX |
| ESP8266: `boot mode:(7,0)`, "waiting for host", płytka nie startuje | coś podciąga D8 (GPIO15) |
| Mac nie widzi płytki (brak `/dev/cu.usbserial-*`), restarty płytki | kabel tylko do ładowania albo hub USB. Kabel z danymi, bezpośrednio do Maca |
| port płytki "Invalid argument" albo zalew powtórzonych danych | zawieszony sterownik CP2102 na Macu. Wypiąć i wpiąć USB |
| dwa nadajniki na linii (np. TX FT232 i ESP) | linia ok. 1,65 V, śmieci. Na linii tylko jeden TX |
| skoki pozycji jednej osi | luźne sprzęgło tej osi |
| zapytania `AT+CWMODE_CUR?` w logu | aplikacja szuka oryginalnego adaptera. Brak odpowiedzi jest normalny |

## Ryzyka i otwarte sprawy

- **ESP8266, przekłamania:** ok. 12% odpowiedzi z błędem ramki (15/127, z USB i FT232 podłączonymi). Otwarty dren na D1 tego nie usunął. Przyczyna nieznana; do sprawdzenia w układzie docelowym (zasilanie z montażu przez Mini360, bez USB i FT232, przewody przylutowane). Filtr odpowiedzi chroni aplikację.
- **ESP32:** ok. 7% odpowiedzi z przekłamaniem w teście z FT232 wpiętym równolegle. Nie sprawdzone w układzie docelowym. Firmware ESP32 nie ma jeszcze filtra odpowiedzi (`replyValid`), przekłamania idą do aplikacji.
- **Prędkość w trybie wolnym** niezgodna ze wzorem 20 000 / `:I`. Bez wpływu na SynScan Pro.
- **Pin 2 (zielony):** funkcja nieznana. Ustalić można podsłuchem oryginalnego pilota (rozgałęźnik RJ45, tylko RX i GND).
- **GoTo** jeszcze nietestowane.

## Bezpieczeństwo

1. Kabel do montażu wpinać i wypinać tylko przy wyłączonym montażu. Na wtyku jest 12 V.
2. ESP8266 i ESP32: nie podłączać USB do komputera, gdy płytka jest zasilana z montażu (Mini360 na VIN). Do testów z USB odłączyć 12 V od Mini360.
3. FT232: zworka na 3,3 V, VCC z FT232 nie łączyć z montażem.
4. Mini360 ustawić na 5,0 V przed podłączeniem ESP.
5. Testy ruchu bez tuby. Nietypowy dźwięk silników: odłączyć zasilanie.

## Źródła

- [emard/esp32synscan](https://github.com/emard/esp32synscan): pinout RJ12 (Virtuoso), 9600 8N1, 3,3 V
- [IceInSpace: kabel RJ12 do RJ45 SynScan AZ](https://www.iceinspace.com.au/forum/showthread.php?t=169962): mapowanie pinów, RX i TX zwarte w Virtuoso
- [vlaate/SkyWatcherWiFi](https://github.com/vlaate/SkyWatcherWiFi): pierwowzór firmware
- [Cloudy Nights: DIY WiFi adapter](https://www.cloudynights.com/forums/topic/656206-diy-wifi-adapter-for-skywatcher-telescope-mounts/page/3/): pomiary oscyloskopem, rezystory
- [Astroshop: Star Discovery](https://astroshop.eu/alt-azimuth-with-goto/skywatcher-az-synscan-goto-star-discovery-mount/p,50254): Freedom Find, sprzęgła
