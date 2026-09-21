"""Explicit CC0 API adapters; discovery writes reviewed locks, never substitutes assets."""
import datetime
import hashlib
import io
import json
import re
from urllib.parse import urlencode
import zipfile

from tools.assets.fetch import request,LIMIT
from tools.assets.manifest import require
from tools.scratch import cache_lock

LICENSE_URLS = {'polyhaven':'https://docs.polyhaven.com/en/faq','ambientcg':'https://docs.ambientcg.com/license/'}


def metadata(url):
    return json.loads(request(url))


def provider_check(provider):
    require(provider in LICENSE_URLS, 'provider has no explicit machine-readable license policy')


def search(provider,tag,limit=10):
    provider_check(provider)
    require(1<=limit<=100 and isinstance(tag,str) and 1<=len(tag)<=100, 'search tag/limit outside bounds')
    if provider=='polyhaven':
        assets = metadata('https://api.polyhaven.com/assets?t=textures')
        found = [dict(id=identity,name=a['name'],provider=provider,license='CC0-1.0')
                 for identity,a in assets.items() if tag.casefold() in ' '.join([identity,a['name'],*a.get('tags',[]),*a.get('categories',[])]).casefold()]
    else:
        response = metadata('https://ambientcg.com/api/v2/full_json?'+urlencode(dict(q=tag,type='Material',limit=limit,sort='Alphabet')))
        found = [dict(id=a['assetId'],name=a.get('displayName',a['assetId']),provider=provider,license='CC0-1.0') for a in response['foundAssets']]
    return sorted(found,key=lambda a:a['id'])[:limit]


def store(data,cache):
    require(len(data)<=LIMIT, 'downloaded asset exceeds byte budget')
    sha = hashlib.sha256(data).hexdigest()
    cache.mkdir(parents=True,exist_ok=True)
    with cache_lock(cache):
        path = cache/sha
        if path.exists():
            require(hashlib.sha256(path.read_bytes()).hexdigest()==sha, 'cached asset SHA256 mismatch')
        else:
            path.write_bytes(data)
    return sha


def pin(provider,identity,role,resolution,cache):
    provider_check(provider)
    require(re.fullmatch('[A-Za-z0-9_-]{1,96}',identity) and re.fullmatch('[a-z][a-z0-9_]{0,23}',role), 'invalid asset ID or role')
    files = {}
    if provider=='polyhaven':
        info = metadata('https://api.polyhaven.com/info/'+identity)
        catalog = metadata('https://api.polyhaven.com/files/'+identity)
        mappings = {'color':['Diffuse','diff','BaseColor'],'normal':['nor_gl'],'roughness':['Rough'],
                    'ao':['AO'],'displacement':['Displacement'],'metallic':['Metal','Metallic','Metalness'],'emission':['Emission']}
        for channel,keys in mappings.items():
            key = next((key for key in keys if key in catalog),None)
            if key is None:
                continue
            sizes = sorted((int(label[:-1])*1024,label) for label in catalog[key] if re.fullmatch('[0-9]+k',label))
            require(sizes, 'provider has no usable texture resolution: '+channel)
            selected = max((s for s in sizes if s[0]<=resolution),default=sizes[0])[1]
            formats = catalog[key][selected]
            record = formats.get('png',formats.get('jpg'))
            require(record is not None, 'provider has no PNG/JPEG channel: '+channel)
            data = request(record['url'])
            require(len(data)==record['size'] and hashlib.md5(data).hexdigest()==record['md5'], 'provider download differs from its size/MD5 metadata')
            files[channel] = dict(url=record['url'],sha256=store(data,cache),bytes=len(data))
        version = info.get('files_hash') or hashlib.sha256(json.dumps(catalog,sort_keys=True).encode()).hexdigest()
        author = ', '.join(sorted(info['authors']))
        url = 'https://polyhaven.com/a/'+identity
    else:
        info = metadata('https://ambientcg.com/api/v2/full_json?'+urlencode(dict(id=identity,include='downloadData')))
        assets = info['foundAssets']
        require(len(assets)==1 and assets[0]['assetId']==identity, 'ambientCG asset ID was not returned exactly')
        info = assets[0]
        downloads = info['downloadFolders']['default']['downloadFiletypeCategories']['zip']['downloads']
        sizes = [(int(m[1])*1024,d) for d in downloads if (m:=re.fullmatch(r'(\d+)K-(PNG|JPG)',d['attribute']))]
        require(sizes, 'ambientCG has no usable PNG/JPEG texture ZIP')
        budget = max((size for size,_ in sizes if size<=resolution),default=min(size for size,_ in sizes))
        archive_record = sorted((d for size,d in sizes if size==budget),key=lambda d:('PNG' not in d['attribute'],d['attribute']))[0]
        data = request(archive_record['downloadLink'])
        archive_hash = store(data,cache)
        with zipfile.ZipFile(io.BytesIO(data)) as archive:
            require(len(archive.infolist())<=128, 'provider ZIP exceeds member budget')
            mappings = {'Color':'color','NormalGL':'normal','Roughness':'roughness','AmbientOcclusion':'ao',
                        'Displacement':'displacement','Metalness':'metallic','Metallic':'metallic','Emission':'emission'}
            for member in archive.infolist():
                match = re.search(r'_('+('|'.join(mappings))+r')\.(png|jpg|jpeg)$',member.filename)
                if not match:
                    continue
                require(member.file_size<=LIMIT, 'provider ZIP member exceeds byte budget')
                content = archive.read(member)
                channel = mappings[match[1]]
                require(channel not in files, 'provider ZIP has duplicate PBR channels')
                files[channel] = dict(url=archive_record['downloadLink'],archive_sha256=archive_hash,member=member.filename,
                                      sha256=store(content,cache),bytes=len(content))
        version = info['releaseDate']+'/'+archive_hash
        author = 'ambientCG'
        url = 'https://ambientcg.com/a/'+identity
    require({'color','normal','roughness'}<=files.keys(), 'provider asset lacks the required full PBR channel set')
    return dict(provider=provider,id=identity,role=role,version=version,source_url=url,author=author,
                license='CC0-1.0',license_url=LICENSE_URLS[provider],retrieved=datetime.date.today().isoformat(),
                attribution=f'{identity} by {author}, CC0-1.0',files=files)
