#!/usr/bin/env python3
"""
Test mostu ESP <-> montaz Sky-Watcher bez aplikacji SynScan.
Komputer musi byc polaczony z siecia ESP (SynScan_WiFi_1234).

Uzycie:
  python3 tools/udp_test.py                 # domyslny zestaw: :e1 :e2 :a1 :a2 :f1 :f2 :j1 :j2
  python3 tools/udp_test.py :e1 :j2         # wlasne komendy
  python3 tools/udp_test.py --host 192.168.4.1 --port 11880 --timeout 1.0 :e1

Odpowiedz "=..." = OK, "!..." = blad montazu, brak = montaz nie odpowiada (piny, RX/TX, masa).
Komendy tylko do ODCZYTU. Nie wysylaj :J (start ruchu) bez nadzoru montazu.
"""
import argparse
import socket
import sys

DEFAULT_CMDS = [":e1", ":e2", ":a1", ":a2", ":f1", ":f2", ":j1", ":j2"]


def esc(b: bytes) -> str:
    out = []
    for c in b:
        if c == 13:
            out.append("\\r")
        elif c == 10:
            out.append("\\n")
        elif 32 <= c < 127:
            out.append(chr(c))
        else:
            out.append(f"\\x{c:02X}")
    return "".join(out)


def decode_hex24(data: str):
    """Sky-Watcher: 6 znakow hex, little-endian (bajty odwrocone)."""
    if len(data) != 6:
        return None
    try:
        return int(data[4:6] + data[2:4] + data[0:2], 16)
    except ValueError:
        return None


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("cmds", nargs="*", default=DEFAULT_CMDS)
    ap.add_argument("--host", default="192.168.4.1")
    ap.add_argument("--port", type=int, default=11880)
    ap.add_argument("--timeout", type=float, default=1.0)
    a = ap.parse_args()

    blocked = [c for c in a.cmds if c[1:2] in ("J", "G", "S", "I", "E", "F", "M")]
    if blocked:
        print(f"Odmowa: {blocked} zmieniaja stan montazu. To narzedzie wysyla tylko odczyty.")
        return 2

    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(a.timeout)
    ok = 0
    for c in a.cmds:
        cmd = (c if c.startswith(":") else ":" + c) + "\r"
        s.sendto(cmd.encode(), (a.host, a.port))
        try:
            data, _ = s.recvfrom(1024)
            txt = data.decode(errors="replace").strip()
            extra = ""
            if txt.startswith("=") and c[1:2] in ("a", "j"):
                v = decode_hex24(txt[1:7])
                if v is not None:
                    if c[1] == "j":
                        v -= 0x800000  # pozycja jest przesunieta o 0x800000
                    extra = f"  (= {v})"
            print(f"{esc(cmd.encode()):8} -> {esc(data)}{extra}")
            ok += 1
        except socket.timeout:
            print(f"{esc(cmd.encode()):8} -> BRAK ODPOWIEDZI ({a.timeout}s)")
    print(f"\nOdpowiedzi: {ok}/{len(a.cmds)}")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
