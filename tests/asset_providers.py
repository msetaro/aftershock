#!/usr/bin/env python3
"""Provider adapters filter licenses, normalize API metadata and pin exact bytes."""
import hashlib
import io
import json
from pathlib import Path
import sys
import tempfile
import zipfile

from run import ROOT
sys.path.insert(0,str(ROOT))
from tools.assets import providers

payload=b'owned download bytes'
md5=hashlib.md5(payload).hexdigest()
with tempfile.TemporaryDirectory(prefix='aftershock-provider-') as temporary:
    archive=io.BytesIO()
    with zipfile.ZipFile(archive,'w') as out:
        for channel in ('Color','NormalGL','Roughness','AmbientOcclusion','Displacement'):
            out.writestr('Owned_1K_'+channel+'.png',payload+channel.encode())
    responses={
        'https://api.polyhaven.com/assets?t=textures':{'bricks':{'name':'Brick','tags':['wall'],'categories':['brick']},'wood':{'name':'Wood','tags':['wood'],'categories':[]}},
        'https://api.polyhaven.com/info/bricks':{'authors':{'Owned Artist':'Capture'},'files_hash':'version-a'},
        'https://api.polyhaven.com/files/bricks':{key:{'1k':{'png':{'url':'https://dl.polyhaven.org/'+key+'.png','md5':md5,'size':len(payload)}}} for key in ('Diffuse','nor_gl','Rough','AO','Displacement')},
        'https://ambientcg.com/api/v2/full_json?id=Owned&include=downloadData':{'foundAssets':[{'assetId':'Owned','releaseDate':'2026-01-01','downloadFolders':{'default':{'downloadFiletypeCategories':{'zip':{'downloads':[{'attribute':'1K-PNG','downloadLink':'https://ambientcg.com/owned.zip'}]}}}}}]},
        'https://ambientcg.com/owned.zip':archive.getvalue(),
    }
    def request(url):
        value=responses[url] if url in responses else payload
        return json.dumps(value).encode() if isinstance(value,dict) else value
    providers.request=request
    result=providers.search('polyhaven','brick',10)
    assert [v['id'] for v in result]==['bricks']
    for provider,identity in [('polyhaven','bricks'),('ambientcg','Owned')]:
        asset=providers.pin(provider,identity,'facade',1024,Path(temporary))
        assert asset['license']=='CC0-1.0' and asset['author'] and asset['version']
        assert {'color','normal','roughness','ao','displacement'}<=asset['files'].keys()
        assert all(len(f['sha256'])==64 for f in asset['files'].values())
        assert asset['license_url'].startswith('https://')
    try:
        providers.search('unlicensed-site','brick',10)
    except ValueError as exc:
        assert 'provider' in str(exc)
    else:
        raise AssertionError('unknown provider accepted without a license filter')
print('PASS: API tag search, complete channel normalization, byte pins and explicit provider license filters')
