#!/usr/bin/env python3

import serial
import sys



def serial_getter():
    # grab fresh ADC values
    # note sometimes UART drops chars so we try a max of 5 times
    # to get proper data
    for i in range(5):
        line = ser.readline()
        try:
            line = float(line)
        except ValueError:
            continue
        break
    yield line

if len(sys.argv) < 2:
    raise Exception("Usage: sniff_usb.py PORT (such as /dev/ttyS5)")

ser = serial.Serial(sys.argv[1], 115200, timeout=1)


while True:
    for line in serial_getter():
        print(line)
