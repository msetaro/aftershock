#!/usr/bin/env python3
"""The known-bug policy must reject unknown errors and stale expectations."""
import contextlib
import io
from run import check_known_bugs

known = "code/qcommon/huffman_static.cpp:206:26: runtime error: load of misaligned address 0x1 for type 'const uint32_t'"
unknown = 'code/qcommon/msg.cpp:1:1: runtime error: signed integer overflow'
with contextlib.redirect_stdout(io.StringIO()):
    check_known_bugs(known)
    for diagnostics in ('', unknown, known + '\n' + unknown):
        try:
            check_known_bugs(diagnostics)
        except SystemExit:
            continue
        raise AssertionError('invalid diagnostics accepted: ' + diagnostics)
print('PASS: known diagnostics reported; unknown errors and stale entries rejected')
