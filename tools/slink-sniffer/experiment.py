import serial, sys, time
port = sys.argv[1]
steps = [  # (label, frame or None, listen seconds)
  ("idle",            None,          8),
  ("play",            "90 00",      12),
  ("next",            "90 08",       8),
  ("pause",           "90 02",       5),
  ("resume",          "90 00",       6),
  ("stop",            "90 01",       5),
  ("query 0F",        "90 0F",       4),
  ("query 25 (cont)", "90 25",       8),
  ("query 26 (off)",  "90 26",       3),
  ("play disc6 trk1", "90 50 06 01", 15),
  ("stop",            "90 01",       4),
  ("power off",       "90 2F",       6),
  ("power on",        "90 2E",      15),
]
with serial.Serial(port, 115200, timeout=0.2) as s:
    time.sleep(2.5)
    s.reset_input_buffer()
    for label, frame, secs in steps:
        print(f"\n===== {label} =====", flush=True)
        if frame:
            s.write((frame + "\n").encode())
        end = time.time() + secs
        while time.time() < end:
            l = s.readline()
            if l: print(l.decode('utf-8','replace').rstrip(), flush=True)
