"""License/provenance checks for complete theme kits, independent of providers."""
import datetime
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
from urllib.parse import urlparse

POLICY = Path(__file__).with_name('policy.json')
METADATA = {'manifest.json','CREDITS','assets.lock.json','assets.json'}


def require(condition,message):
    if not condition:
        raise ValueError(message)


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()


def validate(root, *, publication=True):
    root = root.resolve()
    document = json.loads((root/'manifest.json').read_text())
    policy = json.loads(POLICY.read_text())
    require(document.get('version')==1 and isinstance(document.get('assets'),list), 'manifest version/assets are required')
    seen,ids,credits = set(),set(),[]
    publishable = True
    for asset in document['assets']:
        for field in ('id','source_url','author','license','retrieved','attribution'):
            require(isinstance(asset.get(field),str) and (asset[field] or field=='attribution'), 'missing '+field)
        require(asset['id'] not in ids, 'duplicate asset id')
        ids.add(asset['id'])
        require(urlparse(asset['source_url']).scheme=='https' and bool(urlparse(asset['source_url']).netloc), 'source_url must be an HTTPS provenance URL')
        allowed = asset['license'] in policy['licenses']
        publishable = publishable and allowed
        require(allowed or not publication, 'license is outside the maintainer allowlist: '+asset['license'])
        datetime.date.fromisoformat(asset['retrieved'])
        if asset.get('generator'):
            generator = asset['generator']
            if generator.get('name') in policy['procedural_tools']:
                require(generator.get('version')==policy['procedural_tools'][generator['name']], 'procedural generator version differs from policy')
                require(all(re.fullmatch('[0-9a-f]{64}',generator.get(key,'')) for key in ('script_sha256','parameters_sha256')),
                        'procedural generator needs script and parameters SHA256')
            else:
                require(generator.get('name') in policy['image_generators'] and bool(generator.get('prompt')), 'image generator is outside the maintainer allowlist or missing its prompt')
        for kind in ('originals','cooked'):
            require(isinstance(asset.get(kind),list) and asset[kind], 'asset requires '+kind+' SHA256 records')
            for file in asset[kind]:
                name = file['path']
                path = PurePosixPath(name)
                require(isinstance(name,str) and name and not path.is_absolute() and '..' not in path.parts and '\\' not in name,
                        'manifest file must be a relative asset path')
                target = root/name
                require(target.is_file() and not target.is_symlink() and target.resolve().is_relative_to(root), 'missing file or asset outside kit: '+name)
                require(re.fullmatch('[0-9a-f]{64}',file.get('sha256','')) and digest(target)==file['sha256'], 'SHA256 mismatch: '+name)
                seen.add(name)
        credits.append(f"{asset['id']} — {asset['author']} — {asset['license']}\n{asset['source_url']}\n{asset['attribution']}\n")
    require(not any(p.is_symlink() for p in root.rglob('*')), 'theme kits cannot contain symlinks')
    actual = {p.relative_to(root).as_posix() for p in root.rglob('*') if p.is_file() and p.relative_to(root).as_posix() not in METADATA}
    require(actual==seen, 'unlisted asset files: '+', '.join(sorted(actual-seen)))
    if any(a.get('provider')=='polyhaven' for a in document['assets']):
        credits.append('Powered by Poly Haven — https://polyhaven.com (API service credit; assets are CC0).\n')
    (root/'CREDITS').write_text('\n'.join(credits))
    return dict(ok=True,publishable=publishable,assets=len(ids),files=len(seen),credits=str(root/'CREDITS'))
