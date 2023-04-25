#!/usr/bin/env python3

import serial
import sys
import time


def serial_getter():
    # grab fresh ADC values
    # note sometimes UART drops chars so we try a max of 5 times
    # to get proper data
    for i in range(5):
        line = ser.readline()
        try:
            line = int(line)
        except ValueError:
            continue
        break
    yield line

def fast_getter():
    yield ser.readline()

if len(sys.argv) < 2:
    raise Exception("Usage: sniff_usb.py PORT (such as /dev/ttyS5)")

ser = serial.Serial(sys.argv[1], 115200, timeout=1)


start = time.time()
i = 0

while True:

    try:
        line = int(ser.readline())
    except ValueError:
        continue

    i += 1
    print(line)

    if i == 8000:
        end = time.time()
        print("time to get 8000:", end - start, file=sys.stderr)
        break
