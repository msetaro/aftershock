#!/usr/bin/env python3
"""G7 clang-tidy on changed lines, using verified native Makefile contexts."""
from pathlib import Path
import subprocess,shlex,re,json,os,sys
from concurrent.futures import ThreadPoolExecutor
from collections import Counter
os.chdir(Path(__file__).resolve().parents[2])
base=Path(sys.argv[1] if len(sys.argv)>1 else '/tmp/aftershock-cpp-port/static').resolve()
base.mkdir(parents=True,exist_ok=True)
progress=Path('docs/cpp-port-progress.md').read_text()
files=subprocess.check_output(['git','diff','--name-only','8a7e8ed2','HEAD','--','code'],text=True).splitlines()
filters=[]
for source in files:
 diff=subprocess.check_output(['git','diff','--unified=0','8a7e8ed2','HEAD','--',source],text=True)
 ranges=[]
 for start,count in re.findall(r'^@@ .* \+(\d+)(?:,(\d+))? @@',diff,re.M):
  n=int(count or '1')
  if n:ranges.append([int(start),int(start)+n-1])
 if ranges:filters.append({'name':source,'lines':ranges})
line_filter=json.dumps(filters)
def probe(source):
 row=next((line for line in progress.splitlines() if line.startswith('| `'+source+'` |')), '')
 match=re.search(r'\(([^() ]+\.o)(?:,|\))',row)
 if not match or '| done |' not in row:return source,None,'SKIP: no verified native object context\n'
 target=str(base/'commands'/'release-linux-x86_64'/match[1])
 variables=['USE_SDL=0'] if 'nosdl' in row else []
 recipe=subprocess.check_output(['make','-Bn','V=1','BUILD_CXX=1','BUILD_DIR='+str(base/'commands'),*variables,target],text=True)
 commands=[shlex.split(line) for line in recipe.splitlines() if ' -c code/' in line]
 args=next(args for args in commands if '-o' in args and args[args.index('-o')+1]==target)[1:]
 i=args.index('-o');del args[i:i+2];args.remove('-c');args.remove(source)
 args=[arg for arg in args if arg not in ('-MMD','-MP')]
 r=subprocess.run(['clang-tidy',source,'--quiet','--line-filter='+line_filter,'--',*args],capture_output=True,text=True)
 return source,r.returncode,r.stdout+r.stderr
with ThreadPoolExecutor(4) as pool:results=list(pool.map(probe,[f for f in files if f.endswith('.c')]))
(base/'clang-tidy.log').write_text(''.join(source+'\n'+err for source,_,err in results))
print('clang-tidy:',sum(status is not None for _,status,_ in results),'checked,',sum(bool(status) for _,status,_ in results),'failed,',sum(status is None for _,status,_ in results),'skipped')

(base/'summary.txt').write_text('checked %d, failed %d, skipped %d\n' % (sum(status is not None for _,status,_ in results),sum(bool(status) for _,status,_ in results),sum(status is None for _,status,_ in results)))

warnings=Counter(re.findall(r'warning:.*\[([^]]+)\]', ''.join(err for _,_,err in results)))
print('Analysis findings:', dict(warnings))
(base/'warnings.json').write_text(json.dumps(warnings, indent=2)+'\n')
sys.exit(any(status for _,status,_ in results))
