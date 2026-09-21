#!/usr/bin/env python3
"""Check discoverable authored-format schemas and actionable validation errors."""
import copy
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from jsonschema import Draft202012Validator
from run import ROOT

invalid = {'level': ('version', 0), 'weapon': ('interval_ms', 0),
           'animation': ('states', []), 'material': ('alphaMode', 'invalid'),
           'effect': ('emitters', []), 'match-spec': ('players', 0)}
with tempfile.TemporaryDirectory(prefix='aftershock-formats-', dir=os.environ.get('AFTERSHOCK_SCRATCH')) as temporary:
    for kind, (key, value) in invalid.items():
        description = subprocess.run([sys.executable, 'tools/agent', 'describe', kind], cwd=ROOT,
                                     capture_output=True, text=True)
        assert description.returncode == 0, description.stderr
        contract = json.loads(description.stdout)
        Draft202012Validator.check_schema(contract['schema'])
        Draft202012Validator(contract['schema']).validate(contract['example'])
        source = Path(temporary)/(kind+'.json')
        source.write_text(json.dumps(contract['example']))
        command = [sys.executable, 'tools/agent', 'validate', kind, str(source)]
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
        changed = copy.deepcopy(contract['example'])
        changed[key] = value
        source.write_text(json.dumps(changed))
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        assert result.returncode == 1, result.stdout
        error = json.loads(result.stderr)['error']
        assert error['file'] == str(source) and key in error['path'] and error['hint'], error
        if kind in ('weapon', 'animation', 'material', 'level'):
            if kind == 'level':
                command = [sys.executable, 'tools/level', str(source), '--map-only', '--output', str(Path(temporary)/'level-output')]
            else:
                project = Path(temporary)/'assets.json'
                project.write_text(json.dumps(dict(version=1,assets=[dict(name='test/'+kind,kind=kind,source=source.name)])))
                command = [sys.executable, 'tools/cook', str(project), '--output', str(Path(temporary)/'cook-output')]
            result = subprocess.run(command,cwd=ROOT,capture_output=True,text=True)
            assert result.returncode == 1, result.stdout
            error = json.loads(result.stderr)['error']
            assert error['file'] == str(source) and key in error['path'] and error['hint'], error
print('PASS: six authored schemas, minimal examples and file/path/type/range diagnostics')

# Compare the schema's structural/range rules with the production authoring
# loaders. Source/resource relationships remain the loaders' semantic checks.
sys.path.insert(0, str(ROOT/'tools/cook'))
import weapon
import animation
import model
sys.path.insert(0, str(ROOT/'tools/level'))
from validate import validate as level_validate
sys.path.insert(0, str(ROOT))
from tools.agent.formats import describe, validate
import shutil


def agrees(kind, document, loader, expected):
    schema_ok = Draft202012Validator(describe(kind)['schema']).is_valid(document)
    try:
        loader(document)
        loader_ok = True
    except (ValueError, KeyError, TypeError, OverflowError):
        loader_ok = False
    assert schema_ok == loader_ok == expected, (kind, document, schema_ok, loader_ok, expected)


with tempfile.TemporaryDirectory(prefix='aftershock-format-loaders-', dir=os.environ.get('AFTERSHOCK_SCRATCH')) as temporary:
    root = Path(temporary)
    weapon_example = describe('weapon')['example']
    weapon_loader = lambda value: weapon.cook(root/'weapon.json', 'weapons/test', lambda _: json.dumps(value).encode())
    agrees('weapon', weapon_example, weapon_loader, True)
    for field, values in {'interval_ms': [19,20,60000,60001], 'magazine':[0,1,1000,1001],
                          'spread_degrees':[-1,0,90,91], 'ads_fov':[0,1,179,180]}.items():
        for index, value in enumerate(values):
            agrees('weapon', dict(weapon_example, **{field:value}), weapon_loader, index in (1,2))
    for field in ('time_ms','event','cancel'):
        changed = copy.deepcopy(weapon_example)
        del changed['reload'][0][field]
        agrees('weapon', changed, weapon_loader, False)
    level_example = describe('level')['example']
    level_loader = lambda value: level_validate(value, ROOT/'tests/assets/levels/assets')
    agrees('level', level_example, level_loader, True)
    for value in (0,1,255,256):
        changed = copy.deepcopy(level_example)
        changed['lighting']['ambient'] = value
        agrees('level', changed, level_loader, value<=255)
    for value in (63,64,8192,8193):
        changed = copy.deepcopy(level_example)
        changed['rooms'][0]['size'][2] = value
        # Use a sufficient design limit so only the size boundary is under test.
        changed['rules']['max_sightline'] = 64000
        agrees('level', changed, level_loader, 64<=value<=8192)
    for source in (ROOT/'tests/assets/animation').iterdir():
        if source.is_file():
            (root/source.name).symlink_to(source)
    animation_example = describe('animation')['example']
    project = json.loads((ROOT/'tests/assets/animation/assets.json').read_text())
    models = [dict(row, _source=(root/row['source']).resolve()) for row in project['assets'] if row['kind']=='model']
    def animation_loader(value):
        source = root/'example.animation.json'
        source.write_text(json.dumps(value))
        return animation.cook(source, 'animations/test', {}, lambda path:path.read_bytes(), models)
    agrees('animation', animation_example, animation_loader, True)
    for value in (0,1/65536,16,17):
        changed = copy.deepcopy(animation_example)
        changed['states'][0]['speed'] = value
        agrees('animation', changed, animation_loader, 0<value<=16)
    material_example = describe('material')['example']
    def material_loader(value):
        return model.cook_material(root/'material.json', 'materials/test', lambda _:json.dumps(value).encode(),
                                   {'material_model':'metallic-roughness'})
    agrees('material', material_example, material_loader, True)
    for value in (-1,0,1,2):
        changed = dict(material_example, pbrMetallicRoughness={'roughnessFactor':value})
        agrees('material', changed, material_loader, 0<=value<=1)
    for kind, paths in {'level':['tests/assets/levels/two_lane.json'],
                        'weapon':['tests/assets/weapons/rifle.weapon.json'],
                        'animation':['tests/assets/animation/body.animation.json','tests/assets/animation/rifle.animation.json',
                                     'tests/assets/weapons/rifle.animation.json']}.items():
        for path in paths:
            validate(kind,json.loads((ROOT/path).read_text()),path)
    # Compile only the real, stdlib-only match-spec source and a tiny JSON driver.
    # This checks Go's integer/range/unknown-field rules without a match server.
    shutil.copyfile(ROOT/'tools/match/spec.go',root/'spec.go')
    (root/'probe.go').write_text('''package main
import ("encoding/json"; "os")
func main() {
 var inputs []json.RawMessage
 if err := json.NewDecoder(os.Stdin).Decode(&inputs); err != nil { panic(err) }
 results := []bool{}
 for _, input := range inputs { _, err := decodeSpec(input); results = append(results, err == nil) }
 if err := json.NewEncoder(os.Stdout).Encode(results); err != nil { panic(err) }
}
''')
    subprocess.run(['go','build','-o',str(root/'match-spec'),'spec.go','probe.go'],cwd=root,check=True)
    example = describe('match-spec')['example']
    cases = [example]
    for field, values in {'players':[0,1,64,65], 'mode':[-1,0,4,5], 'frag_limit':[-1,0,10000,10001],
                          'time_limit':[-1,0,1440,1441], 'map':['a','a'*48,'a'*49], 'password':['short','abcdefgh']}.items():
        cases += [dict(example,**{field:value}) for value in values]
    cases += [dict(example,frag_limit=0,time_limit=0),dict(example,unknown=True)]
    result = subprocess.run([root/'match-spec'],input=json.dumps(cases),text=True,capture_output=True,check=True)
    validator = Draft202012Validator(describe('match-spec')['schema'])
    assert json.loads(result.stdout) == [validator.is_valid(value) for value in cases]
print('PASS: schema boundaries agree with production weapon, level, animation, PBR and Go match loaders')
