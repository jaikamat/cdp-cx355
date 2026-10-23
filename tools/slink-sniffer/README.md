# S-Link Bus Sniffer

A standalone firmware for protocol research. It replaces the product firmware on the board while in use; flash the main project again afterwards.

What it does:
- A pin-change interrupt on D8 timestamps every edge on the S-Link line.
- `loop()` decodes LOW pulse widths into frames and prints them with a millisecond timestamp, e.g. `[12345] 98 50 CA 03 03 48`.
- A line of hex bytes typed over serial (`90 00`) is transmitted on the bus, so you can send a command and watch the player's reply and any later broadcasts.
- Every 5 seconds it prints the edge count, which proves capture is alive even when the bus is quiet.

## Usage

```bash
cd tools/slink-sniffer
pio run --target upload
pio device monitor          # then type: 90 00
```

`experiment.py` drives a scripted sequence (idle, play, next, pause, stop, status queries, disc change, power cycle) and prints everything captured:

```bash
python3 experiment.py /dev/cu.usbmodemXXXX
```

## Findings so far

See the "Status Frames the Player Broadcasts" section in the top-level README. Key lesson for the product firmware: on the RA4M1, `pinMode()` rewrites the pin function register and drops the edge interrupt, so detach before transmitting and re-attach after.
