"""Content-addressed downloads and native material cooking from a reviewed lock."""
import hashlib
import io
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
from urllib.parse import urlparse
import urllib.request
import zipfile

from PIL import Image

from tools.assets.manifest import digest,require,validate
from tools.scratch import cache_lock

ROOT = Path(__file__).resolve().parents[2]
USER_AGENT = 'AftershockAssetTools/1.0 (msetaro/aftershock)'
LIMIT = 128*1024*1024


def request(url):
    require(urlparse(url).scheme=='https', 'asset downloads require HTTPS')
    with urllib.request.urlopen(urllib.request.Request(url,headers={'User-Agent':USER_AGENT}),timeout=60) as response:
        data = response.read(LIMIT+1)
    require(len(data)<=LIMIT, 'asset download exceeds 128 MiB')
    return data


def cached(record,cache,offline):
    sha = record['sha256']
    require(re.fullmatch('[0-9a-f]{64}',sha), 'lock needs a SHA256 content pin')
    cache.mkdir(parents=True,exist_ok=True)
    target = cache/sha
    with cache_lock(cache):
        if target.exists():
            require(digest(target)==sha, 'cached asset SHA256 mismatch: '+sha)
            return target.read_bytes()
    require(not offline, 'pinned asset absent from offline cache: '+sha)
    if 'member' in record:
        data = cached(dict(url=record['url'],sha256=record['archive_sha256']),cache,offline)
        with zipfile.ZipFile(io.BytesIO(data)) as archive:
            info = archive.getinfo(record['member'])
            require(info.file_size<=LIMIT, 'asset ZIP member exceeds limit')
            data = archive.read(info)
    else:
        data = request(record['url'])
    require(hashlib.sha256(data).hexdigest()==sha, 'downloaded asset SHA256 differs from lock')
    require('bytes' not in record or len(data)==record['bytes'], 'downloaded asset size differs from lock')
    with cache_lock(cache):
        with tempfile.NamedTemporaryFile(dir=cache,delete=False) as stream:
            temporary = Path(stream.name)
            try:
                stream.write(data)
                stream.flush()
                os.replace(temporary,target)
            finally:
                temporary.unlink(missing_ok=True)
    return data


def fetch(lock_path,out,offline=False):
    lock = json.loads(lock_path.read_text())
    require(lock.get('version')==1 and isinstance(lock.get('assets'),list) and 1<=len(lock['assets'])<=64, 'invalid asset lock')
    resolution = lock['resolution']
    require(type(resolution) is int and 8<=resolution<=4096 and not resolution&(resolution-1), 'texture budget must be a power of two in 8..4096')
    cache = Path(os.environ.get('XDG_CACHE_HOME',Path.home()/'.cache'))/'aftershock-assets'
    out = out.resolve()
    out.parent.mkdir(parents=True,exist_ok=True)
    if out.exists():
        validate(out)
        require(json.loads((out/'assets.lock.json').read_text())==lock, 'output uses another lock; choose a fresh output directory')
        return dict(ok=True,output=str(out),reused=True,**{k:v for k,v in validate(out).items() if k!='ok'})
    sys.path.insert(0,str(ROOT/'tools/cook'))
    from texture import mipmaps
    with tempfile.TemporaryDirectory(prefix='aftershock-theme-',dir=out.parent) as temporary:
        stage = Path(temporary)/'kit'
        stage.mkdir()
        entries,recipes,roles = [],[],set()
        for asset in lock['assets']:
            require(asset.get('provider') in ('polyhaven','ambientcg'), 'provider has no explicit license filter')
            require(asset.get('license')=='CC0-1.0', 'provider asset license is outside CC0 allowlist')
            require(bool(asset.get('version')), 'provider asset version must be pinned')
            role = asset['role']
            require(re.fullmatch('[a-z][a-z0-9_]{0,23}',role) and role not in roles, 'theme roles must be unique lowercase names')
            roles.add(role)
            source = stage/'source'/role
            source.mkdir(parents=True)
            channels,originals = {},[]
            for channel,record in sorted(asset['files'].items()):
                require(channel in ('color','normal','roughness','metallic','ao','displacement','emission'), 'unknown PBR channel')
                data = cached(record,cache,offline)
                suffix = Path(record.get('member',urlparse(record['url']).path)).suffix.lower()
                require(suffix in ('.png','.jpg','.jpeg','.tga'), 'texture source must be PNG/JPEG/TGA')
                original = source/(channel+'_original'+suffix)
                original.write_bytes(data)
                originals.append(dict(path=original.relative_to(stage).as_posix(),sha256=digest(original)))
                with Image.open(io.BytesIO(data)) as image:
                    require(image.width*image.height<=64*1024*1024, 'source texture exceeds pixel budget')
                    pixels = image.convert('RGBA')
                if max(pixels.size)>resolution:
                    pixels = next(p for p in mipmaps(pixels,channel in ('color','emission'),channel=='normal') if max(p.size)<=resolution)
                channels[channel] = pixels
                pixels.save(source/(channel+'.png'))
            require({'color','normal','roughness'}<=channels.keys(), 'material needs color, normal and roughness sources')
            size = channels['color'].size
            require(all(p.size==size for p in channels.values()), 'PBR channel dimensions differ after budget reduction')
            zero = Image.new('L',size,0)
            metal = channels.get('metallic',Image.new('RGBA',size,(0,0,0,255))).getchannel('R')
            Image.merge('RGB',(zero,channels['roughness'].getchannel('R'),metal)).save(source/'metallic_roughness.png')
            material = dict(pbrMetallicRoughness=dict(baseColorTexture=dict(uri='color.png'),
                            metallicRoughnessTexture=dict(uri='metallic_roughness.png'),metallicFactor=1,roughnessFactor=1),
                            normalTexture=dict(uri='normal.png'),alphaMode='OPAQUE')
            if 'emission' in channels:
                material.update(emissiveTexture=dict(uri='emission.png'),emissiveFactor=[1,1,1])
            (source/'material.json').write_text(json.dumps(material,sort_keys=True)+'\n')
            recipes.append(dict(name='textures/theme/'+role,kind='material',source='source/'+role+'/material.json',material_model='metallic-roughness'))
            entry = {key:asset[key] for key in ('id','provider','source_url','author','license','retrieved','attribution')}
            entry.update(id=asset['provider']+'/'+asset['id'],version=asset['version'],originals=originals,cooked=[])
            entries.append(entry)
        (stage/'assets.json').write_text(json.dumps(dict(version=1,assets=recipes),indent=2,sort_keys=True)+'\n')
        result = subprocess.run([sys.executable,str(ROOT/'tools/cook'),str(stage/'assets.json'),'--output',str(stage/'cooked')],
                                capture_output=True,text=True)
        require(result.returncode==0, 'theme cooker failed: '+result.stderr[-8000:])
        for asset,entry in zip(lock['assets'],entries):
            role = asset['role']
            originals = {f['path'] for f in entry['originals']}
            files = [p for p in (stage/'source'/role).iterdir() if p.relative_to(stage).as_posix() not in originals]
            files += list((stage/'cooked/textures/theme').glob(role+'.*'))+list((stage/'cooked/textures/theme').glob(role+'_*.ktx2'))
            entry['cooked'] = [dict(path=p.relative_to(stage).as_posix(),sha256=digest(p)) for p in sorted(files)]
        # The shared cooker index records every role; attribute its generated metadata
        # to the same locked kit rather than leave unlisted runtime files behind.
        for path in (stage/'cooked').iterdir():
            if path.is_file():
                entries[0]['cooked'].append(dict(path=path.relative_to(stage).as_posix(),sha256=digest(path)))
        (stage/'assets.lock.json').write_text(json.dumps(lock,indent=2,sort_keys=True)+'\n')
        (stage/'manifest.json').write_text(json.dumps(dict(version=1,assets=entries),indent=2,sort_keys=True)+'\n')
        report = validate(stage)
        os.replace(stage,out)
    return dict(report,output=str(out),credits=str(out/'CREDITS'),reused=False)
