#!/usr/bin/env python3
"""Reject mislabeled renderers and unequal repeats before accepting frame evidence."""
from pathlib import Path
from run import SCRATCH
import tempfile
import json
import frames

# This check exercises evidence validation; golden comparison has its own unit oracle.
frames.compare = lambda *args, **kwargs: None
with tempfile.TemporaryDirectory() as temporary:
    output = Path(temporary)
    for map_name in ('q3dm17', 'q3dm7'):
        for backend in ('vulkan',):
            log = 'VK_RENDERER: llvmpipe\nDriver: 1.2.3'
            for iteration in (1, 2):
                (output / f'{map_name}-{backend}-replay-{iteration}.log').write_text(log)
                for name in ('frame050', 'frame100', 'frame200'):
                    (output / f'{map_name}-{backend}-{iteration}-{name}.tga').write_text(name)
    frames.check_frames(output, 'quake3', False)
    for name, replacement in [
        ('q3dm17-vulkan-replay-1.log', 'GL_RENDERER: llvmpipe\nGL_VERSION: Mesa 1.2.3'),
        ('q3dm17-vulkan-2-frame100.tga', 'changed'),
    ]:
        path = output / name
        original = path.read_text()
        path.write_text(replacement)
        try:
            frames.check_frames(output, 'quake3', False)
        except SystemExit:
            pass
        else:
            raise AssertionError('invalid evidence accepted: ' + name)
        path.write_text(original)
expected = {'q3dm17/vulkan': {'frame050': 'accepted'}, 'q3dm17/opengl1': {'frame050': 'archived'}}
actual = {'q3dm17/vulkan': {'frame050': 'accepted'}}
assert frames.active_frames(json.dumps(expected)) == frames.active_frames(json.dumps(actual))
actual['q3dm17/vulkan']['frame050'] = 'changed'
assert frames.active_frames(json.dumps(expected)) != frames.active_frames(json.dumps(actual))
assert frames.active_frames(json.dumps(expected)) != frames.active_frames('{}')
print('PASS: wrong renderer, unequal repeats, missing/changed Vulkan goldens rejected; retired OpenGL rows retained')
