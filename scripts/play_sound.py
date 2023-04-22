#!/usr/bin/env python3
import numpy as np
import sounddevice as sd
import sys

if len(sys.argv) < 2:
    print("Usage: play_sound.py FILE")
    sys.exit(1)

with open(sys.argv[1]) as f:
    data = [float(line.strip()) for line in f]

samplerate = 4000
sound = np.array(data)
sound -= np.mean(sound)
sound /=  np.max(np.abs(sound))

print(sound)

# Play the data as sound
sd.play(sound, samplerate=samplerate)
sd.wait()
