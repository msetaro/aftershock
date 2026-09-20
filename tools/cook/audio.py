"""Normalize source audio to the engine's PCM16 WAV container."""
import hashlib
import io
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import wave

import texture


def cook(raw, extension):
    if extension == '.wav':
        with wave.open(io.BytesIO(raw)) as source:
            channels, width, rate, frames, compression, _ = source.getparams()
            if compression != 'NONE' or channels not in (1, 2) or width not in (1, 2, 3, 4) or not 8000 <= rate <= 192000 or not 0 < frames * channels * 2 <= 256 << 20:
                raise ValueError('audio requires mono/stereo PCM at 8–192 kHz, at most 256 MiB decoded')
            data = source.readframes(frames)
            if len(data) != frames * channels * width:
                raise ValueError('incomplete source PCM data')
            if width != 2:
                samples = ((data[i] - 128) << 8 if width == 1 else
                           int.from_bytes(data[i:i + width], 'little', signed=True) >> (8 * (width - 2))
                           for i in range(0, len(data), width))
                data = b''.join(struct.pack('<h', sample) for sample in samples)
    elif extension == '.ogg':
        decoder = texture.encoder().with_name('aftershock-cook-audio' + ('.exe' if os.name == 'nt' else ''))
        with tempfile.TemporaryDirectory(prefix='aftershock-audio-') as temporary:
            source, output = Path(temporary) / 'source.ogg', Path(temporary) / 'samples.pcm'
            source.write_bytes(raw)
            result = subprocess.run([str(decoder), str(source), str(output)], capture_output=True, text=True)
            if result.returncode:
                raise ValueError('Vorbis source requires mono/stereo audio at 8–192 kHz, at most 256 MiB decoded')
            rate, channels = map(int, result.stdout.split())
            data = output.read_bytes()
    else:
        raise ValueError('audio source must be .wav or .ogg')
    fmt = struct.pack('<4sIHHIIHH', b'fmt ', 16, 1, channels, rate, rate * channels * 2, channels * 2, 16)
    stamp = struct.pack('<4sII32s32s', b'ASCK', 68, 1, hashlib.sha256(raw).digest(), bytes(32))
    payload = fmt + stamp + struct.pack('<4sI', b'data', len(data)) + data
    output = bytearray(struct.pack('<4sI4s', b'RIFF', len(payload) + 4, b'WAVE') + payload)
    output[80:112] = hashlib.sha256(output).digest()
    return bytes(output)
