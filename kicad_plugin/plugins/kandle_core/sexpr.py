"""A small, dependency-free S-expression engine for KiCad files.

KiCad stores symbol libraries (``.kicad_sym``), footprints (``.kicad_mod``) and
the library tables (``sym-lib-table`` / ``fp-lib-table``) as S-expressions.

The C++ implementation of Kandle manipulated these files with line-based string
surgery (``find("Footprint")``, regex over font sizes, manual insertion under
``kicad_symbol_lib``).  That is adequate for *appending* a symbol but is unsafe
for *renaming* or *deleting* a symbol inside a shared library file.

This module provides a proper tokenizer, parser and pretty-printer so that the
rest of the backend can operate on a parsed tree rather than raw text.

The tree is made of two node types:

* :class:`Atom` - a leaf token.  ``quoted`` records whether the token was a
  quoted string so that round-tripping preserves semantics (a bare ``yes`` is
  not the same as the string ``"yes"``).
* :class:`SExpr` - a list node, e.g. ``(symbol "LM358" ...)``.  ``head`` is a
  convenience accessor for the first child when it is a bare atom.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import List, Optional, Union

__all__ = ["Atom", "SExpr", "parse", "parse_many", "dumps"]


@dataclass
class Atom:
    """A leaf token in an S-expression."""

    value: str
    quoted: bool = False

    def __str__(self) -> str:  # pragma: no cover - debugging helper
        return format_atom(self)


Node = Union["Atom", "SExpr"]


@dataclass
class SExpr:
    """A list node, e.g. ``(symbol "LM358" ...)``."""

    children: List[Node] = field(default_factory=list)

    # -- convenience accessors -------------------------------------------------
    @property
    def head(self) -> Optional[str]:
        """The first child's value if it is a bare (unquoted) atom."""
        if self.children and isinstance(self.children[0], Atom):
            return self.children[0].value
        return None

    def find_all(self, head: str) -> List["SExpr"]:
        """Return all direct child lists whose head equals ``head``."""
        return [
            c
            for c in self.children
            if isinstance(c, SExpr) and c.head == head
        ]

    def find(self, head: str) -> Optional["SExpr"]:
        """Return the first direct child list whose head equals ``head``."""
        found = self.find_all(head)
        return found[0] if found else None


# --------------------------------------------------------------------------- #
# Tokenizer
# --------------------------------------------------------------------------- #

class _Tokenizer:
    """Splits raw text into ``(``, ``)`` and atom tokens."""

    def __init__(self, text: str):
        self.text = text
        self.i = 0
        self.n = len(text)

    def tokens(self):
        while self.i < self.n:
            ch = self.text[self.i]
            if ch in " \t\r\n":
                self.i += 1
                continue
            if ch == "(":
                self.i += 1
                yield ("open", None)
            elif ch == ")":
                self.i += 1
                yield ("close", None)
            elif ch == '"':
                yield ("atom", self._read_string())
            else:
                yield ("bare", self._read_bare())

    def _read_string(self) -> str:
        # Consume opening quote
        self.i += 1
        out = []
        while self.i < self.n:
            ch = self.text[self.i]
            if ch == "\\":
                # Preserve recognised escapes; KiCad escapes \" and \\.
                nxt = self.text[self.i + 1] if self.i + 1 < self.n else ""
                if nxt in ('"', "\\"):
                    out.append(nxt)
                    self.i += 2
                    continue
                out.append(ch)
                self.i += 1
                continue
            if ch == '"':
                self.i += 1
                return "".join(out)
            out.append(ch)
            self.i += 1
        raise ValueError("Unterminated string literal in S-expression")

    def _read_bare(self) -> str:
        start = self.i
        while self.i < self.n and self.text[self.i] not in ' \t\r\n()"':
            self.i += 1
        return self.text[start:self.i]


# --------------------------------------------------------------------------- #
# Parser
# --------------------------------------------------------------------------- #

def _parse_tokens(token_iter) -> List[SExpr]:
    """Parse a stream of tokens into a list of top-level S-expressions."""
    stack: List[SExpr] = []
    top: List[SExpr] = []

    for kind, value in token_iter:
        if kind == "open":
            node = SExpr()
            if stack:
                stack[-1].children.append(node)
            else:
                top.append(node)
            stack.append(node)
        elif kind == "close":
            if not stack:
                raise ValueError("Unbalanced ')' in S-expression")
            stack.pop()
        elif kind == "atom":
            atom = Atom(value, quoted=True)
            _append_atom(stack, top, atom)
        elif kind == "bare":
            atom = Atom(value, quoted=False)
            _append_atom(stack, top, atom)

    if stack:
        raise ValueError("Unbalanced '(' in S-expression")
    return top


def _append_atom(stack, top, atom):
    if not stack:
        raise ValueError("Atom outside of any list: %r" % atom.value)
    stack[-1].children.append(atom)


def parse(text: str) -> SExpr:
    """Parse ``text`` and return the single top-level S-expression."""
    nodes = parse_many(text)
    if len(nodes) != 1:
        raise ValueError(
            "Expected exactly one top-level S-expression, found %d"
            % len(nodes)
        )
    return nodes[0]


def parse_many(text: str) -> List[SExpr]:
    """Parse ``text`` and return all top-level S-expressions."""
    return _parse_tokens(_Tokenizer(text).tokens())


# --------------------------------------------------------------------------- #
# Serializer
# --------------------------------------------------------------------------- #

_QUOTE_FORCE = set(' \t\r\n()"')


def format_atom(atom: Atom) -> str:
    """Render a single atom, quoting/escaping as required."""
    if atom.quoted or atom.value == "" or any(
        c in _QUOTE_FORCE for c in atom.value
    ):
        escaped = atom.value.replace("\\", "\\\\").replace('"', '\\"')
        return '"%s"' % escaped
    return atom.value


def _is_simple_list(node: SExpr) -> bool:
    """A list whose children are all atoms can be rendered on one line."""
    return all(isinstance(c, Atom) for c in node.children)


def _dump(node: Node, indent: int, out: List[str]) -> None:
    pad = "  " * indent
    if isinstance(node, Atom):
        out.append(pad + format_atom(node))
        return

    if _is_simple_list(node):
        inner = " ".join(format_atom(c) for c in node.children)
        out.append("%s(%s)" % (pad, inner))
        return

    # Mixed list: head (and any leading atoms) on the opening line, then each
    # child list on its own indented line.
    parts = ["%s(" % pad]
    leading = []
    idx = 0
    while idx < len(node.children) and isinstance(node.children[idx], Atom):
        leading.append(format_atom(node.children[idx]))
        idx += 1
    parts[0] += " ".join(leading)
    out.append(parts[0])

    for child in node.children[idx:]:
        _dump(child, indent + 1, out)

    out.append("%s)" % pad)


def dumps(node: SExpr) -> str:
    """Pretty-print an S-expression tree back to text (trailing newline)."""
    out: List[str] = []
    _dump(node, 0, out)
    return "\n".join(out) + "\n"
