"""One invocation root, inherited by children; explicit roots enable suite reuse."""
from contextlib import contextmanager
import os
from pathlib import Path
import tempfile

# Retain default roots for failure evidence, like explicit --output directories.
# The caller owns their lifetime; CI discards its runner temporary directory.
ROOT = Path(os.environ['AFTERSHOCK_SCRATCH']).expanduser().resolve() if os.environ.get('AFTERSHOCK_SCRATCH') else Path(tempfile.mkdtemp(prefix='aftershock-'))
ROOT.mkdir(parents=True, exist_ok=True)
os.environ['AFTERSHOCK_SCRATCH'] = str(ROOT)
os.environ['TMPDIR'] = str(ROOT)
tempfile.tempdir = str(ROOT)


@contextmanager
def cache_lock(directory):
    """Serialize Linux-only pinned-tool installation; completed tools are shared."""
    import fcntl
    directory.mkdir(parents=True, exist_ok=True)
    with (directory/'.install.lock').open('ab') as stream:
        fcntl.flock(stream, fcntl.LOCK_EX)
        yield
