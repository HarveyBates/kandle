"""Kandle backend: pure-Python logic for importing and managing KiCad
third-party components.

This package contains no KiCad/wxPython dependencies so it can be unit tested in
isolation.  The UI layer (``kandle_ui``) and the KiCad action-plugin entry point
depend on this package, not the other way around.
"""

from .manager import ComponentManager, ImportResult

__all__ = ["ComponentManager", "ImportResult"]

__version__ = "1.0.0"
