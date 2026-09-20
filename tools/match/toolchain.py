"""Pinned Linux x86_64 development tools, installed only into the user cache."""
import hashlib
import io
from pathlib import Path
import platform
import tarfile
import urllib.request

ROOT=Path.home()/'.cache/aftershock-match-tools'
NODE='kindest/node:v1.35.8@sha256:07b2536e30b803ed61d1677a79df6115f798ce64c80f9e22f6ed45afd09323c0'
TOOLS={
 'kind':('https://github.com/kubernetes-sigs/kind/releases/download/v0.33.0/kind-linux-amd64',
         'aee6151561422756b764a4ae28e7f44cda5af5a9eead3cc9985112b1de8d8e0d',None),
 'kubectl':('https://dl.k8s.io/release/v1.35.8/bin/linux/amd64/kubectl',
            '874d5e72dbb819f43cff16bcd1e4f8bac5b7f2361fe1e55049b0a6c676fb0cbf',None),
 'helm':('https://get.helm.sh/helm-v4.3.0-linux-amd64.tar.gz',
         '86584a54def73570558f66f5111cc53dfed56689637ae32c1201205d494f54fb','linux-amd64/helm'),
 'agones-1.60.0.tgz':('https://agones.dev/chart/stable/agones-1.60.0.tgz',
         'ecde96e5a61b5869ca2c58d8da0b46b2ae85fb889fb69d106cae7df693a6ee34',None),
}
def tool(name):
    if platform.system()!='Linux' or platform.machine()!='x86_64':
        raise ValueError('kind acceptance tooling is pinned for Linux x86_64')
    url,digest,member=TOOLS[name]
    ROOT.mkdir(parents=True,exist_ok=True)
    archive=ROOT/(name+'.download')
    if not archive.exists() or hashlib.sha256(archive.read_bytes()).hexdigest()!=digest:
        with urllib.request.urlopen(url,timeout=60) as response:
            data=response.read(256*1024*1024+1)
        if hashlib.sha256(data).hexdigest()!=digest:raise ValueError(name+' download SHA256 mismatch')
        archive.write_bytes(data)
    data=archive.read_bytes()
    if member:
        with tarfile.open(fileobj=io.BytesIO(data)) as tar:data=tar.extractfile(member).read()
    target=ROOT/name
    if not target.exists() or target.read_bytes()!=data:
        target.write_bytes(data)
    if member or name in ('kind','kubectl'):target.chmod(0o755)
    return str(target)
