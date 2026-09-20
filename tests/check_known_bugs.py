#!/usr/bin/env python3
"""The known-bug policy must reject unknown errors and stale expectations."""
import contextlib
import io
from run import check_known_bugs

known = 'known: runtime error: test'
patterns = [r'^known: runtime error: test$']
unknown = 'engine/qcommon/msg.cpp:1:1: runtime error: signed integer overflow'
with contextlib.redirect_stdout(io.StringIO()):
    check_known_bugs(known, patterns)
    for diagnostics in ('', unknown, known + '\n' + unknown):
        try:
            check_known_bugs(diagnostics, patterns)
        except SystemExit:
            continue
        raise AssertionError('invalid diagnostics accepted: ' + diagnostics)
print('PASS: known diagnostics reported; unknown errors and stale entries rejected')
