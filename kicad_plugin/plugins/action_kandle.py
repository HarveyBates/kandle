"""KiCad action plugin entry point for Kandle.

Registers a toolbar button in PCBnew that opens the Kandle Component Manager
window.  The heavy lifting lives in ``kandle_core`` (pure Python) and
``kandle_ui`` (wxPython); this file only wires them into KiCad.
"""

from __future__ import annotations

import os

import pcbnew
import wx


def _find_project_dir() -> str:
    """Resolve the current KiCad project directory from the open board."""
    board = pcbnew.GetBoard()
    board_path = board.GetFileName() if board else ""
    if board_path:
        return os.path.dirname(os.path.abspath(board_path))
    return os.getcwd()


class KandleActionPlugin(pcbnew.ActionPlugin):
    def defaults(self):
        self.name = "Kandle - Component Manager"
        self.category = "Import"
        self.description = (
            "Import and manage third-party symbols, footprints and 3D models "
            "(SnapEDA, Ultra Librarian, Component Search Engine)."
        )
        self.show_toolbar_button = True

        # IMPORTANT: the toolbar icon must live inside the 'plugins' tree.
        # When installed via the Plugin & Content Manager, KiCad relocates the
        # package's 'resources/' folder to a separate 3rdparty/resources/<id>/
        # directory, so '../resources/icon.png' no longer resolves and KiCad
        # shows its default (blue jigsaw) icon. Icons kept next to the code
        # travel with it and always resolve. We search a few locations so the
        # plugin also works when run uninstalled from the repo.
        here = os.path.dirname(os.path.abspath(__file__))
        search_dirs = [
            os.path.join(here, "icons"),            # bundled with the code
            os.path.normpath(os.path.join(here, "..", "resources")),  # dev
        ]

        def _icon(*names):
            for directory in search_dirs:
                for name in names:
                    path = os.path.join(directory, name)
                    if os.path.isfile(path):
                        return path
            return ""

        # Light theme: dark glyph icon. Dark theme: light glyph icon.
        self.icon_file_name = _icon("icon_48.png", "icon.png")
        self.dark_icon_file_name = _icon(
            "icon_48_dark.png", "icon_48.png", "icon.png")

    def Run(self):
        # Import lazily so a backend import error doesn't break KiCad startup.
        from kandle_core import project
        from kandle_ui.manager_frame import ManagerFrame

        project_dir = _find_project_dir()
        if not project.is_kicad_project(project_dir):
            wx.MessageBox(
                "No KiCad project (.kicad_pro) found for the current board.\n"
                "Open or save your project first.",
                "Kandle", wx.OK | wx.ICON_WARNING)
            return

        frame = ManagerFrame(None, project_dir)
        frame.Show()
