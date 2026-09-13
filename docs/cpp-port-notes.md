# C++ port notes — record bugs, do not fix them

No engine bug fixes have been made.

- Upstream CMake defects already documented in cpp-port-plan.md section 7 remain untouched.
- Phase 0 found that C++ math-header overload resolution changes unchanged q_math.c
  from double `sincos` to float `sincosf`. This is a port compatibility hazard, not
  a defect in the C oracle. The exact G4 diff and disposition are tracked in
  cpp-port-progress.md. Do not silently add floating-point transformations outside T1–T17.

- C ASan/UBSan baseline (gcc 15.2, q3dm17, Sarge/Major): unaligned int
  loads in unzip.c:1523 and :1524 and vm.c:1181. These are existing byte-packed
  file/VM access patterns; leave them unchanged per the plan. Runtime exited 0,
  no ASan error reported. Leak checking was disabled for this initial baseline.
  Narrow function alignment suppressions are seeded in tools/port/ubsan.supp;
  suppression effectiveness must still be checked before using them as a gate.
