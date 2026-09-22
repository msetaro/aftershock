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
from cook import cook

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
        callback=re.fullmatch(r'(?:void|int)\s*\(\s*\*\s*(\w+)\s*\)\s*\([\s\S]*\)',declaration)
        if callback:
            fields.add(callback[1])
            continue
        match=re.fullmatch(r'(?:(?:struct|const)\s+)?\w+\s+([\s\S]+)',declaration)
        assert match, f'unclassified native declaration: {declaration}'
        for declarator in match[1].split(','):
            member=re.fullmatch(r'\s*\*?\s*(\w+)(?:\[\w+\])*\s*',declarator)
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
level_fields={field.split('[')[0] for field in re.findall(r'offsetof\( level_locals_t, ([\w\[\]]+) \)',records)}
# References/strings are explicit records; these remaining values are recreated by map setup or are frame-local parsing scratch.
level_fields.update(('clients','gentities','gentitySize','logFile','locationHead','bodyQue','changemap',
                     'spawning','numSpawnVars','spawnVars','numSpawnVarChars','spawnVarChars'))
assert level_fields==declared_members(native_body('level_locals_t')), 'level member needs state or lifetime ownership'
print('PASS: every level member has a save description or explicit map/frame lifetime rule')
composed=(ROOT/'game/game/g_composed.cpp').read_text()
composed_body=composed.split('static struct composedState_t {',1)[1].split('} composed[',1)[0]
composed_fields=set(re.findall(r'offsetof\( composedSave_t, (\w+) \)',composed)) - {'flags'}
assert composed_fields | {'loopAnimation','once','loopSound'} == declared_members(composed_body)
animation=(ROOT/'game/game/g_animation.cpp').read_text()
actor_body=animation.split('} animationActors[',1)[0].rsplit('static struct {',1)[1]
actor_fields=set(re.findall(r'offsetof\( animationActorSave_t, (\w+) \)',animation))
assert actor_fields | {'state'} == declared_members(actor_body)
rig_body=animation.split('} animationRigs[',1)[0].rsplit('static struct {',1)[1]
assert declared_members(rig_body)=={'asset','storage','footHeight','rootYaw'}, 'animation rig ownership needs a save rule'
animation_public=(ROOT/'engine/animation/animation_public.h').read_text()
state_body=animation_public.split('struct animState_t {',1)[1].split('};',1)[0]
assert set(re.findall(r'offsetof\( animState_t, (\w+) \)',animation_public))==declared_members(state_body)
print('PASS: composed and animation state owners have complete checkpoint field coverage')
weapon=(ROOT/'game/game/g_data_weapons.cpp').read_text()
weapon_actor=weapon.split('} weaponActors[',1)[0].rsplit('static struct {',1)[1]
weapon_fields=set(re.findall(r'offsetof\( weaponActorSave_t, (\w+) \)',weapon))
assert weapon_fields | {'inventory','animation','configured'} == declared_members(weapon_actor)
weapon_public=(ROOT/'engine/weapons/weapons_public.h').read_text()
weapon_state=weapon_public.split('struct weaponState_t {',1)[1].split('};',1)[0]
assert set(re.findall(r'offsetof\( weaponState_t, (\w+) \)',weapon_public)) == declared_members(weapon_state)
print('PASS: weapon actor fields are saved or reconstructed from verified matching content')
history=(ROOT/'engine/qcommon/net_history_public.h').read_text()
for name, expected in (('netHistory_t',{'next','count','frames'}),
                       ('netHistoryFrame_t',{'time','boxCount','entities','boxes'}),
                       ('netHistoryEntity_t',{'generation','firstBox','boxCount'}),
                       ('netBox_t',{'mins','maxs'})):
    native=history.split('struct '+name+' {',1)[1].split('};',1)[0]
    assert declared_members(native)==expected, f'{name}: checkpoint history ownership needs review'
rewind=(ROOT/'game/game/g_rewind.cpp').read_text()
rewind_entities=rewind.split('} rewindEntities[',1)[0].rsplit('static struct {',1)[1]
assert declared_members(rewind_entities)=={'spawn','playerSpawn','teleport','generation'}
print('PASS: rewind ownership accounts for live history, generations, report clocks and frame-local scratch')
team=(ROOT/'game/game/g_team.cpp').read_text()
team_body=team.split('typedef struct teamgame_s {',1)[1].split('} teamgame_t;',1)[0]
team_fields=set(re.findall(r'offsetof\( teamSave_t, (\w+) \)',team))-{'neutral'}
assert team_fields==declared_members(team_body), 'team owner needs a saved field description'
utilities=(ROOT/'game/game/g_utils.cpp').read_text()
remap_body=utilities.split('} shaderRemap_t;',1)[0].rsplit('typedef struct {',1)[1]
assert set(re.findall(r'offsetof\( shaderRemap_t, (\w+) \)',utilities))==declared_members(remap_body)
print('PASS: team and shader remap records describe every persistent owner field')
main_header=(ROOT/'game/game/ai_main.h').read_text()
waypoint_body=main_header.split('typedef struct bot_waypoint_s {',1)[1].split('} bot_waypoint_t;',1)[0]
# Pointer links use checked indices; the goal has its own shared named record.
waypoint_fields=set(re.findall(r'offsetof\( bot_waypoint_t, (\w+) \)',(ROOT/'game/game/ai_dmq3.cpp').read_text()))
assert waypoint_fields | {'goal','next','prev'}==declared_members(waypoint_body)
goal=(ROOT/'game/game/be_ai_goal.h').read_text().split('typedef struct bot_goal_s {',1)[1].split('} bot_goal_t;',1)[0]
assert set(re.findall(r'offsetof\( bot_goal_t, (\w+) \)',main_header))==declared_members(goal)
print('PASS: bot waypoint and shared goal records describe all members')
bot_main=(ROOT/'game/game/ai_main.cpp').read_text()
bot_body=main_header.split('typedef struct bot_state_s {',1)[1].split('} bot_state_t;',1)[0]
bot_fields={name.split('.')[0] for name in re.findall(r'offsetof\( bot_state_t, ([\w.]+) \)',bot_main)}
bot_fields.update(re.findall(r'&bot_state_t::(\w+)',bot_main))
bot_fields.update(('cur_ps','lastucmd','ainode','activatestack','activategoalheap','checkpoints','patrolpoints','curpatrolpoint'))
assert bot_fields==declared_members(bot_body), f'bot actor member coverage: {bot_fields ^ declared_members(bot_body)}'
activation=main_header.split('typedef struct bot_activategoal_s {',1)[1].split('} bot_activategoal_t;',1)[0]
assert set(re.findall(r'offsetof\( bot_activategoal_t, (\w+) \)',bot_main)) | {'goal','next'}==declared_members(activation)
ai_nodes=set(re.findall(r'bs->ainode = (AINode_\w+);',(ROOT/'game/game/ai_dmnet.cpp').read_text()))
registered_nodes=set(re.findall(r'\{ "(AINode_\w+)", AINode_\w+ \}',bot_main))
assert ai_nodes==registered_nodes, 'AI node callback needs a stable saved identity'
print(f'PASS: all bot actor/activation members and {len(ai_nodes)} typed AI nodes have state ownership')



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
for component in ('callbacks','references','composed','utilities'):
    run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
         '-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections',
         '-fsanitize=undefined','-fno-sanitize-recover=all',
         f'tests/probes/state_{component}.cpp','engine/qcommon/state.cpp',sha,
         *([callbacks] if component=='references' else []),'-Wl,--gc-sections','-o',probe])
    run([probe])

assets=args.output/'animation-assets'
cook(ROOT/'tests/assets/animation/rigs.json',assets)
native=args.output/'native-state.o'
run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
     '-ffunction-sections','-fdata-sections','-fsanitize=undefined','-fno-sanitize-recover=all',
     '-DNATIVE_NAMESPACE=game','-DNATIVE_SOURCE="game/g_state.cpp"',
     '-c','game/module.cpp','-o',native])
run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
     '-ffp-contract=off','-fno-fast-math','-Wall','-Wextra','-Werror',
     '-ffunction-sections','-fdata-sections','-fsanitize=undefined','-fno-sanitize-recover=all',
     'tests/probes/state_animation.cpp','engine/animation/animation.cpp','engine/render/tr_cooked.cpp',
     'engine/qcommon/state.cpp',sha,native,'-Wl,--gc-sections','-o',probe])
run([probe,assets/'animations/anim_body.asanim',assets/'animations/anim_rifle.asanim'])

weapons=args.output/'weapons-assets'
range_assets=args.output/'range-assets'
cook(ROOT/'tests/assets/weapons/assets.json',weapons)
cook(ROOT/'tests/assets/range.json',range_assets)
run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
     '-ffp-contract=off','-fno-fast-math','-Wall','-Wextra','-Werror',
     '-ffunction-sections','-fdata-sections','-fsanitize=undefined','-fno-sanitize-recover=all',
     'tests/probes/state_weapons.cpp','engine/weapons/weapons.cpp','engine/animation/animation.cpp',
     'engine/render/tr_cooked.cpp','engine/qcommon/state.cpp',sha,native,'-Wl,--gc-sections','-o',probe])
run([probe,weapons/'weapons/range_rifle.asweapon',range_assets/'animations/range_rifle.asanim'])

run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
     '-ffp-contract=off','-fno-fast-math','-Wall','-Wextra','-Werror',
     '-ffunction-sections','-fdata-sections','-fsanitize=undefined','-fno-sanitize-recover=all',
     'tests/probes/state_rewind.cpp','engine/qcommon/net_history.cpp',
     'engine/qcommon/state.cpp',sha,'-Wl,--gc-sections','-o',probe])
run([probe])

for component in ('COMBAT','TEAM','PODIUM'):
    run([*shlex.split(args.cxx),f'-DSTATE_{component}','-std=c++20','-O2','-fno-exceptions','-fno-rtti',
         '-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections',
         '-fsanitize=undefined','-fno-sanitize-recover=all',
         'tests/probes/state_globals.cpp','engine/qcommon/state.cpp',sha,native,
         '-Wl,--gc-sections','-o',probe])
    run([probe])

for definitions in ([],['-DAFTERSHOCK_DEVTOOLS']):
    run([*shlex.split(args.cxx),*definitions,'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
         '-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections',
         '-fsanitize=undefined','-fno-sanitize-recover=all',
         'tests/probes/state_definitions.cpp','engine/entities/entities.cpp','engine/qcommon/state.cpp',sha,
         '-Wl,--gc-sections','-o',probe])
    run([probe])

for component in ('QUEUE','CLOCK','TEAM'):
    run([*shlex.split(args.cxx),f'-DSTATE_{component}','-std=c++20','-O2','-fno-exceptions','-fno-rtti',
         '-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections',
         '-fsanitize=undefined','-fno-sanitize-recover=all',
         'tests/probes/state_bot_globals.cpp','engine/qcommon/state.cpp',sha,
         '-Wl,--gc-sections','-o',probe])
    run([probe])

run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
     '-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections',
     '-fsanitize=undefined','-fno-sanitize-recover=all',
     'tests/probes/state_waypoints.cpp','engine/qcommon/state.cpp',sha,
     '-Wl,--gc-sections','-o',probe])
run([probe])

run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
     '-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections',
     '-fsanitize=undefined','-fno-sanitize-recover=all',
     'tests/probes/state_bot_actor.cpp','engine/qcommon/state.cpp',sha,
     '-Wl,--gc-sections','-o',probe])
run([probe])
