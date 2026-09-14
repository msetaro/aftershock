from pathlib import Path
import subprocess,shutil,re,json,hashlib,difflib,sys
b=Path('/tmp/port-pr32');rows=[];logs={};bins={};install=b/'runtime';install.mkdir(exist_ok=True)
for mode in ['c','cxx']:
 binary=b/('build-'+mode)/'release-linux-x86_64/quake3e.ded.x64';bins[mode]=binary;shared=install/'quake3e.ded.x64';shutil.copy2(binary,shared)
 command=['timeout','90','faketime','-f','@2026-01-01 00:00:00 i0.01',str(shared),'+set','dedicated','1','+set','sv_pure','0','+set','com_logfile','0','+map','q3dm17','+addbot','sarge','3','+addbot','major','3','+wait','300','+quit']
 for run in ['warm1','warm2','1','2']:
  path=b/f'runtime-{mode}-{run}.log'
  with path.open('wb') as f:status=subprocess.run(command,stdout=f,stderr=subprocess.STDOUT).returncode
  assert status==0,(path,status)
  raw=path.read_bytes();data=re.sub(rb'^\.\.\.found [0-9]+ cached paks\r?\n',b'',raw,flags=re.M);(b/f'runtime-{mode}-{run}.normalized.log').write_bytes(data);logs[mode+'-'+run]=data
  rows.append(dict(mode=mode,run=run,command=command,binary=str(binary),binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),raw_sha256=hashlib.sha256(raw).hexdigest(),normalized_sha256=hashlib.sha256(data).hexdigest(),lines=len(data.splitlines())))
assert logs['c-1']==logs['c-2'];assert logs['cxx-1']==logs['cxx-2'];diff=''.join(difflib.unified_diff(logs['c-2'].decode().splitlines(True),logs['cxx-1'].decode().splitlines(True)));(b/'runtime.diff').write_text(diff);assert not diff,diff
# Same documented function-name normalization used by G3, retain global kind/name.

symbols={}
for mode,binary in bins.items():
 for kind,args in [('defined',['--defined-only']),('undefined',['--undefined-only'])]:
  command=['nm','-g',*args,str(binary)];raw=subprocess.check_output(command,text=True);raw=subprocess.run(['c++filt'],input=raw,text=True,capture_output=True,check=True).stdout
  names=[]
  for line in raw.splitlines():
   fields=line.split(None,2) if kind=='defined' else line.split(None,1)
   typ,name=fields[-2:];names.append(typ+' '+re.sub(r'\(.*\)', '', name))
  symbols[mode,kind]=set(names);(b/f'symbols-{mode}-{kind}.txt').write_text('\n'.join(sorted(names))+'\n')
assert symbols['c','defined']==symbols['cxx','defined'],(symbols['c','defined']^symbols['cxx','defined'])
result=dict(runtime=rows,defined_counts={m:len(symbols[m,'defined']) for m in bins},defined_identical=True,undefined_c_only=sorted(symbols['c','undefined']-symbols['cxx','undefined']),undefined_cxx_only=sorted(symbols['cxx','undefined']-symbols['c','undefined']))
(b/'acceptance-results.json').write_text(json.dumps(result,indent=2)+'\n');print('PASS: C repeat, C++ repeat, C vs C++ runtime; global defined symbols identical');print(result['defined_counts']);print('C-only imports:',result['undefined_c_only']);print('C++-only imports:',result['undefined_cxx_only'])
