"""KiCad project discovery and the Kandle directory structure."""

from __future__ import annotations

import os
import re
from typing import List, Optional

__all__ = [
    "EXTERN_DIRS",
    "SYMBOLS_DIR",
    "FOOTPRINTS_DIR",
    "MODELS_DIR",
    "TMP_DIR",
    "sanitize_name",
    "find_project_file",
    "is_kicad_project",
    "initialise",
]

SYMBOLS_DIR = "components/symbols"
FOOTPRINTS_DIR = "components/footprints"
MODELS_DIR = "components/3dmodels"
TMP_DIR = "components/.tmp"

EXTERN_DIRS: List[str] = [
    "components",
    SYMBOLS_DIR,
    FOOTPRINTS_DIR,
    MODELS_DIR,
    TMP_DIR,
]


def sanitize_name(filename: str) -> str:
    """Normalise a vendor file/zip name into a component name.

    Mirrors the original CLI behaviour: strips the ``ul_`` (Ultra Librarian)
    and ``LIB_`` (Component Search Engine) prefixes and replaces spaces and
    dashes with underscores.
    """
    name = os.path.splitext(os.path.basename(filename))[0]
    if name.startswith("ul_"):
        name = name[3:]
    if name.startswith("LIB_"):
        name = name[4:]
    name = name.replace(" ", "_").replace("-", "_")
    return name


def find_project_file(directory: str) -> Optional[str]:
    """Return the path to a ``.kicad_pro`` file in ``directory`` if present."""
    try:
        entries = os.listdir(directory)
    except OSError:
        return None
    for entry in entries:
        if entry.endswith(".kicad_pro"):
            return os.path.join(directory, entry)
    return None


def is_kicad_project(directory: str) -> bool:
    return find_project_file(directory) is not None


def initialise(project_dir: str) -> List[str]:
    """Create the Kandle directory structure under ``project_dir``.

    Returns the list of directories that were created (not already present).
    """
    created = []
    for rel in EXTERN_DIRS:
        path = os.path.join(project_dir, rel)
        if not os.path.exists(path):
            os.makedirs(path, exist_ok=True)
            created.append(rel)
    return created


# Internal helper used by the footprint importer.
_NUMBER_RE = re.compile(r"\d+(\.\d+)?")


def constrain_footprint_text(line: str) -> str:
    """Replace the first three numbers on a font-size line with KiCad
    defaults (size 1 x 1, thickness 0.15)."""
    replacements = ["1", "1", "0.15"]
    out = []
    last = 0
    idx = 0
    for match in _NUMBER_RE.finditer(line):
        if idx >= 3:
            break
        out.append(line[last:match.start()])
        out.append(replacements[idx])
        last = match.end()
        idx += 1
    out.append(line[last:])
    return "".join(out)
