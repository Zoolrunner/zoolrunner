"""hfsutils stores the selected volume in per-user state, across processes."""
from contextlib import contextmanager
import fcntl
from pathlib import Path
import tempfile


@contextmanager
def hfsutils_lock():
    with (Path(tempfile.gettempdir()) / "zoolrunner-hfsutils.lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        try:
            yield
        finally:
            fcntl.flock(lock, fcntl.LOCK_UN)
