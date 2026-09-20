#!/usr/bin/env python3
"""Explicitly author the owned 50 ms tone fixture; CI only reads committed bytes."""
import ctypes as C
import ctypes.util
import hashlib
import json
import math
from pathlib import Path
import struct
import wave

ROOT = Path(__file__).resolve().parent
samples = [round(8192 * math.sin(2 * math.pi * 440 * i / 22050)) for i in range(1102)]
with wave.open(str(ROOT / 'tone.wav'), 'wb') as output:
    output.setparams((1, 2, 22050, len(samples), 'NONE', 'not compressed'))
    output.writeframes(struct.pack('<' + 'h' * len(samples), *samples))

class Info(C.Structure):
    _fields_ = [('frames', C.c_int64), ('rate', C.c_int), ('channels', C.c_int),
                ('format', C.c_int), ('sections', C.c_int), ('seekable', C.c_int)]

library = C.CDLL(ctypes.util.find_library('sndfile'))
library.sf_version_string.restype = C.c_char_p
library.sf_open.argtypes = [C.c_char_p, C.c_int, C.POINTER(Info)]
library.sf_open.restype = C.c_void_p
library.sf_writef_short.argtypes = [C.c_void_p, C.POINTER(C.c_int16), C.c_int64]
library.sf_writef_short.restype = C.c_int64
library.sf_close.argtypes = [C.c_void_p]
info = Info(0, 22050, 1, 0x200000 | 0x0060, 0, 0)
handle = library.sf_open(str(ROOT / 'tone.ogg').encode(), 0x20, C.byref(info))
assert handle
try:
    assert library.sf_writef_short(handle, (C.c_int16 * len(samples))(*samples), len(samples)) == len(samples)
finally:
    assert library.sf_close(handle) == 0
(ROOT / 'provenance.json').write_text(json.dumps({
    'generator': library.sf_version_string().decode(), 'frames': len(samples),
    'sample_rate': 22050, 'channels': 1,
    'files': {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest()
              for name in ('export.py', 'tone.wav', 'tone.ogg')}}, indent=2) + '\n')
