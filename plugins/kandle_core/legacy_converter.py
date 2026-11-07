"""Convert legacy KiCad v4/v5 symbol libraries (``.lib``) to v6
(``.kicad_sym``).

This is a faithful Python port of the C++ ``Legacy`` parser (``legacy.cpp``)
and ``Symbol`` writer (``release.cpp``).  The output text matches the format
produced by the original tool so that downstream behaviour is unchanged.

The ``.lib`` format is a line-oriented, token-based format.  Each component
starts with a ``DEF`` line and ends with ``ENDDEF``; the drawing primitives are
single letters (``F`` field, ``X`` pin, ``S`` rectangle, ``P`` polygon,
``C`` circle, ``A`` arc, ``T`` text).
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from typing import List, Optional, Tuple

__all__ = ["convert_lib_text", "convert_lib_file"]


# --------------------------------------------------------------------------- #
# Small helpers (ported from utils.cpp)
# --------------------------------------------------------------------------- #

def mils_to_mm(mils: float) -> float:
    return float(mils) * 0.0254


def _readlines(text: str) -> List[str]:
    """Return non-empty, non-comment lines (comments begin with '#')."""
    lines = []
    for raw in text.splitlines():
        if not raw:
            continue
        if raw[0] == "#":
            continue
        lines.append(raw)
    return lines


def _replace_empty_quotes(line: str) -> str:
    return line.replace('""', '"None"')


# --------------------------------------------------------------------------- #
# Parsed data structures
# --------------------------------------------------------------------------- #

@dataclass
class Definition:
    name: str = ""
    reference: str = ""
    pin_name_offset: int = 0
    show_pin_number: str = "N"
    show_pin_name: str = "N"
    num_units: int = 0
    pwr_cmp: str = "N"


@dataclass
class Information:
    text: str = ""
    pos_x: int = 0
    pos_y: int = 0
    font_size: int = 0
    orientation: str = "H"
    visibility: str = "V"
    horizontal_justification: str = "C"
    vertical_justification: str = "C"
    italic: str = "N"
    bold: str = "N"
    field_name: str = ""


@dataclass
class Pin:
    name: str = ""
    number: str = ""
    pos_x: int = 0
    pos_y: int = 0
    length: int = 0
    orientation: str = "U"
    text_num_size: int = 0
    text_name_size: int = 0
    unit: int = 0
    convert: int = 0
    electric_type: str = "U"
    shape: str = ""


@dataclass
class Rectangle:
    start_x: int = 0
    start_y: int = 0
    end_x: int = 0
    end_y: int = 0
    unit: int = 0
    convert: int = 0
    thickness: int = 0
    background: str = "N"


@dataclass
class Polygon:
    n_points: int = 0
    parts: int = 0
    convert: int = 0
    thickness: int = 0
    coords: List[Tuple[int, int]] = field(default_factory=list)
    background: str = "N"


@dataclass
class Circle:
    pos_x: int = 0
    pos_y: int = 0
    radius: int = 0
    unit: int = 0
    convert: int = 0
    thickness: int = 0
    background: str = "N"


@dataclass
class Arc:
    pos_x: int = 0
    pos_y: int = 0
    radius: int = 0
    start_angle: int = 0
    end_angle: int = 0
    part: int = 0
    convert: int = 0
    thickness: int = 0
    background: str = "N"
    start_point_x: int = 0
    start_point_y: int = 0
    end_point_x: int = 0
    end_point_y: int = 0


@dataclass
class Text:
    orientation: int = 0
    pos_x: int = 0
    pos_y: int = 0
    dimension: int = 0
    unit: int = 0
    convert: int = 0
    text: str = ""


# --------------------------------------------------------------------------- #
# Parser (port of Legacy)
# --------------------------------------------------------------------------- #

class LegacyParseError(Exception):
    pass


class Legacy:
    def __init__(self) -> None:
        self.definition = Definition()
        self.info: List[Information] = []
        self.pins: List[Pin] = []
        self.rectangles: List[Rectangle] = []
        self.polygons: List[Polygon] = []
        self.circles: List[Circle] = []
        self.arcs: List[Arc] = []
        self.texts: List[Text] = []

    def convert(self, lines: List[str]) -> bool:
        found_definition = False
        for line in lines:
            tokens = line.split()
            if not tokens:
                continue
            if not found_definition:
                if "DEF" in tokens:
                    self._parse_definition(line)
                    found_definition = True
            else:
                self._identify(tokens[0], line)
        return True

    def _identify(self, token: str, line: str) -> bool:
        # Tokens longer than 2 chars are user input, not primitive identifiers.
        if len(token) > 2:
            return True
        handler = {
            "F": self._parse_information,
            "X": self._parse_pin,
            "S": self._parse_rectangle,
            "P": self._parse_polygon,
            "C": self._parse_circle,
            "A": self._parse_arc,
            "T": self._parse_text,
        }.get(token[0])
        if handler is None:
            return False
        return handler(line)

    def _parse_definition(self, line: str) -> bool:
        # DEF name reference 0 pin_name_offset show_num show_name num_units
        #     <skip> pwr_cmp
        t = line.split()
        if len(t) < 10 or t[0] != "DEF":
            raise LegacyParseError("Malformed DEF line: %r" % line)
        d = self.definition
        d.name = t[1]
        d.reference = t[2]
        d.pin_name_offset = int(t[4])
        d.show_pin_number = t[5]
        d.show_pin_name = t[6]
        d.num_units = int(t[7])
        d.pwr_cmp = t[9]
        return True

    def _parse_information(self, line: str) -> bool:
        line_cpy = _replace_empty_quotes(line)
        # F<n> "text" x y size orient vis hjust vjust+italic+bold ["fieldname"]
        m = re.match(
            r'^F\S*\s+"([^"]*)"\s+(-?\d+)\s+(-?\d+)\s+(\d+)\s+(\S)\s+(\S)'
            r'\s+(\S)\s+(\S)(\S)(\S)(?:\s+"([^"]*)")?',
            line_cpy,
        )
        if not m:
            return False
        ci = Information(
            text=m.group(1),
            pos_x=int(m.group(2)),
            pos_y=int(m.group(3)),
            font_size=int(m.group(4)),
            orientation=m.group(5),
            visibility=m.group(6),
            horizontal_justification=m.group(7),
            vertical_justification=m.group(8),
            italic=m.group(9),
            bold=m.group(10),
            field_name=m.group(11) or "",
        )
        self.info.append(ci)
        return True

    def _parse_pin(self, line: str) -> bool:
        # X name number x y length orient num_size name_size unit convert
        #   electric_type [shape]
        t = line.split()
        if len(t) < 11:
            # Mirrors the C++ guard: only fail if a name was captured.
            if len(t) >= 2 and t[1]:
                return False
            return True
        pin = Pin(
            name=t[1],
            number=t[2],
            pos_x=int(t[3]),
            pos_y=int(t[4]),
            length=int(t[5]),
            orientation=t[6],
            text_num_size=int(t[7]),
            text_name_size=int(t[8]),
            unit=int(t[9]),
            convert=int(t[10]),
            electric_type=t[11] if len(t) > 11 else "U",
            shape=t[12] if len(t) > 12 else "",
        )
        self.pins.append(pin)
        return True

    def _parse_rectangle(self, line: str) -> bool:
        t = line.split()
        if len(t) < 9:
            return False
        self.rectangles.append(Rectangle(
            start_x=int(t[1]), start_y=int(t[2]),
            end_x=int(t[3]), end_y=int(t[4]),
            unit=int(t[5]), convert=int(t[6]),
            thickness=int(t[7]), background=t[8],
        ))
        return True

    def _parse_polygon(self, line: str) -> bool:
        t = line.split()
        if len(t) < 5:
            return False
        poly = Polygon(
            n_points=int(t[1]), parts=int(t[2]),
            convert=int(t[3]), thickness=int(t[4]),
        )
        coords_tokens = t[5:]
        coords: List[Tuple[int, int]] = []
        for i in range(poly.n_points):
            xi = 2 * i
            yi = 2 * i + 1
            if yi >= len(coords_tokens):
                break
            try:
                coords.append(
                    (int(coords_tokens[xi]), int(coords_tokens[yi]))
                )
            except ValueError:
                break
        poly.coords = coords
        poly.background = t[-1]
        self.polygons.append(poly)
        return True

    def _parse_circle(self, line: str) -> bool:
        t = line.split()
        if len(t) < 8:
            return False
        self.circles.append(Circle(
            pos_x=int(t[1]), pos_y=int(t[2]), radius=int(t[3]),
            unit=int(t[4]), convert=int(t[5]),
            thickness=int(t[6]), background=t[7],
        ))
        return True

    def _parse_arc(self, line: str) -> bool:
        t = line.split()
        if len(t) < 14:
            return False
        self.arcs.append(Arc(
            pos_x=int(t[1]), pos_y=int(t[2]), radius=int(t[3]),
            start_angle=int(t[4]), end_angle=int(t[5]),
            part=int(t[6]), convert=int(t[7]), thickness=int(t[8]),
            background=t[9],
            start_point_x=int(t[10]), start_point_y=int(t[11]),
            end_point_x=int(t[12]), end_point_y=int(t[13]),
        ))
        return True

    def _parse_text(self, line: str) -> bool:
        t = line.split()
        if len(t) < 7:
            return False
        self.texts.append(Text(
            orientation=int(t[1]), pos_x=int(t[2]), pos_y=int(t[3]),
            dimension=int(t[4]), unit=int(t[5]), convert=int(t[6]),
            text=t[7] if len(t) > 7 else "",
        ))
        return True


# --------------------------------------------------------------------------- #
# Writer (port of Symbol)
# --------------------------------------------------------------------------- #

KICAD_VERSION = "20211014"
KICAD_GENERATOR = "kicad_symbol_editor"


def _assert_true(c: str) -> bool:
    return c == "Y"


def _build_font(font_size: int, bold: str = "N", italic: str = "N") -> str:
    size = mils_to_mm(font_size)
    out = "(font (size %.3f %.3f)" % (size, size)
    if bold == "B":
        out += " bold"
    if italic == "I":
        out += " italic"
    out += ")"
    return out


def _add_justification(identifier: str) -> str:
    return {
        "L": "left", "R": "right", "B": "bottom", "T": "top",
    }.get(identifier, "")


def _build_text_justification(info: Information) -> str:
    if info.horizontal_justification != "C":
        return "(justify %s %s)" % (
            _add_justification(info.horizontal_justification),
            _add_justification(info.vertical_justification),
        )
    return ""


def _build_pins_definition(d: Definition) -> str:
    out = ""
    if not _assert_true(d.show_pin_number):
        out += "(pin_numbers hide) "
    offset = mils_to_mm(d.pin_name_offset)
    if _assert_true(d.show_pin_name):
        out += "(pin_names (offset %.3f))" % offset
    else:
        out += "(pin_names (offset %.3f) hide)" % offset
    return out


_PIN_TYPES = {
    "I": "input", "O": "output", "B": "bidirectional", "T": "tri_state",
    "P": "passive", "U": "unspecified", "W": "power_in", "w": "power_out",
    "C": "open_collector", "E": "open_emitter", "N": "free",
}

_PIN_ORIENTATIONS = {"D": 270, "R": 0, "L": 180, "U": 90}


def _get_pin_type(identifier: str) -> str:
    return _PIN_TYPES.get(identifier, "unspecified")


def _get_pin_orientation(identifier: str) -> int:
    return _PIN_ORIENTATIONS.get(identifier, 90)


def _get_pin_shape(shape_buf: str) -> str:
    if not shape_buf:
        return "line"
    offset = 0
    if shape_buf[0] == "N":
        offset = 1
    if offset >= len(shape_buf):
        return "line"
    identifier = shape_buf[offset]
    offset += 1
    nxt = shape_buf[offset] if offset < len(shape_buf) else ""
    if identifier == "I":
        return "inverted"
    if identifier == "C":
        if nxt == "I":
            return "inverted_clock"
        if nxt == "L":
            return "clock_low"
        return "clock"
    if identifier == "L":
        return "input_low"
    if identifier == "V":
        return "output_low"
    if identifier == "F":
        return "edge_clock_high"
    if identifier == "X":
        return "non_logic"
    return "line"


def _fill_type(background: str) -> str:
    return "background" if background in ("F", "f") else "none"


class SymbolWriter:
    def __init__(self) -> None:
        self.lines: List[str] = []

    def build(self, legacy: Legacy) -> str:
        self._header()
        self._symbol(legacy)
        self._properties(legacy)
        self._graphics(legacy)
        self.lines.append("  )\n)")
        return "\n".join(self.lines) + "\n"

    def _header(self) -> None:
        self.lines.append(
            "(kicad_symbol_lib (version %s) (generator %s)"
            % (KICAD_VERSION, KICAD_GENERATOR)
        )

    def _symbol(self, legacy: Legacy) -> None:
        pins_def = _build_pins_definition(legacy.definition)
        self.lines.append(
            '  (symbol "%s" %s (in_bom yes) (on_board yes)'
            % (legacy.definition.name, pins_def)
        )

    def _properties(self, legacy: Legacy) -> None:
        keys = ["Reference", "Value", "Footprint", "Datasheet"]
        for i, info in enumerate(legacy.info):
            key = keys[i] if i < len(keys) else info.field_name
            pos_x = mils_to_mm(info.pos_x)
            pos_y = mils_to_mm(info.pos_y)
            font_buf = _build_font(info.font_size, info.bold, info.italic)
            justify_buf = _build_text_justification(info)
            block = (
                '    (property "%s" "%s" (id %d) (at %.2f %.2f 0)\n'
                "      (effects %s %s"
                % (key, info.text, i, pos_x, pos_y, font_buf, justify_buf)
            )
            if info.visibility == "V":
                block += ")\n    )"
            else:
                block += " hide)\n    )"
            self.lines.append(block)

    def _graphics(self, legacy: Legacy) -> None:
        self.lines.append('    (symbol "%s_0_0"' % legacy.definition.name)
        self._polygons(legacy.polygons)
        self._circles(legacy.circles)
        self._arcs(legacy.arcs)
        self._rectangles(legacy.rectangles)
        self._texts(legacy.texts)
        self._pins(legacy)
        self.lines.append("    )")

    def _polygons(self, polygons: List[Polygon]) -> None:
        for poly in polygons:
            pts = ""
            for (x, y) in poly.coords:
                pts += "          (xy %.3f %.3f)\n" % (
                    mils_to_mm(x), mils_to_mm(y)
                )
            stroke = mils_to_mm(poly.thickness)
            self.lines.append(
                "      (polyline\n"
                "        (pts\n"
                "%s"
                "        )\n"
                "        (stroke (width %.3f) (type default))\n"
                "        (fill (type %s))\n"
                "      )" % (pts, stroke, _fill_type(poly.background))
            )

    def _circles(self, circles: List[Circle]) -> None:
        for c in circles:
            self.lines.append(
                "      (circle (center %.3f %.3f) (radius %.3f)\n"
                "        (stroke (width %.3f) (type default)) (fill (type %s))\n"
                "      )" % (
                    mils_to_mm(c.pos_x), mils_to_mm(c.pos_y),
                    mils_to_mm(c.radius), mils_to_mm(c.thickness),
                    _fill_type(c.background),
                )
            )

    def _arcs(self, arcs: List[Arc]) -> None:
        for a in arcs:
            start_x = mils_to_mm(a.start_point_x)
            start_y = mils_to_mm(a.start_point_y)
            mid_x = mils_to_mm(a.pos_x)
            mid_y = mils_to_mm(a.pos_y)
            end_x = mils_to_mm(a.end_point_x)
            end_y = mils_to_mm(a.end_point_y)
            radius = mils_to_mm(a.radius)
            stroke = mils_to_mm(a.thickness)
            self.lines.append(
                "      (arc (start %.3f %.3f) (mid %.3f %.3f) (end %.3f %.3f)\n"
                "        (stroke (width %.3f) (type default))\n"
                "        (fill (type %s))\n"
                "      )" % (
                    start_x, start_y, (mid_x + radius), mid_y,
                    end_x, end_y, stroke, _fill_type(a.background),
                )
            )

    def _rectangles(self, rectangles: List[Rectangle]) -> None:
        for r in rectangles:
            self.lines.append(
                "      (rectangle (start %.3f %.3f) (end %.3f %.3f)\n"
                "        (stroke (width %.3f) (type default)) (fill (type %s))\n"
                "      )" % (
                    mils_to_mm(r.start_x), mils_to_mm(r.start_y),
                    mils_to_mm(r.end_x), mils_to_mm(r.end_y),
                    mils_to_mm(r.thickness), _fill_type(r.background),
                )
            )

    def _texts(self, texts: List[Text]) -> None:
        for t in texts:
            effects = _build_font(t.dimension)
            self.lines.append(
                '      (text "%s" (at %.3f %.3f %.3f)\n'
                "        (effects %s)\n"
                "      )" % (
                    t.text, mils_to_mm(t.pos_x), mils_to_mm(t.pos_y),
                    mils_to_mm(t.orientation), effects,
                )
            )

    def _pins(self, legacy: Legacy) -> None:
        for pin in legacy.pins:
            pin_type = _get_pin_type(pin.electric_type)
            pin_shape = _get_pin_shape(pin.shape)
            pos_x = mils_to_mm(pin.pos_x)
            pos_y = mils_to_mm(pin.pos_y)
            orientation = _get_pin_orientation(pin.orientation)
            length = mils_to_mm(pin.length)
            font_name = _build_font(pin.text_name_size)
            font_num = _build_font(pin.text_num_size)
            self.lines.append(
                "      (pin %s %s (at %.3f %.3f %d) (length %.3f)\n"
                '        (name "%s" (effects %s))\n'
                '        (number "%s" (effects %s))\n'
                "      )" % (
                    pin_type, pin_shape, pos_x, pos_y, orientation, length,
                    pin.name, font_name, pin.number, font_num,
                )
            )


# --------------------------------------------------------------------------- #
# Public API
# --------------------------------------------------------------------------- #

def convert_lib_text(text: str) -> str:
    """Convert the contents of a ``.lib`` file to ``.kicad_sym`` text."""
    legacy = Legacy()
    legacy.convert(_readlines(text))
    return SymbolWriter().build(legacy)


def convert_lib_file(src_path: str, dst_path: Optional[str] = None) -> str:
    """Convert a ``.lib`` file on disk; returns the destination path.

    If ``dst_path`` is omitted the output is written alongside the source with
    a ``.kicad_sym`` extension.
    """
    import os

    with open(src_path, "r", encoding="utf-8", errors="replace") as fh:
        text = fh.read()
    converted = convert_lib_text(text)
    if dst_path is None:
        root, _ = os.path.splitext(src_path)
        dst_path = root + ".kicad_sym"
    with open(dst_path, "w", encoding="utf-8") as fh:
        fh.write(converted)
    return dst_path
