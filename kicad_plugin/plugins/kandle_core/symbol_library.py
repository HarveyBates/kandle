"""Safe manipulation of KiCad symbol libraries (``.kicad_sym``).

A ``.kicad_sym`` file is a single ``(kicad_symbol_lib ...)`` S-expression that
contains zero or more top-level ``(symbol "NAME" ...)`` blocks.  Each top-level
symbol may contain child unit symbols named ``"NAME_<unit>_<style>"``.

Unlike the original line-based approach, every operation here parses the whole
library into a tree, mutates it and re-serialises it.  This makes *rename* and
*delete* safe even inside a shared library file with many symbols.
"""

from __future__ import annotations

import os
from typing import List, Optional

from . import sexpr
from .sexpr import Atom, SExpr

__all__ = ["SymbolLibrary"]

_DEFAULT_VERSION = "20211014"
_DEFAULT_GENERATOR = "kicad_symbol_editor"


class SymbolLibrary:
    """In-memory model of a ``.kicad_sym`` file."""

    def __init__(self, root: SExpr):
        if root.head != "kicad_symbol_lib":
            raise ValueError("Not a valid kicad_symbol_lib")
        self.root = root

    # -- construction ----------------------------------------------------------
    @classmethod
    def from_text(cls, text: str) -> "SymbolLibrary":
        return cls(sexpr.parse(text))

    @classmethod
    def load(cls, path: str) -> "SymbolLibrary":
        with open(path, "r", encoding="utf-8") as fh:
            return cls.from_text(fh.read())

    @classmethod
    def empty(cls) -> "SymbolLibrary":
        root = SExpr([
            Atom("kicad_symbol_lib"),
            SExpr([Atom("version"), Atom(_DEFAULT_VERSION)]),
            SExpr([Atom("generator"), Atom(_DEFAULT_GENERATOR)]),
        ])
        return cls(root)

    # -- serialisation ---------------------------------------------------------
    def to_text(self) -> str:
        return sexpr.dumps(self.root)

    def save(self, path: str) -> None:
        tmp = path + ".tmp"
        with open(tmp, "w", encoding="utf-8") as fh:
            fh.write(self.to_text())
        os.replace(tmp, path)

    # -- queries ---------------------------------------------------------------
    def _top_symbols(self) -> List[SExpr]:
        return self.root.find_all("symbol")

    def symbol_names(self) -> List[str]:
        names = []
        for sym in self._top_symbols():
            name = _symbol_name(sym)
            if name is not None:
                names.append(name)
        return names

    def get_symbol(self, name: str) -> Optional[SExpr]:
        for sym in self._top_symbols():
            if _symbol_name(sym) == name:
                return sym
        return None

    def has_symbol(self, name: str) -> bool:
        return self.get_symbol(name) is not None

    # -- mutations -------------------------------------------------------------
    def add_symbol(self, symbol: SExpr, overwrite: bool = False) -> None:
        """Insert a top-level ``(symbol ...)`` node into the library."""
        if symbol.head != "symbol":
            raise ValueError("Node is not a symbol")
        name = _symbol_name(symbol)
        if name is None:
            raise ValueError("Symbol has no name")
        if self.has_symbol(name):
            if not overwrite:
                raise ValueError("Symbol %r already exists" % name)
            self.delete_symbol(name)
        self.root.children.append(symbol)

    def delete_symbol(self, name: str) -> bool:
        before = len(self.root.children)
        self.root.children = [
            c
            for c in self.root.children
            if not (isinstance(c, SExpr)
                    and c.head == "symbol"
                    and _symbol_name(c) == name)
        ]
        return len(self.root.children) != before

    def rename_symbol(self, old_name: str, new_name: str) -> None:
        if old_name == new_name:
            return
        if self.has_symbol(new_name):
            raise ValueError("Symbol %r already exists" % new_name)
        sym = self.get_symbol(old_name)
        if sym is None:
            raise KeyError("Symbol %r not found" % old_name)
        rename_symbol_node(sym, old_name, new_name)

    def set_footprint(self, name: str, footprint: str) -> None:
        """Set the ``Footprint`` property value of a symbol."""
        sym = self.get_symbol(name)
        if sym is None:
            raise KeyError("Symbol %r not found" % name)
        set_symbol_footprint(sym, footprint)


# --------------------------------------------------------------------------- #
# Free functions that operate on a single ``(symbol ...)`` node
# --------------------------------------------------------------------------- #

def _symbol_name(symbol: SExpr) -> Optional[str]:
    """Return the quoted name that follows the ``symbol`` head."""
    for child in symbol.children[1:]:
        if isinstance(child, Atom):
            return child.value
        break
    return None


def rename_symbol_node(symbol: SExpr, old_name: str, new_name: str) -> None:
    """Rename a symbol and all of its child unit symbols in place."""
    # Rename the top-level name atom.
    for idx, child in enumerate(symbol.children[1:], start=1):
        if isinstance(child, Atom):
            symbol.children[idx] = Atom(new_name, quoted=True)
            break

    # Rename child unit symbols: "OLD_0_1" -> "NEW_0_1".
    prefix = old_name + "_"
    for child in symbol.children:
        if isinstance(child, SExpr) and child.head == "symbol":
            child_name = _symbol_name(child)
            if child_name and child_name.startswith(prefix):
                suffix = child_name[len(old_name):]
                for i, c in enumerate(child.children[1:], start=1):
                    if isinstance(c, Atom):
                        child.children[i] = Atom(
                            new_name + suffix, quoted=True
                        )
                        break


def _find_property(symbol: SExpr, key: str) -> Optional[SExpr]:
    for child in symbol.children:
        if isinstance(child, SExpr) and child.head == "property":
            # (property "Key" "Value" ...)
            if (len(child.children) >= 2
                    and isinstance(child.children[1], Atom)
                    and child.children[1].value == key):
                return child
    return None


def set_symbol_footprint(symbol: SExpr, footprint: str) -> None:
    """Set (or create) the ``Footprint`` property value."""
    prop = _find_property(symbol, "Footprint")
    if prop is not None and len(prop.children) >= 3 \
            and isinstance(prop.children[2], Atom):
        prop.children[2] = Atom(footprint, quoted=True)
        return
    # No Footprint property present: append a minimal one.
    new_prop = SExpr([
        Atom("property"),
        Atom("Footprint", quoted=True),
        Atom(footprint, quoted=True),
        SExpr([Atom("id"), Atom("2")]),
        SExpr([Atom("at"), Atom("0"), Atom("0"), Atom("0")]),
    ])
    # Insert after the name atom / existing leading atoms.
    insert_at = 1
    for i, child in enumerate(symbol.children):
        if isinstance(child, SExpr):
            insert_at = i
            break
    symbol.children.insert(insert_at, new_prop)


def extract_first_symbol(text: str) -> SExpr:
    """Return the first top-level ``(symbol ...)`` node from library text."""
    root = sexpr.parse(text)
    if root.head != "kicad_symbol_lib":
        raise ValueError("Source is not a kicad_symbol_lib")
    symbols = root.find_all("symbol")
    if not symbols:
        raise ValueError("Source library contains no symbols")
    return symbols[0]
