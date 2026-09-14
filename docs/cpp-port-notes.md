# C++ port notes — record bugs, do not fix them

No engine bug fixes have been made.

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
