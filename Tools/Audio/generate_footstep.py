"""Generate the small, original mono footstep used by the hide-and-seek prototype."""

from pathlib import Path
import math
import random
import struct
import wave

ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "Assets" / "Audio" / "hider_footstep.wav"
RATE = 24000
DURATION = 0.24


def main():
    randomizer = random.Random(20260922)
    samples = []
    low_noise = 0.0
    for index in range(int(RATE * DURATION)):
        t = index / RATE
        low_noise = 0.87 * low_noise + 0.13 * randomizer.uniform(-1.0, 1.0)
        attack = min(1.0, t / 0.008)
        envelope = attack * math.exp(-22.0 * t)
        sole = 0.34 * low_noise + 0.20 * math.sin(2.0 * math.pi * (92.0 - 42.0 * t) * t)
        samples.append(sole * envelope)

    peak = max(abs(value) for value in samples)
    pcm = [int(max(-1.0, min(1.0, value * 0.75 / peak)) * 32767)
           for value in samples]

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(OUTPUT), "wb") as audio:
        audio.setnchannels(1)
        audio.setsampwidth(2)
        audio.setframerate(RATE)
        audio.writeframes(struct.pack(f"<{len(pcm)}h", *pcm))
    print(OUTPUT)


if __name__ == "__main__":
    main()
