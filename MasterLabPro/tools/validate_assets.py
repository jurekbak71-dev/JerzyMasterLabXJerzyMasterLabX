#!/usr/bin/env python3
"""Validate the immutable Package A PNG inventory without image libraries."""

from __future__ import annotations

import hashlib
import json
import struct
import sys
from pathlib import Path


EXPECTED_PNG_COUNT = 235
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def fail(message: str) -> None:
    raise ValueError(message)


def validate(root: Path) -> int:
    root = root.resolve(strict=True)
    manifest_path = root / "manifest.json"
    entries = json.loads(manifest_path.read_text(encoding="utf-8"))
    if not isinstance(entries, list) or len(entries) != EXPECTED_PNG_COUNT:
        fail(f"manifest must contain exactly {EXPECTED_PNG_COUNT} entries")

    listed: set[str] = set()
    for index, entry in enumerate(entries):
        if not isinstance(entry, dict):
            fail(f"entry {index} is not an object")
        try:
            relative = entry["path"]
            expected_width = entry["width"]
            expected_height = entry["height"]
            expected_hash = entry["sha256"]
        except KeyError as error:
            fail(f"entry {index} misses {error.args[0]}")
        if not isinstance(relative, str) or relative in listed:
            fail(f"duplicate or invalid path at entry {index}")
        listed.add(relative)
        path = (root / relative).resolve()
        if root not in path.parents or not path.is_file():
            fail(f"missing or escaping asset: {relative}")
        data = path.read_bytes()
        if len(data) < 24 or data[:8] != PNG_SIGNATURE or data[12:16] != b"IHDR":
            fail(f"invalid PNG header: {relative}")
        width, height = struct.unpack(">II", data[16:24])
        if (width, height) != (expected_width, expected_height):
            fail(
                f"dimension mismatch: {relative}: got {width}x{height}, "
                f"expected {expected_width}x{expected_height}"
            )
        actual_hash = hashlib.sha256(data).hexdigest()
        if actual_hash != expected_hash:
            fail(f"SHA-256 mismatch: {relative}")

    actual = {
        path.relative_to(root).as_posix() for path in root.rglob("*.png")
    }
    if actual != listed:
        missing = sorted(listed - actual)
        extra = sorted(actual - listed)
        fail(f"PNG inventory mismatch; missing={missing}, extra={extra}")
    return len(entries)


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print("usage: validate_assets.py ASSET_ROOT", file=sys.stderr)
        return 2
    try:
        count = validate(Path(argv[1]))
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"asset validation failed: {error}", file=sys.stderr)
        return 1
    print(f"PASS: {count} PNG files validated")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
