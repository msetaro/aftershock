#!/usr/bin/env python3
"""Run the active regression workflow locally with preinstalled prerequisites."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import signal
import subprocess
import sys
import time

import yaml
from run import ROOT, SCRATCH


def applies(expression, matrix):
    if not expression:
        return True
    equal = re.fullmatch(r"matrix\.(\w+) == '([^']*)'", expression)
    if equal:
        return matrix[equal[1]] == equal[2]
    prefix = re.fullmatch(r"startsWith\(matrix\.(\w+), '([^']*)'\)", expression)
    if prefix:
        return matrix[prefix[1]].startswith(prefix[2])
    raise ValueError('unsupported workflow condition; update local runner: '+expression)


def local_command(command, matrix, args):
    token = '${{ env.AFTERSHOCK_SCRATCH }}'
    lines = []
    for line in command.splitlines():
        stripped = line.strip()
        # Hosted setup is supplied by the local environment. No test is omitted.
        if (stripped.startswith('sudo apt-get ') or '"AFTERSHOCK_SCRATCH=$(mktemp ' in line
                or stripped.startswith('python3 -m venv ') or '/bin/pip install ' in line):
            continue
        lines.append(line)
    command = '\n'.join(lines)
    if 'tests/shaders.py' in command:
        # Use the caller's pinned compiler; shaders.py verifies its version and bytes.
        command = 'python3 tests/shaders.py --compiler '+shlex.quote(str(args.glslang))
    command = command.replace(token+'/aftershock-cook-python/bin/python',shlex.quote(sys.executable))
    command = re.sub(r'\bpython3\b',shlex.quote(sys.executable),command)
    command = command.replace(" --clang-format 'pipx run --spec clang-format==21.1.8 clang-format'",'')
    for key,value in matrix.items():
        command = command.replace('${{ matrix.'+key+' }}',str(value))
    command = command.replace('"'+token,'"$AFTERSHOCK_SCRATCH')
    command = command.replace(token,'"$AFTERSHOCK_SCRATCH"')
    for build in ('build/cross','build/sdl'):
        command = command.replace(build,'"$AFTERSHOCK_SCRATCH"/'+build)
    image = 'aftershock-match:local-'+hashlib.sha256(str(SCRATCH).encode()).hexdigest()[:12]
    command = command.replace('aftershock-match:issue28',image)
    if args.openarena_data:
        command = command.replace('tests/openarena.py','tests/openarena.py --data '+shlex.quote(str(args.openarena_data.resolve())))
    if '${{' in command:
        raise ValueError('unexpanded workflow expression: '+command)
    return command.strip()


def catalog(args):
    workflow = yaml.safe_load((ROOT/'.github/workflows/regression.yml').read_text())
    jobs = []
    for name,job in workflow['jobs'].items():
        if name == 'fuzz':
            continue
        matrix = job.get('strategy',{}).get('matrix',{})
        if matrix and set(matrix) != {'include'}:
            raise ValueError('local catalog requires explicit matrix include rows')
        for values in matrix.get('include',[{}]):
            identifier = name+('-'+values['cc'] if 'cc' in values else '')
            steps = []
            for step in job['steps']:
                if 'run' not in step or not applies(step.get('if'),values):
                    continue
                command = local_command(step['run'],values,args)
                if command:
                    steps.append(dict(name=step.get('name','run'),command=command))
            jobs.append(dict(id=identifier,job=name,steps=steps))
    return jobs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--list',action='store_true')
    parser.add_argument('--job',action='append',help='job or exact variant; omit to run the full local suite')
    parser.add_argument('--glslang',type=Path,default=Path('glslang'))
    parser.add_argument('--openarena-data',type=Path,help='installed OpenArena data, staged separately in each job')
    parser.add_argument('--timeout',type=int,default=3600,help='maximum seconds for an individual workflow step')
    args = parser.parse_args()
    jobs = catalog(args)
    if args.job:
        choices = {row['id'] for row in jobs}|{row['job'] for row in jobs}
        if set(args.job)-choices:
            parser.error('unknown job: '+str(set(args.job)-choices))
        jobs = [row for row in jobs if row['id'] in args.job or row['job'] in args.job]
    if args.list:
        print(json.dumps(jobs,indent=2))
        return 0
    report = dict(version=1,full=not args.job,ok=False,
                  head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
                  dirty=bool(subprocess.check_output(['git','status','--porcelain'],cwd=ROOT)),jobs=[])
    report_path = SCRATCH/'suite-report.json'
    print('Suite root: '+str(SCRATCH),flush=True)
    for job in jobs:
        output = SCRATCH/job['id']
        output.mkdir(parents=True,exist_ok=True)
        env = dict(os.environ,AFTERSHOCK_SCRATCH=str(output),TMPDIR=str(output))
        result = dict(id=job['id'],ok=False,steps=[])
        report['jobs'].append(result)
        for index,step in enumerate(job['steps']):
            log_path = output/f'{index:02d}.log'
            evidence = dict(name=step['name'],log=str(log_path),status='running')
            result['steps'].append(evidence)
            report_path.write_text(json.dumps(report,indent=2)+'\n')
            print(job['id']+': '+step['name']+' -> '+str(log_path),flush=True)
            started = time.monotonic()
            try:
                with log_path.open('w') as log:
                    process = subprocess.Popen(['bash','-e','-o','pipefail','-c',step['command']],cwd=ROOT,env=env,
                                               stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
                    try:
                        process.wait(timeout=args.timeout)
                    except (subprocess.TimeoutExpired,KeyboardInterrupt):
                        os.killpg(process.pid,signal.SIGTERM)
                        try:
                            process.wait(timeout=10)
                        except subprocess.TimeoutExpired:
                            os.killpg(process.pid,signal.SIGKILL)
                            process.wait()
                        raise
                evidence['status'] = 'passed' if process.returncode==0 else 'failed'
                evidence['returncode'] = process.returncode
            except subprocess.TimeoutExpired:
                evidence['status'] = 'timeout'
            evidence['seconds'] = round(time.monotonic()-started,3)
            report_path.write_text(json.dumps(report,indent=2)+'\n')
            if evidence['status'] != 'passed':
                print(log_path.read_text(errors='replace')[-6000:],flush=True)
                break
        else:
            result['ok'] = True
    report['ok'] = all(row['ok'] for row in report['jobs'])
    report_path.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(ok=report['ok'],full=report['full'],report=str(report_path))),flush=True)
    return 0 if report['ok'] else 1


if __name__ == '__main__':
    sys.exit(main())
