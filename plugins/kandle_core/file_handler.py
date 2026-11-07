"""Unzip vendor archives and classify their contents.

Supports the same providers as the original CLI (SnapEDA, Ultra Librarian,
Component Search Engine).  Unlike the C++ version this uses the standard
library :mod:`zipfile` rather than shelling out to the ``unzip`` binary, so it
works identically on every platform.
"""

from __future__ import annotations

import os
import zipfile
from dataclasses import dataclass
from typing import List, Optional

from . import project

__all__ = ["ExtractedFiles", "extract_zip", "classify", "peek_zip"]

_SYMBOL_EXTS = (".kicad_sym",)
_LEGACY_SYMBOL_EXTS = (".lib",)
_FOOTPRINT_EXTS = (".kicad_mod",)
_MODEL_EXTS = (".stp", ".step", ".wrl")


@dataclass
class ExtractedFiles:
    """Paths discovered inside an extracted vendor archive."""

    symbol: Optional[str] = None          # .kicad_sym
    legacy_symbol: Optional[str] = None   # .lib (needs conversion)
    footprint: Optional[str] = None       # .kicad_mod
    model: Optional[str] = None           # .stp / .step / .wrl


def extract_zip(zip_path: str, dest_root: str) -> str:
    """Extract ``zip_path`` into ``dest_root/<sanitized-name>``.

    Returns the directory the archive was extracted to.  If the directory
    already exists it is reused (mirrors the original behaviour).
    """
    if not os.path.isfile(zip_path):
        raise FileNotFoundError("No such file: %s" % zip_path)
    if not zipfile.is_zipfile(zip_path):
        raise ValueError("Not a valid zip file: %s" % zip_path)

    name = project.sanitize_name(zip_path)
    out_dir = os.path.join(dest_root, name)
    if os.path.isdir(out_dir):
        return out_dir

    os.makedirs(out_dir, exist_ok=True)
    with zipfile.ZipFile(zip_path) as zf:
        _safe_extract(zf, out_dir)
    return out_dir


def _classify_names(names: List[str]) -> ExtractedFiles:
    """Classify a list of archive member names by file type.

    Shares the ordering rules used by :func:`classify` (files under a ``KiCad``
    directory win), but operates on names only so it can be used to preview an
    archive without extracting it.
    """
    result = ExtractedFiles()
    kicad_first = [
        n for n in names
        if "/KiCad/" in n or n.endswith("/KiCad")
    ]
    ordered = kicad_first + [n for n in names if n not in kicad_first]
    for path in ordered:
        ext = os.path.splitext(path)[1].lower()
        if result.symbol is None and ext in _SYMBOL_EXTS:
            result.symbol = path
        elif (result.symbol is None and result.legacy_symbol is None
                and ext in _LEGACY_SYMBOL_EXTS):
            result.legacy_symbol = path
        elif result.footprint is None and ext in _FOOTPRINT_EXTS:
            result.footprint = path
        elif result.model is None and ext in _MODEL_EXTS:
            result.model = path
    return result


def peek_zip(zip_path: str) -> ExtractedFiles:
    """Classify an archive's contents without extracting it.

    Best-effort preview used by the UI to show which assets an import will
    produce.  Returns an empty :class:`ExtractedFiles` if the archive cannot be
    read.  Zip entries always use forward slashes, so name-based classification
    is platform independent.
    """
    if not os.path.isfile(zip_path) or not zipfile.is_zipfile(zip_path):
        return ExtractedFiles()
    with zipfile.ZipFile(zip_path) as zf:
        names = [n for n in zf.namelist() if not n.endswith("/")]
    return _classify_names(names)


def _safe_extract(zf: zipfile.ZipFile, dest: str) -> None:
    """Extract guarding against path traversal (zip-slip)."""
    dest_abs = os.path.abspath(dest)
    for member in zf.namelist():
        target = os.path.abspath(os.path.join(dest, member))
        if not (target == dest_abs or target.startswith(dest_abs + os.sep)):
            raise ValueError("Unsafe path in archive: %s" % member)
    zf.extractall(dest)


def _walk_files(root: str) -> List[str]:
    found = []
    for dirpath, _dirnames, filenames in os.walk(root):
        for fn in sorted(filenames):
            found.append(os.path.join(dirpath, fn))
    return found


def classify(extract_dir: str) -> ExtractedFiles:
    """Classify the files inside an extracted archive by type.

    The first file found of each type wins, matching the original tool.  The
    Component Search Engine layout nests KiCad files under a ``KiCad``
    directory, so files there are preferred.
    """
    result = ExtractedFiles()

    # Prefer files under a "KiCad" directory (Component Search Engine layout).
    all_files = _walk_files(extract_dir)
    kicad_dir_files = [
        f for f in all_files
        if os.sep + "KiCad" + os.sep in f or f.endswith(os.sep + "KiCad")
    ]
    ordered = kicad_dir_files + [
        f for f in all_files if f not in kicad_dir_files
    ]

    for path in ordered:
        ext = os.path.splitext(path)[1].lower()
        if result.symbol is None and ext in _SYMBOL_EXTS:
            result.symbol = path
        elif (result.symbol is None and result.legacy_symbol is None
                and ext in _LEGACY_SYMBOL_EXTS):
            result.legacy_symbol = path
        elif result.footprint is None and ext in _FOOTPRINT_EXTS:
            result.footprint = path
        elif result.model is None and ext in _MODEL_EXTS:
            result.model = path

    return result
