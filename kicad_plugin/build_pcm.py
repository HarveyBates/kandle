#!/usr/bin/env python3
"""Build a KiCad Plugin & Content Manager (PCM) archive for Kandle.

The resulting zip follows the PCM layout (``metadata.json``, ``plugins/`` and
``resources/`` at the archive root) and can be installed via
*Plugin and Content Manager -> Install from File* in KiCad 9+.

Usage::

    python build_pcm.py [output.zip]
"""

from __future__ import annotations

import fnmatch
import os
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))

# What goes into the archive (relative to this directory).
INCLUDE = ["metadata.json", "plugins", "resources"]

# Never ship these.
EXCLUDE_PATTERNS = [
    "*.pyc", "__pycache__", "*.pyo", ".DS_Store", "*.tmp",
]


def _excluded(path: str) -> bool:
    for part in path.split(os.sep):
        for pat in EXCLUDE_PATTERNS:
            if fnmatch.fnmatch(part, pat):
                return True
    return False


# Icons the plugin expects. Missing ones only degrade appearance, so warn
# rather than fail the build.
EXPECTED_ICONS = [
    "resources/icon.png",             # PCM catalogue icon (64x64)
    # Toolbar icons must be bundled inside plugins/ because PCM relocates
    # resources/ to a separate tree on install.
    "plugins/icons/icon_48.png",      # toolbar icon, light theme
    "plugins/icons/icon_48_dark.png",  # toolbar icon, dark theme
]


def _check_icons() -> None:
    missing = [
        rel for rel in EXPECTED_ICONS
        if not os.path.isfile(os.path.join(HERE, rel.replace("/", os.sep)))
    ]
    for rel in missing:
        print("WARNING: expected icon not found: %s" % rel)


def build(output: str) -> None:
    _check_icons()
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED) as zf:
        for item in INCLUDE:
            full = os.path.join(HERE, item)
            if os.path.isfile(full):
                zf.write(full, item)
                continue
            for dirpath, dirnames, filenames in os.walk(full):
                dirnames[:] = [d for d in dirnames if not _excluded(d)]
                for fn in filenames:
                    abs_path = os.path.join(dirpath, fn)
                    rel = os.path.relpath(abs_path, HERE)
                    if _excluded(rel):
                        continue
                    zf.write(abs_path, rel)
    print("Wrote %s" % output)


if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        HERE, "kandle-pcm.zip")
    build(out)
