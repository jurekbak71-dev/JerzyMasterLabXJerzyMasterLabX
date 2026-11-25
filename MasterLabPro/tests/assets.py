#!/usr/bin/env python3
"""Black-box regression for the approved Package A asset validator."""

import subprocess
import sys
from pathlib import Path


project = Path(__file__).resolve().parents[1]
validator = project / "tools" / "validate_assets.py"
result = subprocess.run(
    [sys.executable, str(validator), str(project / "assets")],
    check=False,
    capture_output=True,
    text=True,
)
if result.returncode != 0:
    raise SystemExit(result.stderr or result.stdout or "asset validation failed")
if "235 PNG files validated" not in result.stdout:
    raise SystemExit(f"unexpected validator output: {result.stdout!r}")
print("PASS: approved asset manifest, paths, hashes and dimensions")
