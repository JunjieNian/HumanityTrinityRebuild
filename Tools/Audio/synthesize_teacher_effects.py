"""Deterministic original shoes/keys and a subtle corridor tail for offline speech."""
from pathlib import Path
import array
import json
import math
import random
import wave

OUT = Path(__file__).resolve().parents[2] / 'Assets' / 'Audio' / 'Teacher'
RATE = 22050
OUT.mkdir(parents=True, exist_ok=True)
random.seed(2601006)


def save(name, samples):
    peak = max(max(map(abs, samples), default=0), 1e-8)
    gain = min(.86 / peak, 1)
    data = array.array('h', (round(max(-1, min(1, v * gain)) * 32767) for v in samples))
    with wave.open(str(OUT / (name + '.wav')), 'wb') as stream:
        stream.setparams((1, 2, RATE, 0, 'NONE', 'not compressed'))
        stream.writeframes(data.tobytes())


shoe = []
for i in range(round(RATE * .36)):
    t = i / RATE
    heel = math.sin(2 * math.pi * 118 * t) * math.exp(-t * 42) * .5
    click = random.uniform(-1, 1) * math.exp(-t * 115) * .35
    toe_t = max(0, t - .055)
    toe = (math.sin(2 * math.pi * 195 * toe_t) + random.uniform(-.4, .4)) * math.exp(-toe_t * 60) * .21 if t >= .055 else 0
    shoe.append(heel + click + toe)
save('SW_TeacherFootstep', shoe)

keys = [0.] * round(RATE * .65)
for offset, strength in ((.0, .16), (.035, .24), (.084, .12), (.13, .09)):
    for i in range(round(RATE * offset), len(keys)):
        t = i / RATE - offset
        keys[i] += strength * sum(math.sin(2 * math.pi * f * t) for f in (1830, 2677, 3911)) / 3 * math.exp(-t * 18)
save('SW_TeacherKeys', keys)

for dry in sorted(OUT.glob('*_dry.wav')):
    with wave.open(str(dry), 'rb') as stream:
        assert stream.getframerate() == RATE and stream.getnchannels() == 1 and stream.getsampwidth() == 2
        samples = [v / 32768 for v in array.array('h', stream.readframes(stream.getnframes()))]
    wet = samples + [0.] * round(.3 * RATE)
    for delay, gain in ((.067, .11), (.117, .075), (.191, .035)):
        at = round(delay * RATE)
        for i, v in enumerate(samples):
            wet[i + at] += v * gain
    save(dry.stem.removesuffix('_dry'), wet)

report = []
for path in sorted(OUT.glob('SW_*.wav')):
    if path.stem.endswith('_dry'):
        continue
    with wave.open(str(path), 'rb') as stream:
        report.append({'file': path.name, 'duration_seconds': round(stream.getnframes()/stream.getframerate(), 3), 'rate': stream.getframerate(), 'channels': stream.getnchannels()})
(OUT / 'audio_manifest.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
print(json.dumps(report, indent=2))
