"""Current production discrete-step caller controls; supersedes relative-gain assertions.
Historical relative input/registration proof remains in git history. No game/UI calls.
"""
from pathlib import Path
import runpy
runpy.run_path(str(Path(__file__).resolve().parent/"fixed-zoom/caller.py"),run_name="__main__")
