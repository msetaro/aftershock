#!/usr/bin/env python3
"""Exercise resize and hidden-window rendering on an isolated Xvfb display."""
import argparse
import ctypes
import ctypes.util
import os
from pathlib import Path
from run import SCRATCH
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import time

from run import ROOT, content_maps, content_settings


def wait_for(predicate, process, seconds=30):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        if predicate():
            return
        if process.poll() is not None:
            raise RuntimeError('client exited before the lifecycle check completed')
        time.sleep(0.05)
    raise RuntimeError('client lifecycle check timed out')


class XInput:
    """Real key input, used only inside a private Xvfb invocation."""
    def __init__(self):
        self.x11 = ctypes.CDLL(ctypes.util.find_library('X11'))
        self.xt = ctypes.CDLL(ctypes.util.find_library('Xtst'))
        self.x11.XOpenDisplay.argtypes = [ctypes.c_char_p]
        self.x11.XOpenDisplay.restype = ctypes.c_void_p
        self.x11.XCloseDisplay.argtypes = [ctypes.c_void_p]
        self.x11.XSync.argtypes = [ctypes.c_void_p, ctypes.c_int]
        self.x11.XStringToKeysym.argtypes = [ctypes.c_char_p]
        self.x11.XStringToKeysym.restype = ctypes.c_ulong
        self.x11.XKeysymToKeycode.argtypes = [ctypes.c_void_p, ctypes.c_ulong]
        self.x11.XKeysymToKeycode.restype = ctypes.c_ubyte
        self.xt.XTestFakeKeyEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_int, ctypes.c_ulong]
        self.display = self.x11.XOpenDisplay(None)
        assert self.display

    def key_event(self, name, down):
        code = self.x11.XKeysymToKeycode(self.display, self.x11.XStringToKeysym(name.encode()))
        assert code
        self.xt.XTestFakeKeyEvent(self.display, code, int(down), 0)
        self.x11.XSync(self.display, 0)
        time.sleep(0.06)

    def key(self, name):
        self.key_event(name, True)
        self.key_event(name, False)

    def verify_window(self, process):
        tree = subprocess.check_output(['xwininfo', '-root', '-tree'], text=True)
        candidates = re.findall(r'^\s*(0x[0-9a-fA-F]+).*640x480', tree, re.M)
        owned = [window for window in candidates if re.search(
            r'=\s*' + str(process.pid) + r'\s*$',
            subprocess.check_output(['xprop', '-id', window, '_NET_WM_PID'], text=True))]
        assert len(owned) == 1, 'expected one window owned by the launched client'

    def close(self):
        self.x11.XCloseDisplay(self.display)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
    parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
    parser.add_argument('--output', type=Path, default=(SCRATCH / 'aftershock-window-tests'))
    parser.add_argument('--inside-xvfb', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    if not args.inside_xvfb:
        subprocess.run(['timeout', '90', 'xvfb-run', '-a', sys.executable, str(Path(__file__).resolve()),
                        *sys.argv[1:], '--inside-xvfb'], cwd=ROOT, check=True)
        return
    args.output.mkdir(parents=True, exist_ok=True)
    binary = args.binary.resolve()
    paks = sorted(args.data.resolve().glob('*.pk3'))
    if not binary.is_file() or not paks:
        parser.error('an existing client build and installed content are required')
    icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
    if len(icds) != 1:
        parser.error('exactly one installed lavapipe ICD is required')
    env = dict(os.environ, LIBGL_ALWAYS_SOFTWARE='1', GALLIUM_DRIVER='llvmpipe', LP_NUM_THREADS='1',
               VK_ICD_FILENAMES=str(icds[0]), VK_DRIVER_FILES=str(icds[0]))
    x11 = ctypes.CDLL(ctypes.util.find_library('X11'))
    display_type, window_type = ctypes.c_void_p, ctypes.c_ulong
    x11.XOpenDisplay.argtypes = [ctypes.c_char_p]
    x11.XOpenDisplay.restype = display_type
    x11.XCloseDisplay.argtypes = [display_type]
    x11.XSync.argtypes = [display_type, ctypes.c_int]
    x11.XResizeWindow.argtypes = [display_type, window_type, ctypes.c_uint, ctypes.c_uint]
    x11.XUnmapWindow.argtypes = [display_type, window_type]
    x11.XMapWindow.argtypes = [display_type, window_type]
    display = x11.XOpenDisplay(None)
    if not display:
        raise RuntimeError('cannot open the private Xvfb display')
    log_path = args.output / 'window.log'
    process = None
    try:
        with tempfile.TemporaryDirectory(prefix='aftershock-window-') as temporary:
            home = Path(temporary)
            base = home / ('baseoa' if args.content == 'openarena' else 'baseq3')
            (base / 'demos').mkdir(parents=True)
            for pak in paks:
                (base / pak.name).symlink_to(pak)
            map_name = content_maps(args.content)[0]
            golden = ROOT / 'tests/golden' / ('openarena' if args.content == 'openarena' else '')
            shutil.copyfile(golden / (map_name + '.dm_68'), base / 'demos' / (map_name + '.dm_68'))
            command = [binary, '+set', 'fs_basepath', home, '+set', 'fs_homepath', home,
                       *content_settings(args.content), '+set', 'r_fullscreen', '0', '+set', 'r_mode', '3',
                       '+set', 'r_fbo', '1', '+set', 's_initsound', '0', '+set', 'sv_pure', '0',
                       '+set', 'net_ip', '127.0.0.1', '+set', 'com_maxfps', '20',
                       '+set', 'com_maxfpsUnfocused', '20', '+set', 'com_logfile', '0',
                       '+set', 'cl_autoRecordDemo', '0', '+set', 'timedemo', '0',
                       '+demo', map_name, '+wait', '80', '+screenshot', 'hidden',
                       '+wait', '100', '+screenshot', 'restored', '+wait', '2', '+vkinfo', '+quit']
            with log_path.open('wb') as log:
                process = subprocess.Popen([str(arg) for arg in command], cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT)
                wait_for(lambda: b'CL_InitCGame:' in log_path.read_bytes(), process)
                tree = subprocess.check_output(['xwininfo', '-root', '-tree'], text=True)
                candidates = re.findall(r'^\s*(0x[0-9a-fA-F]+).*640x480', tree, re.M)
                owned = []
                for candidate in candidates:
                    pid = subprocess.check_output(['xprop', '-id', candidate, '_NET_WM_PID'], text=True)
                    if re.search(r'=\s*' + str(process.pid) + r'\s*$', pid):
                        owned.append(int(candidate, 16))
                if len(owned) != 1:
                    raise RuntimeError('expected one 640x480 window owned by the launched client: ' + tree)
                window = owned[0]
                x11.XResizeWindow(display, window, 800, 600)
                x11.XSync(display, 0)
                time.sleep(0.5)
                dimensions = subprocess.check_output(['xwininfo', '-id', hex(window)], text=True)
                if 'Width: 800' not in dimensions or 'Height: 600' not in dimensions:
                    raise RuntimeError('window resize was not applied')
                x11.XResizeWindow(display, window, 640, 480)
                x11.XSync(display, 0)
                time.sleep(0.5)
                x11.XUnmapWindow(display, window)
                x11.XSync(display, 0)
                state = subprocess.check_output(['xwininfo', '-id', hex(window)], text=True)
                if 'Map State: IsUnMapped' not in state:
                    raise RuntimeError('window did not enter the hidden state')
                wait_for(lambda: (base / 'screenshots/hidden.tga').exists(), process)
                x11.XMapWindow(display, window)
                x11.XSync(display, 0)
                state = subprocess.check_output(['xwininfo', '-id', hex(window)], text=True)
                if 'Map State: IsViewable' not in state:
                    raise RuntimeError('window did not restore')
                process.wait(timeout=30)
                if process.returncode:
                    raise RuntimeError('client failed; see ' + str(log_path))
            text = log_path.read_text()
            if 'ERROR:' in text or 'Signal caught' in text or 'restarting swapchain' not in text:
                raise RuntimeError('resize did not complete a clean swapchain restart; see ' + str(log_path))
            for name in ('hidden', 'restored'):
                screenshot = base / 'screenshots' / (name + '.tga')
                data = screenshot.read_bytes()
                if struct.unpack_from('<HH', data, 12) != (640, 480):
                    raise RuntimeError('unexpected screenshot dimensions: ' + name)
                shutil.copyfile(screenshot, args.output / screenshot.name)
            print('PASS: owned-window resize/swapchain restart, hidden FBO capture and restored capture')
    finally:
        if process and process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=5)
        x11.XCloseDisplay(display)


if __name__ == '__main__':
    main()
