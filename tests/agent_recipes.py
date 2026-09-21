#!/usr/bin/env python3
"""Execute handbook shell blocks; Git recipes use private histories and recorded API shapes."""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
from run import ROOT, SCRATCH

NAMES = ('add-weapon', 'make-level', 'import-character', 'add-effect', 'debug-prediction', 'bisect-golden', 'known-good')


def blocks(name):
    source = ROOT/'docs/agents'/(name+'.md')
    commands = re.findall(r'^```sh\n(.*?)^```$',source.read_text(),re.M|re.S)
    assert commands, source
    for command in commands:
        subprocess.run(['bash','-n'],input=command,text=True,check=True)
    return '\n'.join(commands)


def history(path):
    path.mkdir(parents=True)
    env = dict(os.environ,GIT_AUTHOR_NAME='Recipe test',GIT_AUTHOR_EMAIL='recipe@localhost',
               GIT_COMMITTER_NAME='Recipe test',GIT_COMMITTER_EMAIL='recipe@localhost')
    def git(*args):
        return subprocess.check_output(['git','-c','commit.gpgsign=false',*args],cwd=path,env=env,text=True).strip()
    git('init','-b','main')
    (path/'tests').mkdir()
    (path/'tests/run.py').write_text("from pathlib import Path\nassert Path('value').read_text() == Path('golden').read_text()\n")
    (path/'value').write_text('1')
    (path/'golden').write_text('1')
    git('add','.')
    git('commit','-m','good')
    good = git('rev-parse','HEAD')
    (path/'README.md').write_text('An unrelated change.\n')
    git('add','.')
    git('commit','-m','neutral')
    (path/'value').write_text('2')
    git('add','.')
    git('commit','-m','changed behavior')
    return good,git('rev-parse','HEAD'),env


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check',action='store_true',help='parse every recipe without runtime prerequisites')
    parser.add_argument('--client',type=Path)
    parser.add_argument('--server',type=Path)
    parser.add_argument('--content',choices=['quake3','openarena'],default='quake3')
    parser.add_argument('--data',type=Path,default=Path.home()/'.q3a/baseq3')
    args = parser.parse_args()
    commands = {name:blocks(name) for name in NAMES}
    if args.check:
        print('PASS: all seven handbook recipes have executable shell blocks')
        return
    if not args.client or not args.server:
        parser.error('--client and --server development binaries are required')
    output = SCRATCH/'agent-recipes'
    output.mkdir()
    fixture = output/'history'
    good,bad,env = history(fixture)
    stub = output/'bin'
    stub.mkdir()
    # No GitHub write is performed. The real recipe still checks every API job.
    gh = stub/'gh'
    gh.write_text('''#!/usr/bin/env python3
import json,os,sys
path=sys.argv[-1]
sha=os.environ['VERIFIED_COMMIT']
if path.endswith('/commits/main'):
    value={'sha':sha}
elif '/workflows/' in path:
    value={'workflow_runs':[{'id':1 if '/build.yml/' in path else 2,'head_sha':sha,'status':'completed','conclusion':'success'}]}
elif '/runs/1/jobs' in path:
    value={'total_count':18,'jobs':[{'name':str(i),'status':'completed','conclusion':os.environ.get('RECIPE_CHECK','success')} for i in range(16)]+[{'name':'create-testing','status':'completed','conclusion':'success'},{'name':'update-release','status':'completed','conclusion':'skipped'}]}
elif '/runs/2/jobs' in path:
    value={'total_count':10,'jobs':[{'name':str(i),'status':'completed','conclusion':'success'} for i in range(10)]}
else:
    raise SystemExit('unexpected gh command: '+repr(sys.argv))
print(json.dumps(value))
''')
    gh.chmod(0o755)
    env.update(PATH=str(stub)+os.pathsep+str(Path(sys.executable).parent)+os.pathsep+env['PATH'],RECIPE_REPOSITORY=str(fixture),
               BISECT_GOOD=good,BISECT_BAD=bad,VERIFIED_COMMIT=bad,NEW_KNOWN_GOOD='known-good-2099-01-01',
               CLIENT=str(args.client.resolve()),SERVER=str(args.server.resolve()),
               CONTENT=args.content,DATA=str(args.data.resolve()),MAP='oa_dm1' if args.content=='openarena' else 'q3dm17')
    selected_prefix = subprocess.check_output(['python3','-c','import sys; print(sys.prefix)'],env=env,text=True).strip()
    assert selected_prefix == sys.prefix, 'recipe shell escaped the selected Python environment'
    for name,command in commands.items():
        destination = output/name
        destination.mkdir()
        env['RECIPE_OUT'] = str(destination)
        with (output/(name+'.log')).open('w') as log:
            result = subprocess.run(['bash','-e','-o','pipefail','-c',command],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=600)
        assert result.returncode == 0, (name,(output/(name+'.log')).read_text()[-4000:])
        print('PASS recipe: '+name,flush=True)
    assert bad in (output/'bisect-golden/bisect.log').read_text()
    original = subprocess.check_output(['git','rev-parse',env['NEW_KNOWN_GOOD']],cwd=fixture)
    for state in ('failure','skipped','success'):
        env['RECIPE_CHECK'] = state
        result = subprocess.run(['bash','-e','-o','pipefail','-c',commands['known-good']],cwd=ROOT,env=env,capture_output=True,timeout=30)
        assert result.returncode != 0, 'red/skipped checks or an existing tag were accepted'
        assert subprocess.check_output(['git','rev-parse',env['NEW_KNOWN_GOOD']],cwd=fixture)==original
    print('PASS: recipe Git checks reject red/skipped checks and never replace an existing tag')


if __name__ == '__main__':
    main()
