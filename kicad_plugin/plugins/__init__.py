"""Kandle KiCad plugin registration.

KiCad imports this ``__init__.py`` from every directory on its plugin search
path.  We register the action plugin here so the toolbar button appears in
PCBnew.  Registration is wrapped defensively so an import failure surfaces in
the KiCad scripting console rather than breaking the editor.
"""

import os
import sys

# Ensure sibling packages (kandle_core, kandle_ui) are importable when KiCad
# adds this directory to sys.path by file rather than as a package.
_HERE = os.path.dirname(os.path.abspath(__file__))
if _HERE not in sys.path:
    sys.path.insert(0, _HERE)

try:
    from action_kandle import KandleActionPlugin

    KandleActionPlugin().register()
except Exception as exc:  # noqa: BLE001  - report, never crash KiCad
    import traceback

    sys.stderr.write("Kandle plugin failed to load: %s\n" % exc)
    traceback.print_exc()
