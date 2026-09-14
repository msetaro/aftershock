# C++ port notes — record bugs, do not fix them

The strict port made no engine bug fixes. Modernization #31 dispositions are recorded below.

- Upstream CMake defects already documented in cpp-port-plan.md section 7 remain untouched.
- Phase 0 found that C++ math-header overload resolution changes unchanged q_math.c
  from double `sincos` to float `sincosf`. This is a port compatibility hazard, not
  a defect in the C oracle. The exact G4 diff and disposition are tracked in
  cpp-port-progress.md. The accepted T21 catalog now pins these C math calls.

- C ASan/UBSan baseline (gcc 15.2, q3dm17, Sarge/Major): unaligned int
  loads in unzip.c:1523 and :1524 and vm.c:1181. These are existing byte-packed
  file/VM access patterns; leave them unchanged per the plan. Runtime exited 0,
  no ASan error reported. Leak checking was disabled for this initial baseline.
  Narrow function alignment suppressions are seeded in tools/port/ubsan.supp;
  suppression effectiveness was confirmed by the subsequent C runtime smoke (exit 0, no sanitizer diagnostics).

- The q_math hazard is confirmed by `tools/port/math_gate.sh`: fixed-input hashes
  differed before T21; both math gates now pass, including vector_math 5c00b4de.
  The unchanged C implementation remains the oracle.

- The exact unattended bot smoke is nondeterministic for two runs of the same C binary: Item events differ because engine/game initialization uses wall-clock seeds. This is a verification limitation, not a new engine bug; no timing or seed behavior changed. The full repeat diff is retained in tools/port/evidence/c-runtime-repeat.diff.

- linux_snd.c passes void(void) thread procedures through void* to pthread_create, whose callback type is void*(*)(void*). This preexisting signature mismatch remains; T1 casts wrap the existing conversion without changing thread bodies or signatures. C object hash remains identical.

- cl_curl.c Com_DL_Begin tests dl->URL[strlen(dl->URL)] against slash. That index is the terminating NUL, so the slash append always executes when percent-1 URL replacement fails. Recorded during constness review; source unchanged.

- FS_AllowedExtension compares the result of strrchr relationally to fileName + 3 before checking it for NULL. Inputs without an extension reach a relational comparison involving NULL; this existing undefined pointer comparison was noticed while preparing G5. No source fix.

- The prescribed libfaketime increment procedure resolves the earlier unfaked bot-smoke repeat limitation: two warmed C logs are now byte-identical. No engine timing or seed code changed.
- Clang C++ warns that qcurl_easy_setopt_warn uses a CURLoption enum as the last named va_start argument (cl_curl.c:264); the enum undergoes default argument promotion. This preexisting source pattern is left intact; no public signature or varargs logic is changed.

- Sanitized GCC build warns that AAS_Reachability_JumpArea beststart may be uninitialized at be_aas_reach.cpp:2196. The original C sanitizer build reports the same warning. If the nested qualifying-face/edge loops never supply a candidate, VectorMiddle reads beststart before the later bestdist check. Recorded without initialization/control-flow changes.

- Modernization #3 differential driver under Clang ASan/UBSan reports an unaligned
  `const uint32_t` load in `HuffmanGetSymbol` at huffman_static.cpp:206 when reading the
  existing MSG roundtrip fixture (buffer offset one). Reproduce:
  `python3 tests/run.py unit --cc clang --cxx 'clang++ -stdlib=libc++' --sanitize --output /tmp/aftershock-san-tests`.
  Existing tools/port/ubsan.supp does not cover this function; no new suppression or
  engine fix was added. Repair belongs in a separate #31 failing-test-first PR.

- Modernization #3 seeded PNG fuzzing reports `PNG_ChunkHeader` member access on a
  byte-packed chunk at tr_image_png.cpp:467 (the valid 1x1 RGBA seed already triggers
  it). Reproduce: `python3 tests/fuzz/run.py png --runs 1000`. No inline fix.
- Modernization #3 JPEG fuzzing reports a Huffman-table index outside the four-entry
  array at libjpeg/jdmarker.c:508: get_dht forms the table-element address before
  rejecting an invalid index. Reproduce: `python3 tests/fuzz/run.py jpeg --runs 1000`;
  observed crash SHA1 1868dc3f9cb6875028fd4f795a1d596b87706c16 under /tmp/aftershock-fuzz/jpeg.
  Vendor code remains untouched; a dedicated #31 test/fix/upstream PR is required.

- #31 Huffman alignment fix: `HuffmanGetSymbol` now copies the packed word with
  memcpy before applying the unchanged shifts/mask. The fatal sanitizer unit
  reproducer failed before and passes after. GCC production assembly/symbols and
  regenerated unit/differential goldens are identical; both-map bot logs and
  fixed-demo frames remain identical. Removed its known-bugs entry; it never had
  a UBSan suppression. Upstream C regression covers 256 symbols at 32 bit offsets.
  Upstream C fix/test: https://github.com/ec-/Quake3e/pull/424.

  Huffman fork fix merged: https://github.com/msetaro/aftershock/pull/36.

- #31 filesystem extension validation: extensionless names (`"no_extension"`,
  `"a"`, `""`) and short dotted names (`"."`, `".x"`) now have permanent assertions.
  Reproducer: `python3 tests/run.py unit --cc clang --cxx clang++ --sanitize
  --pointer-compare --output /tmp/tests-pointers`. Before: ASan invalid-pointer-pair
  in FS_AllowedExtension, NULL versus filename+3, exit 1. After moving the NULL
  check first and using `(e - fileName) >= 3`: pass with unchanged unit hash.
  Separate UBSan and pointer-comparison runs avoid combined-instrumentation
  interference seen with Clang 21 while retaining both checks in CI. Upstream C
  fix and failing-then-passing test: https://github.com/ec-/Quake3e/pull/425.
  Fork PR #37 passed regression 34877286456 and full build 34877286443.

- #31 caller audit finding (separate fix pending): Unix and Windows Sys_LoadLibrary
  pass an uninitialized local `ext` to Com_Error when FS_AllowedExtension returns
  true. The latter only writes `ext` when rejecting an executable extension.
  Reproducer path: Sys_LoadLibrary with an extensionless or non-executable name
  reaches the error formatter with an indeterminate `%s` pointer. Recorded from
  call-flow inspection, not executed; no loader test or source fix in this PR.

- #31 download URL separator fix: Com_DL_Begin indexed the terminating NUL instead
  of the last character. `python3 tests/download.py` fails before on a trailing-slash
  base producing a double separator and passes after the guarded last-character
  check. GCC/Clang test calls real begin/cleanup with libcurl, but never performs
  a transfer. New golden `tests/golden/download.txt` records bases with/without `/`,
  percent-1 replacement/escaping and the preserved empty-base result. No existing
  gameplay or frame golden changes. Caller audit: CL_Download is the sole caller
  and already rejects empty configured URLs; the shared fix still guards empty input.
  Upstream C test/fix: https://github.com/ec-/Quake3e/pull/426.
  Local unit/negative-control, collision, smoke and replay gates pass unchanged.
  Fork PR #38 passed regression 34878247137 and full build 34878247337.

- #31 ALSA callback fix: both native audio thread procedures now have the required
  `void *(*)(void *)` signature; pthread_create no longer needs casts. Returning NULL
  replaces the explicit pthread_exit call and its unused import. Reproducer:
  `python3 tests/audio.py` fails compilation before and passes after with GCC and
  Clang/libc++. The real ALSA null sink receives positive MMAP/DIRECT submissions,
  then both threads join. No physical device is needed. Caller audit finds only
  the two setup_ALSA registrations. The dynamic-ALSA production object also builds.
  Upstream C test/fix: https://github.com/ec-/Quake3e/pull/427. Unit/collision
  regeneration produces no golden diff; both-map smoke and both-renderer replay pass.
  Fork PR #39 passed regression 34879358567 and full build 34879358584.

- #31 curl va_start fix: the last named argument is now int; a CURLoption local
  preserves the forwarding logic. `python3 tests/download.py --cc clang --cxx
  'clang++ -stdlib=libc++' --output /tmp/tests-curl` fails before on -Werror=varargs
  and passes after, including a local-file transfer verifying long/pointer/offset
  options. Removed the Makefile's -Wno-varargs. URL golden is unchanged.
  Correction to the original classification: Clang C/C11 accepts the upstream
  source, and a type probe finds CURLoption compatible with its promoted C type
  (unsigned int). C++ distinguishes the enum and rejects va_start. This is a port
  defect on our toolchains; no failing upstream C test or upstream PR is claimed.
  The pre-C++26 parameter restriction is described in WG21 P2537R2:
  https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2537r2.html.
  GCC production codegen and symbol gates are identical. Unit/one-ULP, collision,
  serial both-map smoke and both-renderer replay pass unchanged. All callers are
  within cl_curl.cpp; no public header declaration or wire layout changes.
  Fork PR #40 passed regression 34880567812 and full build 34880567796.
  The normalized symbol gate passes; the internal C++ mangled name changes with
  the parameter type, and all callers rebuild in the same translation unit.

- #31 ZIP packed reads: removed unzlocal_GetCurrentFileInfoInternal's alignment
  suppression. `python3 tests/run.py runtime --sanitize --output /tmp/tests-runtime-ubsan`
  is the existing valid-content bot smoke under GCC UBSan. The original ZIP object
  fails at the unaligned int load; CopyLittleLong through a signed temporary passes
  both original Q3 map goldens. Keeping the temporary signed preserves the original
  conversion to the wider uLong fields. All archive iteration paths use the same
  shared function (file-info queries and first/next/position navigation). Both OA
  smoke maps and normal unit/collision/smoke/replay gates also pass unchanged.
  Symbol gate passes; GCC assembly only exchanges stack slots 64/68 and matching
  sign-extending consumers, reviewed acceptable under #31. No gate weakening.
  Upstream C reproducer fails before and passes after:
  https://github.com/ec-/Quake3e/pull/428. Fork PR #41 passed regression
  34885119593 and full build 34885119668 on source 09e7cc96.

- Additional #31 finding during runtime expansion: Clang UBSan reports zcalloc
  and zcfree calls through incompatible function-pointer types. Their private
  signatures use byte pointers while z_stream callbacks use void pointers.
  Reproduce after the ZIP alignment fix with `python3 tests/run.py runtime --cc clang
  --cxx clang++ --sanitize --output /tmp/tests-clang-runtime`. The error occurs
  during ordinary valid-pak startup. A recovery survey reports both callback kinds
  before a later SIGSEGV at legacy QVM startup (cause not yet isolated). No callback
  fix or new suppression in the ZIP PR; the GCC runtime baseline and existing
  Clang unit sanitizer checks remain the required gates. The ASan/faketime runtime
  experiment timed out before output; it is not treated as an engine failure or pass.

- #31 VM packed operand fix: the existing `python3 tests/run.py runtime --sanitize`
  fails before at VM_LoadInstructions with its alignment suppression removed.
  CopyLittleLong into an int32_t temporary passes the original Q3/OA map goldens;
  tools/port/ubsan.supp is now empty. The interpreter and x86/aarch64/armv7/powerpc
  compilers all call this shared function. GCC production symbols pass and all
  26 function-section byte sequences are identical. The text codegen gate differs
  only in the compiler-generated switch-table name CSWTCH.89/90, with unchanged
  instructions and table data; no gate weakening. Upstream C startup on a valid
  qagame QVM fails before and passes after (only its separate ZIP bug suppressed).
  Upstream fix: https://github.com/ec-/Quake3e/pull/429. Explicit unit/collision
  regeneration has no golden diff; normal smoke and fixed-demo replay pass unchanged.
