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
        match=re.fullmatch(r'(?:(?:struct|const|union)\s+)?(?:unsigned\s+short(?:\s+int)?|\w+)\s+([\s\S]+)',declaration)
        assert match, f'unclassified native declaration: {declaration}'
        for declarator in match[1].split(','):
            member=re.fullmatch(r'\s*\**\s*(\w+)(?:\[\w+\])*\s*',declarator)
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
input_source=(ROOT/'engine/botlib/be_ea.cpp').read_text()
input_body=(ROOT/'engine/botlib/botlib_public.h').read_text().split('typedef struct bot_input_s {',1)[1].split('} bot_input_t;',1)[0]
assert set(re.findall(r'offsetof\( bot_input_t, (\w+) \)',input_source))==declared_members(input_body)
move_source=(ROOT/'engine/botlib/be_ai_move.cpp').read_text()
move_body=move_source.split('typedef struct bot_movestate_s {',1)[1].split('} bot_movestate_t;',1)[0]
move_fields=set(re.findall(r'offsetof\( bot_movestate_t, ([\w.\[\]]+) \)',move_source))
assert {name.split('[')[0] for name in move_fields}==declared_members(move_body)
move_header=(ROOT/'engine/botlib/be_ai_move.h').read_text()
spots=int(re.search(r'#define MAX_AVOIDSPOTS\s+(\d+)',move_header)[1])
spot_body=move_header.split('typedef struct bot_avoidspot_s {',1)[1].split('} bot_avoidspot_t;',1)[0]
for i in range(spots):
    assert {name.split('.')[1] for name in move_fields if name.startswith(f'avoidspots[{i}].')}==declared_members(spot_body)
assert len([name for name in move_fields if name.startswith('avoidspots[')])==spots*3
print('PASS: botlib input and movement metadata account for every member and avoidance slot')
weight_header=(ROOT/'engine/botlib/be_ai_weight.h').read_text()
for tag,name,expected in (('fuzzyseperator_s','fuzzyseperator_t',{'index','value','type','weight','minweight','maxweight','child','next'}),
                          ('weight_s','weight_t',{'name','firstseperator'}),
                          ('weightconfig_s','weightconfig_t',{'numweights','weights','filename'})):
    body=weight_header.split('typedef struct '+tag+' {',1)[1].split('} '+name+';',1)[0]
    assert declared_members(body)==expected, f'{name}: weight topology/value ownership changed'
print('PASS: bot weights classify topology, names, mutable values and pointer ownership')
goal_source=(ROOT/'engine/botlib/be_ai_goal.cpp').read_text()
goal_body=goal_source.split('typedef struct bot_goalstate_s {',1)[1].split('} bot_goalstate_t;',1)[0]
goal_fields=set(re.findall(r'offsetof\( bot_goalstate_t, ([\w.\[\]]+) \)',goal_source))
assert {name.split('[')[0] for name in goal_fields} | {'itemweightconfig','itemweightindex'}==declared_members(goal_body)
goal_header=(ROOT/'engine/botlib/be_ai_goal.h').read_text()
stack=int(re.search(r'#define MAX_GOALSTACK\s+(\d+)',goal_header)[1])
goal_record=goal_header.split('typedef struct bot_goal_s {',1)[1].split('} bot_goal_t;',1)[0]
for i in range(stack):
    assert {name.split('.')[1] for name in goal_fields if name.startswith(f'goalstack[{i}].')}==declared_members(goal_record)
assert len([name for name in goal_fields if name.startswith('goalstack[')])==stack*8
print('PASS: botlib goal stack, avoidance and content-pointer ownership cover every member')
level_item_body=goal_source.split('typedef struct levelitem_s {',1)[1].split('} levelitem_t;',1)[0]
assert set(re.findall(r'offsetof\( levelItemsSave_t, (\w+) \)',goal_source))==declared_members(level_item_body)
print('PASS: level-item payload and list-link ownership cover every native member')
weapon_source=(ROOT/'engine/botlib/be_ai_weap.cpp').read_text()
weapon_state=weapon_source.split('typedef struct bot_weaponstate_s {',1)[1].split('} bot_weaponstate_t;',1)[0]
assert declared_members(weapon_state)=={'weaponweightconfig','weaponweightindex'}
print('PASS: botlib weapon state retains weight ownership and immutable index/content identity')
character_source=(ROOT/'engine/botlib/be_ai_char.cpp').read_text()
for tag,name,expected in (('bot_character_s','bot_character_t',{'c','filename','skill','refcnt','reftime'}),
                          ('bot_characteristic_s','bot_characteristic_t',{'type','value'})):
    body=character_source.split('typedef struct '+tag+' {',1)[1].split('} '+name+';',1)[0]
    assert declared_members(body)==expected, f'{name}: character identity/clock ownership changed'
union=character_source.split('union cvalue {',1)[1].split('};',1)[0]
assert declared_members(union)=={'integer','_float','string'}
print('PASS: character attributes, reference counts and rebased process-clock ages have explicit ownership')
chat_source=(ROOT/'engine/botlib/be_ai_chat.cpp').read_text()
chat_body=chat_source.split('typedef struct bot_chatstate_s {',1)[1].split('} bot_chatstate_t;',1)[0]
assert set(re.findall(r'offsetof\( bot_chatstate_t, (\w+) \)',chat_source)) | {'firstmessage','lastmessage','chat'}==declared_members(chat_body)
chat_header=(ROOT/'engine/botlib/be_ai_chat.h').read_text()
message_body=chat_header.split('typedef struct bot_consolemessage_s {',1)[1].split('} bot_consolemessage_t;',1)[0]
assert declared_members(message_body)=={'prev','next','message','time','type','handle'}
print('PASS: chat actor/console fields have scalar, queue-link or separate chat-content ownership')
for tag,name,expected in (
 ('bot_chatmessage_s','bot_chatmessage_t',{'chatmessage','time','next'}),
 ('bot_chattype_s','bot_chattype_t',{'name','numchatmessages','firstchatmessage','next'}),
 ('bot_chat_s','bot_chat_t',{'types','filename','chatname'}),
 ('bot_replychat_s','bot_replychat_t',{'keys','priority','numchatmessages','firstchatmessage','next'}),
 ('bot_replychatkey_s','bot_replychatkey_t',{'flags','string','match','next'}),
 ('bot_randomstring_s','bot_randomstring_t',{'string','next'}),
 ('bot_randomlist_s','bot_randomlist_t',{'string','numstrings','firstrandomstring','next'}),
 ('bot_synonym_s','bot_synonym_t',{'string','weight','next'}),
 ('bot_synonymlist_s','bot_synonymlist_t',{'context','totalweight','firstsynonym','next'}),
 ('bot_matchstring_s','bot_matchstring_t',{'string','next'}),
 ('bot_matchpiece_s','bot_matchpiece_t',{'type','firststring','variable','next'}),
 ('bot_matchtemplate_s','bot_matchtemplate_t',{'context','type','subtype','first','next'})):
    body=chat_source.split('typedef struct '+tag+' {',1)[1].split('} '+name+';',1)[0]
    assert declared_members(body)==expected, f'{name}: chat timer/content identity changed'
print('PASS: chat line timers and all immutable matching/expansion graph fields have ownership')
libvar_header=(ROOT/'engine/botlib/l_libvar.h').read_text()
body=libvar_header.split('typedef struct libvar_s {',1)[1].split('} libvar_t;',1)[0]
assert declared_members(body)=={'name','string','flags','modified','value','next'}
print('PASS: bot variables retain named payload, flags, cached numeric values and list order')
interface_header=(ROOT/'engine/botlib/be_interface.h').read_text()
body=interface_header.split('typedef struct botlib_globals_s {',1)[1].split('} botlib_globals_t;',1)[0]
interface_source=(ROOT/'engine/botlib/be_interface.cpp').read_text()
assert set(re.findall(r'offsetof\( botlibGlobalSave_t, globals\.(\w+) \)',interface_source))==declared_members(body)
for tag,name in (('maplocation_s','maplocation_t'),('campspot_s','campspot_t')):
    body=goal_source.split('typedef struct '+tag+' {',1)[1].split('} '+name+';',1)[0]
    assert set(re.findall(r'offsetof\( '+name+r', (\w+) \)',goal_source)) | {'next'}==declared_members(body)
print('PASS: botlib global clocks and immutable map-location/camp descriptors have complete field ownership')
aas_header=(ROOT/'engine/botlib/be_aas.h').read_text()
aas_entities=(ROOT/'engine/botlib/be_aas_entity.cpp').read_text()
body=aas_header.split('typedef struct aas_entityinfo_s {',1)[1].split('} aas_entityinfo_t;',1)[0]
assert set(re.findall(r'offsetof\( aas_entityinfo_t, (\w+) \)',aas_entities))==declared_members(body)
aas_def=(ROOT/'engine/botlib/be_aas_def.h').read_text()
body=aas_def.split('typedef struct aas_entity_s {',1)[1].split('} aas_entity_t;',1)[0]
assert declared_members(body)=={'i','areas','leaves'}
body=aas_def.split('typedef struct aas_link_s {',1)[1].split('} aas_link_t;',1)[0]
aas_links=(ROOT/'engine/botlib/be_aas_sample.cpp').read_text()
assert set(re.findall(r'offsetof\( aasLinksSave_t, (\w+) \)',aas_links)) - {'areaHeads','entityHeads'}==declared_members(body)
print('PASS: AAS entity history and spatial link membership/order have complete field ownership')
aas_move=(ROOT/'engine/botlib/be_aas_move.cpp').read_text()
body=aas_def.split('typedef struct aas_settings_s {',1)[1].split('} aas_settings_t;',1)[0]
assert set(re.findall(r'offsetof\( aas_settings_t, (\w+) \)',aas_move))==declared_members(body)
print('PASS: every AAS physics setting has a named checkpoint field')
body=aas_def.split('typedef struct aas_routingcache_s {',1)[1].split('} aas_routingcache_t;',1)[0]
assert declared_members(body)=={'type','time','size','cluster','areanum','origin','starttraveltime','travelflags','prev','next','time_prev','time_next','reachabilities','traveltimes'}
# Cache size/inline data pointers are reconstructed, list time order is the record
# slot order, and synchronous routing-update work memory is dead outside queries.
body=aas_def.split('typedef struct aas_routingupdate_s {',1)[1].split('} aas_routingupdate_t;',1)[0]
assert declared_members(body)=={'cluster','areanum','start','tmptraveltime','areatraveltimes','inlist','next','prev'}
world_body=aas_def.split('typedef struct aas_s {',1)[1].split('} aas_t;',1)[0]
world_scalars={'loaded','initialized','savefile','bspchecksum','time','numframes','filename','mapname','numreachabilityareas','reachabilitytime','maxentities','maxclients','frameroutingupdates'}
world_geometry={'numbboxes','bboxes','numvertexes','vertexes','numplanes','planes','numedges','edges','edgeindexsize','edgeindex','numfaces','faces','faceindexsize','faceindex','numareas','areas','numareasettings','areasettings','reachabilitysize','reachability','numnodes','nodes','numportals','portals','portalindexsize','portalindex','numclusters','clusters'}
world_spatial={'linkheap','linkheapsize','freelinks','arealinkedentities','entities'}
world_routing={'travelflagfortype','areacontentstravelflags','areaupdate','portalupdate','reversedreachability','areatraveltimes','clusterareacache','portalcache','oldestcache','newestcache','portalmaxtraveltimes','reachabilityareaindex','reachabilityareas'}
assert world_scalars | world_geometry | world_spatial | world_routing==declared_members(world_body)
print('PASS: AAS world/cache members have scalar, geometry, spatial, derived-routing or synchronous scratch ownership')
bsp_source=(ROOT/'engine/botlib/be_aas_bspq3.cpp').read_text()
for tag,name,expected in (('bsp_s','bsp_t',{'loaded','entdatasize','dentdata','numentities','entities'}),
                          ('bsp_entity_s','bsp_entity_t',{'epairs'}),
                          ('bsp_epair_s','bsp_epair_t',{'key','value','next'})):
    body=bsp_source.split('typedef struct '+tag+' {',1)[1].split('} '+name+';',1)[0]
    assert declared_members(body)==expected
print('PASS: BSP text and parsed entity-pair ownership is immutable and verified by content identity')

















for file in ('g_main','ai_main','ai_dmq3','g_bot','g_animation','g_data_weapons','g_rewind'):
    source=(ROOT/f'game/game/{file}.cpp').read_text()
    cvars=set()
    for declarations in re.findall(r'^(?:static )?vmCvar_t ([^;]+);',source,re.M):
        cvars.update(name.strip() for name in declarations.split(','))
    owner=source.split('static cvarTable_t gameCvarTable[]',1)[1].split('};',1)[0] if file=='g_main' else source.split('static const gCachedCvar_t saved',1)[1].split('};',1)[0]
    assert cvars <= set(re.findall(r'&([a-zA-Z_]\w*)',owner)), f'{file}: cached cvar lacks checkpoint ownership'
print('PASS: every persistent native-game cvar has named checkpoint ownership; handles remain process-local')
main_source=(ROOT/'game/game/g_main.cpp').read_text().split('static cvarTable_t gameCvarTable[]',1)[1].split('};',1)[0]
selected=set(re.findall(r'\{\s*(?:&\w+|NULL),\s*"([^"]+)"',main_source))
for file in ('ai_main','ai_dmq3','g_bot','g_animation','g_data_weapons','g_rewind'):
    bindings=(ROOT/f'game/game/{file}.cpp').read_text().split('static const gCachedCvar_t saved',1)[1].split('};',1)[0]
    selected.update(re.findall(r'"([^"]+)"',bindings))
extra=(ROOT/'game/game/g_state.cpp').read_text().split('static const char *const names[]',1)[1].split('};',1)[0]
selected.update(re.findall(r'"([^"]+)"',extra))
# Runtime process/filesystem context and the map identity are supplied by the
# coordinator; session/botsession and item-disable names are generated explicitly.
context={'cl_running','com_buildScript','mapname','sv_mapChecksum','fs_basepath','fs_game','fs_cdpath'}
for file in (ROOT/'game/game').glob('*.cpp'):
    source=file.read_text()
    reads=set(re.findall(r'trap_Cvar_(?:VariableIntegerValue|VariableStringBuffer|VariableValue|Set)\(\s*"([^"]+)"',source))
    reads.update(re.findall(r'trap_Cvar_Register\(\s*&\w+,\s*"([^"]+)"',source))
    assert reads <= selected|context, f'{file.name}: unowned game cvar names {reads-selected-context}'
print('PASS: literal native cvar reads/registrations have explicit saved or runtime-context ownership')


run([sys.executable, 'tools/replication.py', '--check'])
sha=args.output/'sha.o'
probe=args.output/'probe'
shared=args.output/'shared.o'
run([*shlex.split(args.cc),'-std=c99','-O2','-c','third_party/sha256/sha-256.c','-o',sha])
run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
     '-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections','-fsanitize=undefined',
     '-fno-sanitize-recover=all','-c','engine/qcommon/q_shared.cpp','-o',shared])
run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
     '-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections',
     '-fsanitize=undefined','-fno-sanitize-recover=all',
     'tests/probes/state_portals.cpp','engine/qcommon/state.cpp',sha,'-Wl,--gc-sections','-o',probe])
run([probe])
# Real botlib phases with owned empty configs, including normal setup/shutdown.
bot_sources=re.findall(r'^  (engine/botlib/[^ ]+\.cpp)$', (ROOT/'cmake/Sources.cmake').read_text(), re.M)
assert len(bot_sources)>20 and len(bot_sources)==len(set(bot_sources))
run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
     '-DBOTLIB','-ffunction-sections','-fdata-sections','-fsanitize=undefined',
     '-fno-sanitize-recover=all','tests/probes/state_bot_aggregate.cpp',
     *[path for path in bot_sources if not path.endswith('/l_log.cpp')],
     'engine/qcommon/q_math.cpp','engine/qcommon/state.cpp',shared,sha,
     '-Wl,--gc-sections','-o',probe])
run([probe])
for definitions in ([], ['-DSTATE_NATIVE_CACHE']):
    run([*shlex.split(args.cxx),*definitions,'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
         '-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections',
         '-fsanitize=undefined','-fno-sanitize-recover=all',
         'tests/probes/state_cvars.cpp','engine/qcommon/q_math.cpp',
         'engine/qcommon/state.cpp',shared,sha,'-Wl,--gc-sections','-o',probe])
    run([probe])

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
for component in ('callbacks','references','composed','utilities','cached_cvars','semantics'):
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

run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
     '-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections',
     '-fsanitize=undefined','-fno-sanitize-recover=all',
     'tests/probes/state_main_cvars.cpp','engine/qcommon/state.cpp',sha,native,
     '-Wl,--gc-sections','-o',probe])
run([probe])

for component in ('ITEMS','FILTERS','MEMORY'):
    run([*shlex.split(args.cxx),f'-DSTATE_{component}','-std=c++20','-O2','-fno-exceptions','-fno-rtti',
         '-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections',
         '-fsanitize=undefined','-fno-sanitize-recover=all',
         'tests/probes/state_native_owners.cpp','engine/qcommon/state.cpp',sha,
         '-Wl,--gc-sections','-o',probe])
    run([probe])

for component in ('COMBAT','TEAM','PODIUM'):
    run([*shlex.split(args.cxx),f'-DSTATE_{component}','-std=c++20','-O2','-fno-exceptions','-fno-rtti',
         '-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections',
         '-fsanitize=undefined','-fno-sanitize-recover=all',
         'tests/probes/state_globals.cpp','engine/qcommon/state.cpp',sha,native,
         '-Wl,--gc-sections','-o',probe])
    run([probe])

for definitions in ([],['-DAFTERSHOCK_DEVTOOLS']):
    run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
         '-Wall','-Wextra','-Werror',*definitions,'-ffunction-sections','-fdata-sections',
         '-fsanitize=undefined','-fno-sanitize-recover=all',
         'tests/probes/state_definitions.cpp','engine/entities/entities.cpp','engine/qcommon/state.cpp',sha,
         '-Wl,--gc-sections','-o',probe])
    run([probe])

for component in ('QUEUE','CLOCK','TEAM','CONTENT'):
    run([*shlex.split(args.cxx),f'-DSTATE_{component}','-std=c++20','-O2','-fno-exceptions','-fno-rtti',
         '-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections',
         '-fsanitize=undefined','-fno-sanitize-recover=all',
         'tests/probes/state_bot_globals.cpp','engine/qcommon/state.cpp',sha,
         '-Wl,--gc-sections','-o',probe])
    run([probe])

# Optional legacy mission-pack code has writable string literals and unused parameters.
for definitions in ([],['-DMISSIONPACK','-Wno-write-strings','-Wno-unused-parameter']):
    run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
         '-Wall','-Wextra','-Werror',*definitions,'-ffunction-sections','-fdata-sections',
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

for component in ('input','move','weights','weight_prepare','characters','chat_queue','chat_content','chat_prepare','libvars','interface','aas_entities','aas_links','aas_world','aas_settings','aas_routing','bsp_content','parser'):
    run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
         '-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections',
         '-fsanitize=undefined','-fno-sanitize-recover=all',
         f'tests/probes/state_bot_{component}.cpp',
         *(['engine/botlib/l_script.cpp'] if component in ('parser','weight_prepare','chat_prepare') else []),
         *(['engine/botlib/l_precomp.cpp'] if component in ('weight_prepare','chat_prepare') else []),
         'engine/qcommon/state.cpp',sha,
         '-Wl,--gc-sections','-o',probe])
    run([probe])

for component in ('goal','weapon'):
    run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
         '-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections',
         '-fsanitize=undefined','-fno-sanitize-recover=all',
         f'tests/probes/state_bot_{component}_prepare.cpp','engine/botlib/be_ai_weight.cpp',
         'engine/botlib/l_script.cpp','engine/botlib/l_precomp.cpp',
         'engine/qcommon/state.cpp',sha,'-Wl,--gc-sections','-o',probe])
    run([probe])

for component in ('goals','weapons','goal_map'):
    run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
         '-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections',
         '-fsanitize=undefined','-fno-sanitize-recover=all',
         f'tests/probes/state_bot_{component}.cpp','engine/botlib/be_ai_weight.cpp','engine/qcommon/state.cpp',sha,
         '-Wl,--gc-sections','-o',probe])
    run([probe])
