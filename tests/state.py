#!/usr/bin/env python3
"""Verify versioned POD fields and explicit N-to-N+1 state migration."""
import argparse
from pathlib import Path
import shlex
import sys
import re
import subprocess
from check_boundaries import TOKENS, blank
from run import ROOT, SCRATCH, run

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-state')
parser.add_argument('--cc',default='gcc')
parser.add_argument('--cxx',default='g++')
args=parser.parse_args()
args.output.mkdir(parents=True,exist_ok=True)
# Keep every owned engine libc draw visible to checkpoint capture.
raw_random=re.compile(r'\b(?:rand|srand)\s*\(')
assert raw_random.search('rand()') and raw_random.search('std::srand(1)')
assert not raw_random.search(TOKENS.sub(lambda match: blank(match[0]), '// rand()\n"srand()"'))
for name in subprocess.check_output(['git','ls-files','-z','--','engine'],cwd=ROOT).decode().split('\0'):
    if Path(name).suffix not in ('.cpp','.h','.c','.inc') or name in (
            'engine/qcommon/q_shared.cpp','engine/renderervk/shaders/spirv/shader_data.cpp'):
        continue
    source=TOKENS.sub(lambda match: blank(match[0]), (ROOT/name).read_text())
    assert not raw_random.search(source), f'{name}: use Q_Rand/Q_Srand so checkpoints retain the stream'
# Every entity callback assignment needs a stable identity in its owner table.
assigned, registered = set(), set()
callback_kinds = r'(think|reached|blocked|touch|use|pain|die)'
for path in (ROOT/'game/game').glob('*.cpp'):
    source=path.read_text()
    code=TOKENS.sub(lambda match: blank(match[0]), source)
    for kind, value in re.findall(r'(?:\b\w+|\])(?:->|\.)'+callback_kinds+r'\s*=(?!=)\s*([^;]+);', code):
        value=value.strip()
        if value in ('0','NULL','nullptr'):
            continue
        assert re.fullmatch(r'\w+', value), f'{path}: describe indirect callback assignment: {value}'
        assigned.add((kind,value))
    for name, fields in re.findall(r'\{\s*\.name\s*=\s*"(\w+)"\s*,([^}]+)\}',source):
        for kind, value in re.findall(r'\.'+callback_kinds+r'\s*=\s*(\w+)',fields):
            assert name==value, f'{path}: callback identity must retain its stable function name'
            assert (kind,value) not in registered, f'duplicate callback identity: {name}'
            registered.add((kind,value))
assert assigned==registered, f'callback coverage: missing {assigned-registered}, unused {registered-assigned}'
print(f'PASS: {len(assigned)} assigned entity callbacks have typed, stable save identities')
# New native members must be classified instead of silently disappearing from saves.
def declared_members(body):
    body=re.sub(r'/\*[\s\S]*?\*/|//[^\n]*|^\s*#.*$', '', body, flags=re.M)
    fields=set()
    for declaration in body.split(';'):
        declaration=declaration.strip()
        if not declaration:
            continue
        callback=re.fullmatch(r'void\s*\(\s*\*\s*(\w+)\s*\)\s*\([\s\S]*\)',declaration)
        if callback:
            fields.add(callback[1])
            continue
        match=re.fullmatch(r'(?:(?:struct|const)\s+)?\w+\s+([\s\S]+)',declaration)
        assert match, f'unclassified native declaration: {declaration}'
        for declarator in match[1].split(','):
            member=re.fullmatch(r'\s*\*?\s*(\w+)(?:\[\w+\])?\s*',declarator)
            assert member, f'unclassified native member: {declaration}'
            fields.add(member[1])
    return fields
local=(ROOT/'game/game/g_local.h').read_text()
entity_body=local.split('struct gentity_s {',1)[1].split('\n};',1)[0]
records=(ROOT/'game/game/g_state.cpp').read_text()
fields=set(re.findall(r'offsetof\( gentity_t, (\w+) \)',records))
fields.update(re.findall(r'&gentity_t::(\w+)',records))
strings=records.split('#define ENTITY_STRINGS( X )',1)[1].split('static bool CaptureString',1)[0]
fields.update(re.findall(r'X\( (\w+),',strings))
fields.update(('s','r','client','item','think','reached','blocked','touch','use','pain','die'))
assert fields==declared_members(entity_body), f'entity save field coverage: {fields ^ declared_members(entity_body)}'
shared=(ROOT/'engine/public/g_public.h').read_text().split('} entityShared_t;',1)[0].rsplit('typedef struct {',1)[1]
shared_fields=set(re.findall(r'offsetof\( entityShared_t, (\w+) \)',records)) | {'s'}
assert shared_fields==declared_members(shared), 'shared entity state needs a save description'
assert declared_members('int first, second; float added;')=={'first','second','added'}
print(f'PASS: every entity member has a scalar, reference, string, callback or shared-state description')
def native_body(name):
    end=local.index('} '+name+';')
    return local[:end].rsplit('typedef struct {',1)[1]
client_fields=set(re.findall(r'offsetof\( gclient_t, ([\w.]+) \)',records))
client_body=local.split('struct gclient_s {',1)[1].split('\n};',1)[0]
assert {field.split('.')[0] for field in client_fields} | {'ps','hook','persistantPowerup','areabits'} == declared_members(client_body)
for name, prefix, separate in (('clientPersistant_t','pers.',{'cmd','teamState'}),
                               ('clientSession_t','sess.',set()),
                               ('playerTeamState_t','pers.teamState.',set())):
    described={field[len(prefix):] for field in client_fields if field.startswith(prefix) and '.' not in field[len(prefix):]}
    assert described | separate == declared_members(native_body(name)), f'{name}: saved client field coverage'
print('PASS: every client/session/team member has a save description or explicit ownership rule')
run([sys.executable, 'tools/replication.py', '--check'])
sha=args.output/'sha.o'
probe=args.output/'probe'
shared=args.output/'shared.o'
run([*shlex.split(args.cc),'-std=c99','-O2','-c','third_party/sha256/sha-256.c','-o',sha])
run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
     '-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections','-fsanitize=undefined',
     '-fno-sanitize-recover=all','-c','engine/qcommon/q_shared.cpp','-o',shared])
for definitions in ([], ['-DSTATE_NATIVE_GAME']):
    run([*shlex.split(args.cxx),*definitions,'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
         '-Wall','-Wextra','-Werror','-Wconversion','-Wshadow','-fsanitize=undefined','-fno-sanitize-recover=all',
         '-ffunction-sections','-fdata-sections',
         'tests/probes/state.cpp','engine/qcommon/state.cpp',shared,sha,'-Wl,--gc-sections','-o',probe])
    run([probe])
run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
     '-fno-fast-math','-ffp-contract=off','-fno-strict-aliasing','-fwrapv','-fno-builtin',
     '-U_GNU_SOURCE','-D_DEFAULT_SOURCE','-D__NO_INLINE__','-Wall','-Werror',
     '-ffunction-sections','-fdata-sections','-fsanitize=undefined','-fno-sanitize-recover=all',
     'tests/probes/state_random.cpp','engine/qcommon/state.cpp',sha,'-Wl,--gc-sections','-o',probe])
run([probe])

callbacks=args.output/'callbacks.o'
run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
     '-ffunction-sections','-fdata-sections','-fsanitize=undefined','-fno-sanitize-recover=all',
     '-DNATIVE_NAMESPACE=game','-DNATIVE_SOURCE="game/g_callbacks.cpp"',
     '-c','game/module.cpp','-o',callbacks])
for component in ('callbacks','references'):
    run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
         '-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections',
         '-fsanitize=undefined','-fno-sanitize-recover=all',
         f'tests/probes/state_{component}.cpp','engine/qcommon/state.cpp',sha,
         *([callbacks] if component=='references' else []),'-Wl,--gc-sections','-o',probe])
    run([probe])
