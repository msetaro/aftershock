# Aftershock — agent instructions

Aftershock is Matt's fork of the Quake3e engine (upstream: ec-/Quake3e). It is a C codebase
(~300 engine files, ~308k lines, plus vendored libjpeg/libogg/libvorbis/libcurl/libsdl).

## Current project goal: strict C -> C++20 port

The ONLY goal right now is to port the engine sources from C to C++ **with no behavioral change**.
This is not a modernization, cleanup, or refactor. Read `docs/cpp-port-plan.md` before doing any
port work; it defines the allowed transformations, the verification gates, and the PR rules.

Hard rules for port work:
- No new behavior, no bug fixes, no "while I'm here" cleanups. If you find a bug, record it in
  `docs/cpp-port-notes.md` and leave the code as-is.
- Only apply transformations from the allowed catalog in the plan. Anything else needs a separate
  commit prefixed `DEVIATION:` with a justification.
- Do not introduce classes, references, templates, STL, `auto`, `nullptr`, `constexpr`, or any
  C++ idiom. The result must read as the same C code that happens to compile as C++20.
- Vendored third-party libraries under `code/libjpeg`, `code/libogg`, `code/libvorbis`,
  `code/libcurl`, `code/libsdl` are NOT ported. Leave them as C. `code/renderer2` is not ported either.
- Do not rename files (`.c` -> `.cpp`) until the plan's rename phase; that phase is a separate,
  content-free `git mv` change.
- The C build is the oracle. Every change must keep the C build green until the rename phase.

## Code rules for the port phase

Full table with the modernization-phase counterpart is in `docs/cpp-port-plan.md` section 11.
- Style is "match the surrounding line": tabs, spaces inside parentheses `( a, b )`, `NULL`,
  C-style casts, `qboolean`. Writing `nullptr` in a file full of `NULL` fails review even if
  the code is correct. clang-format runs on changed lines only; never reformat untouched lines.
- Warnings: `-Werror` against a frozen list of disabled warnings. Do not "fix" pre-existing
  warnings (a signed/unsigned change can change behavior); that is a later, separate PR class.
- Never restructure a floating-point expression in `qcommon/cm_*`, `q_math.c`, `bg_*`, `msg.c`
  or snapshot code. Cross-build determinism is what netcode and demos depend on.
- Keep intentional undefined-behavior patterns as they are (`Q_rsqrt` punning, file buffers
  cast to structs). They define behavior. Do not replace with `memcpy`/`std::bit_cast`.
- No assertions, no RAII, no smart pointers, no namespaces, no fixed-width type changes. The
  memory model (hunk/zone) and `Com_Error` (a `longjmp`) are untouched.
- Subsystem boundaries are the existing prefixes (`Sys_`, `Com_`, `FS_`, `CL_`, `SV_`, `R_`,
  `S_`) and `*_public.h` vs `*_local.h`. Include only other subsystems' public headers.
- Definition of done: C and C++ builds green, every verification gate green, every hunk maps to
  a catalog entry T1-T17, diff size proportionate, PR description lists transformation counts
  and pasted gate output.

## Layout

- `code/qcommon` shared core: cvars, commands, filesystem, network, collision (cm_*), QVM (vm_*)
- `code/server`, `code/client` server and client
- `code/botlib` bot AI library
- `code/renderercommon`, `code/renderer` (OpenGL1), `code/renderervk` (Vulkan); `code/renderer2` (OpenGL2) is out of scope and not ported
- `code/unix`, `code/win32`, `code/sdl` platform layers
- `code/asm` hand-written assembly (referenced by C symbol name; needs `extern "C"` after the port)
- `code/cgame`, `code/game`, `code/ui` only the shared public headers / bg code for the QVM ABI

## Building

The GNU Makefile is the only supported build. CMake exists but is broken upstream (wrong arch
suffix, renderers not linked with -lm, missing asm source); do not use it. MSVC projects live in
`code/win32/msvc2017` and are CI-only. Output goes to `build/<config>-<platform>-<arch>/`.

```
make -j$(nproc)                                  # client + server + dlopen renderers
make -j$(nproc) BUILD_CLIENT=0                   # dedicated server only (no SDL/X11/curl needed)
make -j$(nproc) BUILD_SERVER=0 USE_RENDERER_DLOPEN=0 RENDERER_DEFAULT=vulkan
make debug -j$(nproc)                            # -O0 -g build
make clean
```

C++ probe build (until the Makefile gains a proper C++ mode; see plan):

```
make -k -j$(nproc) BUILD_DIR=/tmp/cxx CC="g++ -x c++ -std=c++20 -fpermissive"
```

Local machine: gcc 15.2, clang 21, 20 cores, all dev packages installed; every Makefile
configuration builds. No display in agent shells: client runtime tests need a desktop session
or Xvfb + Mesa software drivers; the dedicated server runs headless. Game data is in
`~/.q3a/baseq3/` and is found automatically; never commit or copy pak files into the repo.

## Verification commands

See `docs/cpp-port-plan.md` section "Verification gates". Minimum before any port PR:
1. C build: `make -j$(nproc)` and `make -j$(nproc) BUILD_CLIENT=0` are green.
2. C++ build of the touched module compiles with zero new errors and zero new `-fpermissive`
   warnings compared to the previous commit.
3. `git diff --stat` is proportionate: a mechanical port touches a few percent of lines at most.

## Conventions

- Keep the existing code style exactly (tabs, brace placement, `qboolean`, `NULL`, C casts).
- Commit messages: `<module>: <what>` like upstream, e.g. `qcommon: C++ compatibility for net_chan`.
- One module (or one coherent file group) per PR.
