# Build instructions

Aftershock uses CMake 3.25+, a C++20 compiler and Ninja. Supported targets are
64-bit little-endian x86_64 and ARM64. Vulkan is the sole rendering backend;
OpenGL selection and the old Make/handwritten Visual Studio projects are retired.

```
cmake --workflow --preset release
cmake --workflow --preset debug
```

The default builds the client and dedicated server, with static native game
libraries and a static renderer. Binaries are inside each build directory under
`<config>-<platform>-<arch>/`. Linux needs SDL2 development headers; the default
curl configuration also uses system curl headers. Hosted dependency installation
is recorded in `.github/workflows/build.yml`. macOS uses the bundled SDL library
and requires a working Vulkan implementation such as MoltenVK at runtime.

In an MSVC developer shell, use Ninja with the release/debug presets or generate
Visual Studio 2022 projects from the same source lists:

```
cmake --workflow --preset msvc-x64
cmake --workflow --preset msvc-arm64
```

For Linux-to-Windows cross builds:

```
cmake -S . -B build/mingw -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw64.cmake -DUSE_CURL=OFF
cmake --build build/mingw
```

For an optional PC renderer module:

```
cmake -S . -B build/modules -G Ninja -DUSE_RENDERER_DLOPEN=ON
cmake --build build/modules
```

Set `BUILD_CLIENT=OFF` for server-only or `BUILD_SERVER=OFF` for client-only.
The Vulkan module and client must come from the same build. Shader packages are
verified offline; changed shaders require the pinned compiler described in
[tests/README.md](tests/README.md). Game content is supplied separately and never
included in build artifacts. See [AGENTS.md](AGENTS.md) for remaining build options
and regression commands.
