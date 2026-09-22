#!/usr/bin/env python3
"""Check the native HTTPS transport against a private TLS endpoint."""
import argparse
import http.server
from pathlib import Path
import shlex
import ssl
import subprocess
import threading
from run import ROOT, SCRATCH, run

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx',default='g++')
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-backend-http')
args=parser.parse_args()
args.output=args.output.resolve()
args.output.mkdir(parents=True,exist_ok=True)
cert,key=args.output/'cert.pem',args.output/'key.pem'
run(['openssl','req','-x509','-newkey','rsa:2048','-nodes','-days','1','-subj','/CN=127.0.0.1',
     '-addext','subjectAltName=IP:127.0.0.1','-keyout',key,'-out',cert],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
key.chmod(0o600)
redirected=[]
class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self,*args): pass
    def handle(self):
        try: super().handle()
        except (BrokenPipeError,ConnectionResetError,ssl.SSLEOFError): pass
    def do_GET(self):
        if self.path=='/redirect':
            self.send_response(302);self.send_header('Location','/unexpected');self.send_header('Content-Length','0');self.end_headers()
        elif self.path=='/large':
            self.send_response(200);self.send_header('Content-Length','40000');self.end_headers()
            try:self.wfile.write(b'x'*40000)
            except (BrokenPipeError,ConnectionResetError):pass
        else:
            redirected.append(self.path)
            self.send_response(403);self.end_headers()
    def do_POST(self):
        data=self.rfile.read(int(self.headers.get('Content-Length','0')))
        valid=self.headers.get('Authorization')=='Bearer aabb' and self.headers.get('Content-Type')=='application/json' and data==b'{"test":1}'
        self.send_response(200 if valid else 400);self.send_header('Content-Length','11');self.end_headers();self.wfile.write(b'{"ok":true}')
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler)
context=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);context.load_cert_chain(cert,key)
server.socket=context.wrap_socket(server.socket,server_side=True)
thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
try:
    for name,flags,libraries in [('linked',['-DUSE_CURL'],['-lcurl']),
                                  ('loaded',['-DUSE_CURL','-DUSE_CURL_DLOPEN'],['-ldl']),
                                  ('unavailable',[],[])]:
        binary=args.output/name
        run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti','-fsanitize=undefined',
             '-fno-sanitize-recover=all',*flags,'-ffunction-sections','-fdata-sections',
             'tests/probes/backend_http.cpp','engine/platform/sys_http.cpp','engine/platform/sys_curl.cpp',
             '-Wl,--gc-sections',*libraries,'-pthread','-o',binary])
        run([binary,f'https://127.0.0.1:{server.server_port}',cert],timeout=25)
    assert not redirected,redirected
finally:
    server.shutdown();server.server_close();thread.join()
