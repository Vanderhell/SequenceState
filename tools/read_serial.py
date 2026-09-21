import sys
import time
import re

import serial


port = sys.argv[1] if len(sys.argv) > 1 else "COM37"
timeout = float(sys.argv[2]) if len(sys.argv) > 2 else 35.0
summary = len(sys.argv) > 3 and sys.argv[3] == "summary"
stream = serial.Serial(port, 115200, timeout=0.25)
end = time.time() + timeout
data = bytearray()
try:
    # USB-Serial/JTAG boards commonly use RTS/DTR as the reset/run handshake.
    stream.dtr = False
    stream.rts = True
    time.sleep(0.15)
    stream.rts = False
    while time.time() < end:
        chunk = stream.read(4096)
        if chunk:
            if not summary:
                sys.stdout.write(chunk.decode("utf-8", errors="replace"))
                sys.stdout.flush()
            data.extend(chunk)
        if b"DONE" in data:
            break
finally:
    stream.close()

if summary:
    text = data.decode("utf-8", errors="replace")
    for line in re.split(r"\r?\n", text):
        if re.search(r"chip=|idf=|TEST |BENCH |LONG |HEAP final|DONE", line):
            print(line)
