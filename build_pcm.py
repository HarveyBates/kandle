#!/usr/bin/env python3
"""Build a KiCad Plugin & Content Manager (PCM) archive for Kandle.

The resulting zip follows the PCM layout (``metadata.json``, ``plugins/`` and
``resources/`` at the archive root) and can be installed via
*Plugin and Content Manager -> Install from File* in KiCad 9+.

After building the archive it also computes the fields the *official KiCad
addons repository* (https://gitlab.com/kicad/addons/metadata) requires for
each version entry -- ``download_url``, ``download_sha256``, ``download_size``
and ``install_size`` -- and writes a submission-ready
``metadata.repo.json``. The ``metadata.json`` shipped *inside* the archive is
intentionally left without those fields (they are redundant there and the repo
CI expects the in-zip copy to omit them).

Usage::

    python build_pcm.py [output.zip] [--download-url URL]

If ``--download-url`` is omitted a GitHub release URL is derived from the
package homepage and version, e.g.::

    https://github.com/HarveyBates/kandle/releases/download/v1.0.0/kandle-pcm.zip
"""

from __future__ import annotations

import argparse
import fnmatch
import hashlib
import json
import os
import re
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


def _archive_metrics(output: str) -> tuple[str, int, int]:
    """Return (sha256, download_size, install_size) for the built archive.

    ``download_size`` is the on-disk size of the zip; ``install_size`` is the
    sum of the *uncompressed* sizes of its members -- both as required by the
    KiCad addons repository schema.
    """
    sha = hashlib.sha256()
    with open(output, "rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            sha.update(chunk)
    download_size = os.path.getsize(output)
    with zipfile.ZipFile(output) as zf:
        install_size = sum(info.file_size for info in zf.infolist())
    return sha.hexdigest(), download_size, install_size


def _default_download_url(meta: dict, version: str) -> str | None:
    """Derive a GitHub release asset URL from the package homepage."""
    home = (meta.get("resources", {}) or {}).get("homepage", "")
    m = re.match(r"https?://github\.com/([^/]+/[^/]+)", home)
    if not m:
        return None
    repo = m.group(1).rstrip("/")
    return (
        "https://github.com/%s/releases/download/v%s/kandle-pcm.zip"
        % (repo, version)
    )


def write_repo_metadata(output: str, download_url: str | None) -> None:
    """Write metadata.repo.json ready for the KiCad addons repository MR."""
    with open(os.path.join(HERE, "metadata.json"), encoding="utf-8") as fh:
        meta = json.load(fh)

    sha256, download_size, install_size = _archive_metrics(output)

    for ver in meta.get("versions", []):
        url = download_url or _default_download_url(meta, ver["version"])
        if not url:
            raise SystemExit(
                "Could not derive a download URL; pass --download-url.")
        ver["download_url"] = url
        ver["download_sha256"] = sha256
        ver["download_size"] = download_size
        ver["install_size"] = install_size

    repo_path = os.path.join(HERE, "metadata.repo.json")
    with open(repo_path, "w", encoding="utf-8") as fh:
        json.dump(meta, fh, indent=2, ensure_ascii=False)
        fh.write("\n")

    print("Wrote %s" % repo_path)
    print("  download_sha256 = %s" % sha256)
    print("  download_size   = %d bytes" % download_size)
    print("  install_size    = %d bytes" % install_size)


def main(argv: list[str]) -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "output", nargs="?", default=os.path.join(HERE, "kandle-pcm.zip"),
        help="output archive path (default: kandle-pcm.zip)")
    parser.add_argument(
        "--download-url", default=None,
        help="public URL where the archive will be hosted "
             "(default: derived GitHub release URL)")
    args = parser.parse_args(argv)

    build(args.output)
    write_repo_metadata(args.output, args.download_url)


if __name__ == "__main__":
    main(sys.argv[1:])
