#!/usr/bin/env python3
"""Check public subsystem includes and platform/filesystem ownership of OS calls."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
TOKENS = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
INCLUDE = re.compile(r'^[ \t]*#[ \t]*include\s*([<"])([^>"\n]+)[>"]', re.M)
OS_CALLS = set('''dlopen dlsym dlclose mmap munmap mprotect getenv setenv putenv unsetenv
system popen pclose fork execv execve execl waitpid fopen freopen fclose fread fwrite
fseek ftell fflush fprintf vfprintf fputs fgets fgetc fputc remove rename stat lstat
mkdir rmdir unlink open close read write access chmod umask socket bind listen accept
connect send recv sendto recvfrom select poll setsockopt getsockopt getaddrinfo freeaddrinfo
gethostbyname gethostname ioctl ioctlsocket closesocket gettimeofday clock_gettime time
localtime ctime getauxval sleep usleep nanosleep SDL_Init SDL_Quit'''.split())
OS_CALLS.update('''CreateFileA CreateFileW ReadFile WriteFile CloseHandle CreateProcessA
CreateProcessW CreateNamedPipeA ConnectNamedPipe DisconnectNamedPipe FlushFileBuffers
WaitForSingleObject QueryPerformanceCounter QueryPerformanceFrequency GetLastError
MessageBoxA MessageBoxW OutputDebugString OutputDebugStringA OutputDebugStringW GetStdHandle ShowWindow DebugBreak LoadLibraryA LoadLibraryW GetProcAddress FreeLibrary
VirtualAlloc VirtualFree VirtualProtect GetEnvironmentVariableA GetEnvironmentVariableW'''.split())
CALL = re.compile(r'\b(' + '|'.join(sorted(OS_CALLS)) + r'|SDL_\w+)\s*\(')
OS_HEADER = re.compile(r'^(?:sys/|netinet/|arpa/|X11/|SDL|windows\.h$|winsock2?\.h$|unistd\.h$|dlfcn\.h$|pthread\.h$|dirent\.h$)')


def blank(text):
    return ''.join('\n' if c == '\n' else ' ' for c in text)


def code(text, strings=False):
    text = TOKENS.sub(lambda m: blank(m[0]) if strings or m[0].startswith('/') else m[0], text)
    # Disabled examples are not executable. All platform/configuration branches
    # other than literal #if 0 are checked, including configurations not built here.
    stack = []
    active = True
    lines = []
    for line in text.splitlines(keepends=True):
        directive = re.match(r'\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)', line)
        visible = active
        if directive:
            op, condition = directive.groups()
            if op in ('if', 'ifdef', 'ifndef'):
                disabled = op == 'if' and condition.strip() == '0'
                stack.append((active, disabled))
                active = active and not disabled
            elif op in ('else', 'elif') and stack:
                parent, disabled = stack[-1]
                active = parent
            elif op == 'endif' and stack:
                active, _ = stack.pop()
        lines.append(line if visible else blank(line))
    return ''.join(lines)


def check(path, text):
    errors = []
    subsystem = path.parts[:2]
    platform = path.parts[:2] == ('engine', 'platform')
    gpu_backend = path.parts[:2] == ('engine', 'renderervk')
    filesystem = path == Path('engine/qcommon/files.cpp')
    source = code(text)
    for match in INCLUDE.finditer(source):
        kind, name = match.groups()
        reason = None
        if not (platform or filesystem) and OS_HEADER.match(name):
            reason = 'OS header belongs in platform or files.cpp'
        if not (platform or gpu_backend) and (name == 'vulkan.h' or name.startswith('vulkan/')):
            reason = 'Vulkan SDK belongs in its backend or platform'
        if kind == '"':
            target = (ROOT / path.parent / name).resolve()
            if not target.is_relative_to(ROOT):
                reason = 'include escapes the source tree'
            else:
                target = target.relative_to(ROOT)
                if target.parts[:2] == ('third_party', 'vulkan') and not (platform or gpu_backend):
                    reason = 'Vulkan SDK belongs in its backend or platform'
                if path.parts[0] == 'engine' and target.parts[0] == 'game':
                    reason = 'engine must not include game'
                elif target.parts[0] in ('engine', 'game') and target.parts[:2] != subsystem:
                    public = target.name.endswith('_public.h') or target in (Path('engine/qcommon/q_shared.h'), Path('game/bg/q_shared.h'))
                    if not public:
                        reason = 'cross-subsystem include requires a public header'
                elif target.parts[0] not in ('engine', 'game', 'third_party'):
                    reason = 'include is outside the source layout'
        if reason:
            errors.append((source.count('\n', 0, match.start()) + 1, reason + ': ' + name))
    if not (platform or filesystem):
        source = code(text, strings=True)
        for match in CALL.finditer(source):
            before = source[:match.start()].rstrip()
            if before.endswith(('->', '.')):
                continue  # Codec/read/write callbacks are not OS entry points.
            errors.append((source.count('\n', 0, match.start()) + 1, 'OS call belongs in platform or files.cpp: ' + match[1]))
    return errors


def selfcheck():
    core = Path('engine/client/probe.cpp')
    assert check(core, '#include "../server/server.h"')
    assert check(core, '#include "../../game/bg/q_shared.h"')
    assert check(Path('game/game/probe.cpp'), '#include "../../engine/client/client.h"')
    assert not check(core, '#include "../sound/snd_public.h"')
    assert check(core, '#include <vulkan/vulkan.h>')
    assert check(Path('engine/renderercommon/probe.h'), '#include "../../third_party/vulkan/vulkan.h"')
    assert not check(Path('engine/renderervk/probe.cpp'), '#include "../../third_party/vulkan/vulkan.h"')
    assert not check(Path('engine/platform/probe.cpp'), '#include <vulkan/vulkan.h>')
    assert check(core, '#include <windows.h>\nCreateFileA("x");')
    assert check(core, '#ifdef _WIN32\nsocket(0);\n#else\nfopen("x", "r");\n#endif')
    assert check(core, '#if 0\n#if X\nfopen("x", "r");\n#endif\n#else\ngetenv("x");\n#endif')
    assert not check(core, '// fopen("x");\nconst char *s = "socket(0)";\ncodec -> read(0);\n#if 0\nfopen("x", "r");\n#endif')
    assert not check(Path('engine/platform/probe.cpp'), 'socket(0);')
    assert not check(Path('engine/qcommon/files.cpp'), 'fopen("x", "r");')


def main():
    selfcheck()
    count = 0
    errors = []
    for base in ('engine', 'game'):
        for path in sorted((ROOT / base).rglob('*')):
            if path.suffix not in ('.c', '.cpp', '.h', '.inc'):
                continue
            count += 1
            relative = path.relative_to(ROOT)
            errors.extend(f'{relative}:{line}: {message}' for line, message in check(relative, path.read_text()))
    if errors:
        print('\n'.join(errors))
        return 1
    print(f'PASS: {count} source/header files; public includes, OS ownership and negative controls')
    return 0


if __name__ == '__main__':
    sys.exit(main())
