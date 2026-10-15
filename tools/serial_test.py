#!/usr/bin/env python3
"""
Test montazu Sky-Watcher przez przejsciowke USB-UART (FT232), bez ESP.
Wysyla TYLKO komendy odczytu (:e :a :f :j). Wymaga: pip3 install pyserial

  python3 tools/serial_test.py                       # 9600, odczyty
  python3 tools/serial_test.py --scan                # :e1 na kolejnych predkosciach
  python3 tools/serial_test.py --sniff               # tylko nasluch (np. pilot wpiety rownolegle)
  python3 tools/serial_test.py -p /dev/cu.usbserial-XXXX -b 9600
"""
import argparse, glob, sys, time

try:
    import serial
except ImportError:
    sys.exit("Brak pyserial: pip3 install pyserial")

READS = [":e1", ":e2", ":a1", ":a2", ":f1", ":f2", ":j1", ":j2"]
BAUDS = [2400, 4800, 9600, 19200, 38400, 57600, 115200]


def show(b):
    return "".join(
        "\\r" if c == 13 else "\\n" if c == 10 else chr(c) if 32 <= c < 127 else f"\\x{c:02X}"
        for c in b
    )


def ask(s, cmd, wait=0.3):
    s.reset_input_buffer()
    s.write((cmd + "\r").encode())
    s.flush()
    t = time.time()
    out = b""
    while time.time() - t < wait:
        out += s.read(64)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-p", "--port")
    ap.add_argument("-b", "--baud", type=int, default=9600)
    ap.add_argument("--scan", action="store_true")
    ap.add_argument("--sniff", action="store_true")
    a = ap.parse_args()

    port = a.port
    if not port:
        ports = glob.glob("/dev/cu.usbserial-*") + glob.glob("/dev/cu.wchusbserial-*")
        if not ports:
            sys.exit("Nie widze przejsciowki USB-UART (/dev/cu.usbserial-*)")
        port = ports[0]
    print(f"Port {port}")

    s = serial.Serial(port, a.baud, timeout=0.05)

    if a.sniff:
        print(f"Nasluch {a.baud} 8N1, Ctrl+C konczy")
        try:
            while True:
                d = s.read(256)
                if d:
                    print(f"{time.strftime('%H:%M:%S')}  {show(d)}")
        except KeyboardInterrupt:
            return

    if a.scan:
        for b in BAUDS:
            s.baudrate = b
            time.sleep(0.05)
            r = ask(s, ":e1")
            print(f"{b:>6}  :e1  ->  {show(r) if r else 'BRAK'}")
        return

    print(f"{a.baud} 8N1")
    for c in READS:
        r = ask(s, c)
        print(f"{c:4}  ->  {show(r) if r else 'BRAK'}")


if __name__ == "__main__":
    main()
