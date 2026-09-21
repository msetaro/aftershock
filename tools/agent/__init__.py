"""Local, development-only Aftershock control client."""
import json
import os
from pathlib import Path
import queue
import signal
import subprocess
import tempfile
import threading

from tools.scratch import ROOT as SCRATCH

ROOT = Path(__file__).resolve().parents[2]


class Engine:
    """One private engine, home directory and optional headless display."""

    def __init__(self, binary, data=None, content='quake3', *, headless=True, arguments=(), home=None):
        self.temporary = tempfile.TemporaryDirectory(prefix='aftershock-agent-', dir=SCRATCH)
        self.root = Path(self.temporary.name)
        self.home = Path(home).resolve() if home else self.root/'home'
        self.game = 'baseoa' if content == 'openarena' else 'baseq3'
        self.base = self.home/self.game
        self.base.mkdir(parents=True, exist_ok=True)
        paks = sorted(Path(data or Path.home()/'.q3a/baseq3').resolve().glob('*.pk3'))
        if not paks:
            self.temporary.cleanup()
            raise ValueError('installed game content is required')
        for pak in paks:
            target = self.base/pak.name
            if not target.exists():
                target.symlink_to(pak)
        self.log_path = self.root/'engine.log'
        self.log = self.log_path.open('ab', buffering=0)
        command = [str(Path(binary).resolve()), '--agent', '+set', 'fs_basepath', str(self.home),
                   '+set', 'fs_homepath', str(self.home), '+set', 'fs_game', self.game,
                   '+set', 'net_enabled', '0', '+set', 'sv_pure', '0', '+set', 's_initsound', '0',
                   '+set', 'r_fullscreen', '0', '+set', 'r_mode', '3', '+set', 'r_swapInterval', '0',
                   '+set', 'cl_autoRecordDemo', '0', '+set', 'com_introplayed', '1',
                   '+set', 'com_skipIdLogo', '1', *map(str, arguments)]
        env = dict(os.environ)
        if headless and os.name != 'nt':
            # Older xvfb-run merges child stderr into stdout. Redirect inside it.
            command = ['xvfb-run', '-a', 'sh', '-c',
                       'agent_log_path=$1; shift; exec "$@" 2>>"$agent_log_path"',
                       'aftershock-agent', str(self.log_path), *command]
            if not env.get('VK_DRIVER_FILES'):
                icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
                if len(icds) != 1:
                    self.log.close()
                    self.temporary.cleanup()
                    raise RuntimeError('select a Vulkan driver with VK_DRIVER_FILES')
                env.update(VK_DRIVER_FILES=str(icds[0]), VK_ICD_FILENAMES=str(icds[0]), LP_NUM_THREADS='1')
        self.process = subprocess.Popen(command, cwd=ROOT, env=env, stdin=subprocess.PIPE,
                                        stdout=subprocess.PIPE, stderr=self.log, text=True,
                                        encoding='utf-8', bufsize=1, start_new_session=os.name != 'nt')
        self.replies = queue.Queue()
        self.sequence = 0
        self.events = []
        self.reader = threading.Thread(target=self._read, daemon=True)
        self.reader.start()

    def _read(self):
        try:
            for line in self.process.stdout:
                try:
                    self.replies.put(json.loads(line))
                except ValueError as error:
                    raise ValueError(f'invalid channel line: {line[:512]!r}') from error
        except (ValueError, OSError) as error:
            self.replies.put(error)
        finally:
            self.replies.put(EOFError('engine closed its response pipe'))

    def request(self, op, *, timeout=120, **fields):
        self.sequence += 1
        self.process.stdin.write(json.dumps(dict(id=self.sequence, op=op, **fields))+'\n')
        self.process.stdin.flush()
        while True:
            try:
                reply = self.replies.get(timeout=timeout)
            except queue.Empty as error:
                raise TimeoutError(f'{op}: no reply; {self.log_path.read_text(errors="replace")[-2000:]}') from error
            if isinstance(reply, Exception):
                raise RuntimeError(f'{op}: {reply}; {self.log_path.read_text(errors="replace")[-2000:]}') from reply
            if 'event' in reply:
                self.events.append(reply)
                continue
            if reply.get('id') != self.sequence:
                raise RuntimeError(f'{op}: out-of-order response {reply}')
            if not reply.get('ok'):
                raise ValueError(reply['error'])
            return reply['result']

    def step(self, frames=1):
        return self.request('step', frames=frames)

    def close(self):
        try:
            self.process.stdin.close()
            self.process.wait(timeout=20)
        except (BrokenPipeError, subprocess.TimeoutExpired):
            if os.name == 'nt':
                self.process.kill()
            else:
                os.killpg(self.process.pid, signal.SIGKILL)
            self.process.wait()
        finally:
            self.reader.join(timeout=2)
            self.process.stdout.close()
            self.log.close()
            self.temporary.cleanup()

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()
