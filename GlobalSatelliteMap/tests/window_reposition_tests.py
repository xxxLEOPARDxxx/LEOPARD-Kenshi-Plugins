"""Compatibility entry: scalar resize checks are superseded by ready coverage.

The production-extracted presentation suite covers resize/pan/viewport changes,
unchanged frames, ownership/lifetime, pending pixels and current mip rules.
"""
from pathlib import Path
import runpy

if __name__ == '__main__':
    runpy.run_path(str(Path(__file__).with_name('presentation_caller_tests.py')), run_name='__main__')
