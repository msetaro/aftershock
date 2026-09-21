"""Pinned portable Blender, installed only in a user cache."""
import hashlib
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import tarfile
import tempfile
import urllib.request

from tools.scratch import cache_lock

VERSION='5.0.1'
SHA256='8019580ee1b7262e505f4196a00237ccf743c88d205b38d34201510676e60b09'
ARCHIVE=f'blender-{VERSION}-linux-x64.tar.xz'
URL='https://ftp.halifax.rwth-aachen.de/blender/release/Blender5.0/'+ARCHIVE


def blender(binary=None):
    if binary is None:
        if platform.system()!='Linux' or platform.machine() not in ('x86_64','AMD64'):
            raise ValueError('pass a pinned Blender 5.0.1 binary on this platform')
        cache=Path(os.environ.get('XDG_CACHE_HOME',Path.home()/'.cache'))/'aftershock-modernization'/('blender-'+VERSION)
        with cache_lock(cache):
            cache.mkdir(parents=True,exist_ok=True)
            archive=cache/ARCHIVE
            if not archive.exists():
                with tempfile.NamedTemporaryFile(dir=cache,delete=False) as stream:
                    temporary=Path(stream.name)
                    try:
                        with urllib.request.urlopen(URL,timeout=120) as response:
                            shutil.copyfileobj(response,stream)
                        stream.flush()
                        with temporary.open('rb') as source:
                            if hashlib.file_digest(source,'sha256').hexdigest()!=SHA256:
                                raise ValueError('Blender archive SHA256 mismatch')
                        os.replace(temporary,archive)
                    finally:
                        temporary.unlink(missing_ok=True)
            with archive.open('rb') as source:
                if hashlib.file_digest(source,'sha256').hexdigest()!=SHA256:
                    raise ValueError('Blender archive SHA256 mismatch')
            binary=cache/('blender-'+VERSION+'-linux-x64')/'blender'
            if not binary.is_file():
                with tarfile.open(archive) as package:
                    package.extractall(cache,filter='data')
    binary=Path(binary).resolve()
    version=subprocess.check_output([str(binary),'--version'],text=True,timeout=30)
    if not re.match(r'Blender 5\.0\.1\s',version):
        raise ValueError('Blender 5.0.1 required')
    return binary
