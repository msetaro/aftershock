#!/usr/bin/env python3
"""Run path-selected feedback within a total time budget; full CI remains the merge gate."""
import argparse
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time
from run import ROOT, SCRATCH

# These are asset-independent entry points with no required binary arguments.
FAST = ('bot_chat_shutdown', 'agent_protocol', 'agent_client', 'agent_formats', 'affected_contract',
        'state', 'navigation', 'animation', 'entities', 'physics', 'ui_framework', 'audio_spatial', 'audio_events', 'audio_streams', 'audio_voice', 'capture', 'weapons', 'effects', 'effects_reference', 'post', 'temporal', 'lod', 'streaming', 'native_math', 'native_shared', 'replication',
        'protocol', 'rewind', 'replication_policy', 'identity', 'rhi', 'render_graph',
        'shadow_views', 'probes', 'materials', 'cook', 'level', 'lighting',
        'devtools_data', 'check_boundaries', 'check_types', 'check_format',
        'check_isolation', 'isolation', 'suite_contract', 'publish_build', 'match_content')
COMMANDS = {name: [sys.executable, 'tests/'+name+'.py'] for name in FAST}
COMMANDS['recipes'] = [sys.executable, 'tests/agent_recipes.py', '--check']
COMMANDS['unit'] = [sys.executable, 'tests/run.py', 'unit', '--negative-control']
COMMANDS['match_go'] = ['go', '-C', 'tools/match', 'test', '-race', './...']
# Prefix matches compose: a cooker animation edit needs both cooker and animation checks.
RULES = (
    (('engine/navigation/', 'game/game/g_navigation.cpp', 'tools/cook/navigation', 'tools/cook/behavior', 'third_party/recast/', 'tests/navigation', 'tests/probes/navigation', 'tests/probes/behavior', 'tests/probes/perception'), ('navigation',)),
    (('engine/qcommon/state.cpp', 'engine/qcommon/cvar.cpp', 'engine/qcommon/profile.cpp', 'engine/platform/sys_save.cpp', 'engine/platform/save_public.h', 'engine/client/cl_checkpoint.cpp', 'engine/server/sv_checkpoint.cpp', 'engine/server/sv_world.cpp', 'engine/public/state_public.h', 'engine/public/state_replication_public.h', 'tools/replication.py', 'engine/qcommon/q_shared.h', 'game/bg/q_shared.h', 'game/game/', 'engine/botlib/', 'tests/state.py', 'tests/probes/state'), ('state',)),
    (('engine/botlib/be_ai_chat.cpp',), ('bot_chat_shutdown',)),
    (('engine/entities/', 'tools/cook/entities', 'tests/entities', 'tests/probes/entities', 'tests/assets/entities/'), ('entities',)),
    (('engine/ui/', 'engine/client/cl_data_ui', 'tools/cook/ui', 'tests/ui_framework', 'tests/probes/ui_framework', 'tests/assets/ui/'), ('ui_framework',)),
    (('engine/client/cl_voice', 'engine/qcommon/voice_public.h', 'cmake/Audio.cmake', 'third_party/opus/'), ('audio_voice',)),
    (('engine/platform/sdl/sdl_snd.cpp', 'tests/probes/capture.cpp'), ('capture',)),
    (('tools/cook/audio.py', 'tests/audio_events'), ('audio_events',)),
    (('engine/sound/', 'tests/audio_spatial', 'tests/probes/audio_spatial'), ('audio_spatial', 'audio_events', 'audio_streams', 'audio_voice')),
    (('engine/qcommon/files.cpp', 'engine/platform/unix/unix_shared.cpp', 'engine/platform/win32/win_shared.cpp'), ('audio_streams',)),
    (('engine/physics/', 'engine/client/cl_physics', 'engine/qcommon/cm_physics', 'game/cgame/cg_physics', 'third_party/jolt', 'cmake/Physics.cmake', 'tests/physics', 'tests/probes/physics', 'tests/assets/physics/'), ('physics',)),
    (('engine/devtools/', 'tools/agent/'), ('agent_protocol', 'agent_client', 'agent_formats', 'devtools_data')),
    (('engine/effects/', 'tools/cook/effect', 'tests/assets/effects/'), ('effects', 'effects_reference')),
    (('engine/render/tr_stream', 'tests/streaming'), ('streaming',)),
    (('tools/cook/lod', 'engine/render/tr_model', 'engine/render/tr_mesh'), ('lod',)),
    (('engine/animation/', 'tools/cook/animation', 'tests/assets/animation/'), ('animation',)),
    (('engine/weapons/', 'tools/cook/weapon', 'tests/assets/weapons/'), ('weapons',)),
    (('engine/render/', 'engine/rhi/', 'engine/renderervk/', 'engine/renderercommon/'),
     ('rhi', 'render_graph', 'shadow_views', 'probes', 'materials')),
    (('engine/client/', 'engine/server/', 'engine/qcommon/', 'engine/public/'),
     ('unit', 'replication', 'protocol', 'rewind', 'replication_policy', 'identity')),
    (('game/', 'engine/botlib/'), ('native_math', 'native_shared', 'weapons')),
    (('engine/platform/', 'engine/sound/'), ('unit', 'check_boundaries')),
    (('tools/cook/',), ('cook',)),
    (('tools/level/', 'tests/assets/level/'), ('level', 'lighting')),
    (('tools/match/',), ('match_content', 'match_go')),
    (('tools/scratch.py', 'tests/run.py'), ('isolation', 'check_isolation', 'unit')),
    (('tests/affected',), ('affected_contract',)),
    (('tests/suite', '.github/workflows/'), ('suite_contract', 'publish_build', 'check_isolation', 'unit')),
    (('CMakeLists.txt', 'CMakePresets.json', 'cmake/', 'third_party/'), ('unit', 'check_boundaries')),
    (('.clang',), ('check_format',)),
    (('docs/agents/', 'tests/agent_recipes.py'), ('recipes',)),
    (('docs/', 'AGENTS.md', 'README.md'), ('check_isolation',)),
)


def select(paths):
    selected, fallback = [], []
    for path in paths:
        matched = False
        for prefixes, names in RULES:
            if path.startswith(prefixes):
                selected.extend(names)
                matched = True
        stem = Path(path).stem
        if path.startswith('tests/') and stem in COMMANDS:
            selected.append(stem)
            matched = True
        if not matched:
            fallback.append(path)
            selected.extend(('unit', 'check_boundaries'))
    return list(dict.fromkeys(selected)), fallback


def changed(base):
    merge_base = subprocess.check_output(['git', 'merge-base', 'HEAD', base],cwd=ROOT,text=True).strip()
    # Include committed changes, staged/unstaged edits and untracked source files.
    paths = subprocess.check_output(['git', 'diff', '--name-only', '-z', merge_base, '--'],cwd=ROOT).decode().split('\0')
    paths += subprocess.check_output(['git', 'ls-files', '--others', '--exclude-standard', '-z'],cwd=ROOT).decode().split('\0')
    return sorted(set(paths)-{''})


def execute(command, log_path, timeout):
    started = time.monotonic()
    with log_path.open('w') as log:
        process = subprocess.Popen(command,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
        try:
            process.wait(timeout=timeout)
            status = 'passed' if process.returncode == 0 else 'failed'
        except (subprocess.TimeoutExpired, KeyboardInterrupt) as error:
            os.killpg(process.pid,signal.SIGTERM)
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid,signal.SIGKILL)
                process.wait()
            if isinstance(error,KeyboardInterrupt):
                raise
            status = 'timeout'
    return dict(status=status,returncode=process.returncode,seconds=round(time.monotonic()-started,3),log=str(log_path))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('base_ref')
    parser.add_argument('--budget',type=float,default=600,help='total seconds; timeout/incomplete exits 2')
    parser.add_argument('--list',action='store_true',help='print the selected commands without running')
    args = parser.parse_args()
    if not 0 < args.budget <= 86400:
        parser.error('--budget must be finite and in (0,86400]')
    paths = changed(args.base_ref)
    names, fallback = select(paths)
    report = dict(full=False,ok=False,base=args.base_ref,paths=paths,budget_seconds=args.budget,
                  fallback_paths=fallback,tests=[dict(name=name,command=COMMANDS[name],status='pending') for name in names])
    if args.list:
        print(json.dumps(report,indent=2))
        return 0
    report_path = SCRATCH/'affected-report.json'
    print(f'Fast feedback: {len(names)} checks, {args.budget:g}s budget; full CI still required. Logs: {SCRATCH}',flush=True)
    if fallback:
        print('Unmapped paths use core checks; inspect coverage before merge: '+', '.join(fallback),flush=True)
    deadline = time.monotonic()+args.budget
    for test in report['tests']:
        report_path.write_text(json.dumps(report,indent=2)+'\n')
        remaining = deadline-time.monotonic()
        if remaining <= 0:
            break
        print(test['name'],flush=True)
        test.update(execute(test['command'], SCRATCH/(test['name']+'.log'), remaining))
        if test['status'] != 'passed':
            break
    report['ok'] = all(row['status']=='passed' for row in report['tests'])
    report_path.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(ok=report['ok'],full=False,report=str(report_path))),flush=True)
    if any(row['status']=='failed' for row in report['tests']):
        return 1
    return 0 if report['ok'] else 2


if __name__ == '__main__':
    sys.exit(main())
