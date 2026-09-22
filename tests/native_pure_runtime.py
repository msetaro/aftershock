#!/usr/bin/env python3
"""A normal native client joins a pure server using matching installed content."""
import argparse
import os
from pathlib import Path
from run import SCRATCH
import signal
import socket
import subprocess
import tempfile
import time
from run import ROOT,content_maps,content_settings

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--client',type=Path,required=True)
parser.add_argument('--server',type=Path,required=True)
parser.add_argument('--package',type=Path,action='append',default=[],help='owned package mounts; must include package-pure.cfg')
parser.add_argument('--content',choices=['quake3','openarena'],default='quake3')
parser.add_argument('--data',type=Path,default=Path.home()/'.q3a/baseq3')
parser.add_argument('--output',type=Path,default=(SCRATCH / 'aftershock-native-pure-runtime'))
args=parser.parse_args()
args.output.mkdir(parents=True,exist_ok=True)
paks=sorted(args.data.resolve().glob('*.pk3'));assert paks,'installed content is required'
icds=list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'));assert len(icds)==1
with tempfile.TemporaryDirectory(prefix='aftershock-native-pure-') as temporary:
    home=Path(temporary);base=home/('baseoa' if args.content=='openarena' else 'baseq3');base.mkdir()
    for pak in paks:(base/pak.name).symlink_to(pak)
    for index, package in enumerate(args.package):
        (base/f'{index:02d}-{package.name}').symlink_to(package.resolve())
    with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as reservation:
        reservation.bind(('127.0.0.1',0));port=reservation.getsockname()[1]
    common=['+set','fs_basepath',str(home),*content_settings(args.content),'+set','net_enabled','1','+set','net_ip','127.0.0.1']
    environment=dict(os.environ,LC_ALL='C',LP_NUM_THREADS='1',VK_DRIVER_FILES=str(icds[0]),VK_ICD_FILENAMES=str(icds[0]))
    server_log=args.output/'server.log';client_log=args.output/'client.log'
    with server_log.open('w') as out:
        server=subprocess.Popen([str(args.server.resolve()),*common,'+set','fs_homepath',str(home/'server'),
            '+set','net_port',str(port),'+set','dedicated','1','+set','bot_enable','0','+set','sv_pure','1',
            '+set','g_password','pure-match-password',*(['+exec','package-pure.cfg'] if args.package else []),'+map',content_maps(args.content)[0]],
            cwd=ROOT,env=environment,stdin=subprocess.PIPE,stdout=out,stderr=subprocess.STDOUT,text=True)
        client=None
        try:
            deadline=time.monotonic()+15
            while 'Static game loaded.' not in server_log.read_text():
                assert server.poll() is None and time.monotonic()<deadline,'server load failed'
                time.sleep(.1)
            package_command = 'exec package-pure.cfg\n' if args.package else ''
            (base/'pure-client.cfg').write_text(f'connect 127.0.0.1:{port}\nwait 200\n{package_command}say native_pure_ready\nwait 20\nquit\n')
            with client_log.open('w') as output:
                client=subprocess.Popen(['xvfb-run','-a',str(args.client.resolve()),*common,
                    '+set','fs_homepath',str(home/'client'),'+set','net_port','0','+set','password','pure-match-password',
                    '+set','r_mode','3','+set','r_fullscreen','0','+set','s_initsound','0',
                    '+set','cl_allowDownload','0','+set','cl_autoRecordDemo','0','+set','com_maxfps','20',
                    '+exec','pure-client.cfg'],cwd=ROOT,env=environment,stdout=output,stderr=subprocess.STDOUT,start_new_session=True)
                assert client.wait(timeout=40)==0
            text=server_log.read_text()
            assert 'ClientBegin: 0' in text and 'native_pure_ready' in text,'native client was not accepted by pure server'
            assert 'Unpure client' not in text
            assert 'Static cgame loaded.' in client_log.read_text()
            if args.package:
                assert 'package_pure_mounted' in text and 'package_pure_mounted' in client_log.read_text(), 'package content must load on both peers under sv_pure'
        finally:
            if client is not None and client.poll() is None:
                os.killpg(client.pid,signal.SIGTERM)
                try:client.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    os.killpg(client.pid,signal.SIGKILL);client.wait(timeout=5)
            if server.poll() is None:
                server.stdin.write('quit\n');server.stdin.flush()
                try:server.wait(timeout=5)
                except subprocess.TimeoutExpired:server.kill();server.wait(timeout=5)
print('PASS: matching native client joins sv_pure=1, enters play and sends a gameplay message')
