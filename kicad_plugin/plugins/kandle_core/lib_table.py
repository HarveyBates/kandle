"""Read/write KiCad project library tables (``sym-lib-table`` /
``fp-lib-table``).

These files live in the project root and tell KiCad which symbol and footprint
libraries belong to the project.  The original CLI left this as a manual step
(README steps 5/6).  Registering the libraries here is what enables KiCad 9+ to
pick the new components up automatically after a reload.

Format::

    (sym_lib_table
      (version 7)
      (lib (name "op_amps")(type "KiCad")
           (uri "${KIPRJMOD}/components/extern/symbols/op_amps.kicad_sym")
           (options "")(descr ""))
    )
"""

from __future__ import annotations

import os
from typing import Optional

from . import sexpr
from .sexpr import Atom, SExpr

__all__ = ["LibTable", "register_symbol_lib", "register_footprint_lib"]

_SYM_TABLE_HEAD = "sym_lib_table"
_FP_TABLE_HEAD = "fp_lib_table"
_DEFAULT_VERSION = "7"


class LibTable:
    def __init__(self, root: SExpr, head: str):
        if root.head != head:
            raise ValueError("Not a %s" % head)
        self.root = root
        self.head = head

    @classmethod
    def load_or_empty(cls, path: str, head: str) -> "LibTable":
        if os.path.exists(path):
            with open(path, "r", encoding="utf-8") as fh:
                text = fh.read().strip()
            if text:
                return cls(sexpr.parse(text), head)
        root = SExpr([
            Atom(head),
            SExpr([Atom("version"), Atom(_DEFAULT_VERSION)]),
        ])
        return cls(root, head)

    def lib_names(self):
        names = []
        for lib in self.root.find_all("lib"):
            name = lib.find("name")
            if name and len(name.children) >= 2 \
                    and isinstance(name.children[1], Atom):
                names.append(name.children[1].value)
        return names

    def has_lib(self, name: str) -> bool:
        return name in self.lib_names()

    def add_lib(self, name: str, uri: str, lib_type: str = "KiCad",
                descr: str = "") -> bool:
        """Add a library entry if not already present. Returns True if added."""
        if self.has_lib(name):
            return False
        entry = SExpr([
            Atom("lib"),
            SExpr([Atom("name"), Atom(name, quoted=True)]),
            SExpr([Atom("type"), Atom(lib_type, quoted=True)]),
            SExpr([Atom("uri"), Atom(uri, quoted=True)]),
            SExpr([Atom("options"), Atom("", quoted=True)]),
            SExpr([Atom("descr"), Atom(descr, quoted=True)]),
        ])
        self.root.children.append(entry)
        return True

    def remove_lib(self, name: str) -> bool:
        before = len(self.root.children)
        kept = []
        for c in self.root.children:
            if isinstance(c, SExpr) and c.head == "lib":
                nm = c.find("name")
                if nm and len(nm.children) >= 2 \
                        and isinstance(nm.children[1], Atom) \
                        and nm.children[1].value == name:
                    continue
            kept.append(c)
        self.root.children = kept
        return len(self.root.children) != before

    def to_text(self) -> str:
        return sexpr.dumps(self.root)

    def save(self, path: str) -> None:
        tmp = path + ".tmp"
        with open(tmp, "w", encoding="utf-8") as fh:
            fh.write(self.to_text())
        os.replace(tmp, path)


def register_symbol_lib(project_dir: str, name: str,
                        relative_uri: str) -> bool:
    """Register a symbol library in ``<project>/sym-lib-table``."""
    path = os.path.join(project_dir, "sym-lib-table")
    table = LibTable.load_or_empty(path, _SYM_TABLE_HEAD)
    added = table.add_lib(name, "${KIPRJMOD}/" + relative_uri.lstrip("/"))
    if added:
        table.save(path)
    return added


def register_footprint_lib(project_dir: str, name: str,
                           relative_uri: str) -> bool:
    """Register a footprint library in ``<project>/fp-lib-table``."""
    path = os.path.join(project_dir, "fp-lib-table")
    table = LibTable.load_or_empty(path, _FP_TABLE_HEAD)
    added = table.add_lib(name, "${KIPRJMOD}/" + relative_uri.lstrip("/"))
    if added:
        table.save(path)
    return added
