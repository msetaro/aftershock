"""Pinned offline map tools, fetched into the user's cache, never the source tree."""
import hashlib
import os
from pathlib import Path
import platform
import shutil
import struct
import subprocess
import tempfile
import urllib.request

VERSION = '20260114'
ARCHIVE = f'netradiant-custom-{VERSION}-linux-x86_64.7z'
SHA256 = 'f48f6f1d0db2b910ef9cb5dc5d8a722852510f3c5c278dc17615c0466b8a7a3d'
URL = f'https://github.com/Garux/netradiant-custom/releases/download/{VERSION}/{ARCHIVE}'


def tools():
    if platform.system()!='Linux' or platform.machine() not in ('x86_64','AMD64'):
        raise ValueError('pinned BSP/AAS toolchain requires Linux x86_64; --map-only is portable')
    cache = Path(os.environ.get('XDG_CACHE_HOME',Path.home()/'.cache'))/'aftershock-level-tools'
    cache.mkdir(parents=True,exist_ok=True)
    archive = cache/ARCHIVE
    if not archive.is_file():
        with tempfile.NamedTemporaryFile(dir=cache,delete=False) as stream:
            temporary = Path(stream.name)
            try:
                with urllib.request.urlopen(URL,timeout=120) as response:
                    shutil.copyfileobj(response,stream)
                stream.flush()
                if hashlib.sha256(temporary.read_bytes()).hexdigest()!=SHA256:
                    raise ValueError('map tool archive SHA256 mismatch')
                os.replace(temporary,archive)
            finally:
                temporary.unlink(missing_ok=True)
    if hashlib.sha256(archive.read_bytes()).hexdigest()!=SHA256:
        raise ValueError('map tool archive SHA256 mismatch: '+str(archive))
    root = cache/'squashfs-root'
    if not (root/'usr/bin/q3map2.x86_64').is_file() or not (root/'usr/bin/mbspc.x86_64').is_file():
        try:
            import libarchive
        except ImportError as exc:
            raise ValueError('install tools/level/requirements.txt into a Python venv to extract the pinned toolchain') from exc
        image = cache/'NetRadiant-Custom-x86_64.AppImage'
        found = False
        with libarchive.file_reader(str(archive)) as entries:
            for entry in entries:
                if entry.pathname==image.name:
                    with image.open('wb') as stream:
                        for block in entry.get_blocks():
                            stream.write(block)
                    found = True
        if not found:
            raise ValueError('pinned tool archive has no AppImage')
        image.chmod(0o755)
        with (cache/'extract.log').open('w') as log:
            subprocess.run([str(image),'--appimage-extract'],cwd=cache,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=120)
    env = dict(os.environ,LD_LIBRARY_PATH=str(root/'usr/lib'),LC_ALL='C',TZ='UTC')
    return root/'usr/bin/q3map2.x86_64',root/'usr/bin/mbspc.x86_64',env


def canonical_padding(path):
    """Zero uninitialized q3map2 alignment bytes; preserve every declared lump byte."""
    data = bytearray(path.read_bytes())
    if len(data)<144 or struct.unpack_from('<4sI',data)!=(b'IBSP',46):
        raise ValueError('map compiler did not produce IBSP 46')
    used = bytearray(len(data))
    used[:144] = b'\1'*144
    for index in range(17):
        offset,length = struct.unpack_from('<ii',data,8+index*8)
        if offset<144 or length<0 or offset+length>len(data) or any(used[offset:offset+length]):
            raise ValueError('map compiler produced invalid/overlapping BSP lumps')
        used[offset:offset+length] = b'\1'*length
    for i,occupied in enumerate(used):
        if not occupied:
            data[i] = 0
    path.write_bytes(data)


def compile_map(output,name):
    q3map2,bspc,env = tools()
    # Stable relative inputs also keep q3map2 command-line metadata machine independent.
    with tempfile.TemporaryDirectory(prefix='aftershock-map-') as temporary:
        work = Path(temporary)
        shutil.copytree(output,work/'baseq3')
        relative = 'baseq3/maps/'+name
        common = [str(q3map2),'-game','quake3','-fs_basepath','.', '-fs_homepath','./home','-threads','1']
        with (output/'compile.log').open('w') as log:
            for stage in (['-meta','-leaktest',relative+'.map'],['-vis',relative+'.bsp'],['-light','-fast',relative+'.bsp']):
                subprocess.run(common+stage,cwd=work,env=env,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=300)
            bsp = work/(relative+'.bsp')
            canonical_padding(bsp)
            subprocess.run([str(bspc),'-bsp2aas',relative+'.bsp','-threads','1','-forcesidesvisible'],
                           cwd=work,env=env,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=300)
        aas = work/(relative+'.aas')
        if not aas.is_file() or struct.unpack_from('<4sI',aas.read_bytes())!=(b'EAAS',5):
            raise ValueError('BSPC did not produce EAAS 5; see compile.log')
        for extension in ('.bsp','.aas'):
            shutil.copyfile(work/(relative+extension),output/'maps'/(name+extension))
