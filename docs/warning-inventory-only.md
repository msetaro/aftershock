Diagnostic branch only; never merge. Baseline 63615d82 includes PR #104 padding. The separately verified standard-MSVC Q_setjmp annotation from the pending C4611 preview is included, without shifting lines. Shared-header warning suppressions become same-line comments, preserving diagnostic source positions. Debug builds use /W4. No accepted fixtures or goldens change.

Second diagnostic commit applies only the cache preview of 1,667 explicit implicit-conversion casts in 154 source files. All 1,527 affected local syntax configurations pass. Object comparisons are running; no merge is authorized for this diagnostic branch. Re-inventory to identify remaining C4244 sites.

Third diagnostic commit applies the v5 conversion preview, including RHS-first compound calculations, scalar/vector macros, and reviewed Windows/header/typedef paths. All 2,380 retained syntax configurations pass. Native helper verification and production comparisons remain mandatory; no merge for this branch.

Fourth diagnostic commit measures the C4702-only preview. All 151 affected local syntax configurations pass and all four game helper hashes retain their baseline. C4244 and C4702 are errors for owned C++ sources in this diagnostic build. Never merge this branch.
