"""Shared fixtures and sample data for the Kandle backend tests."""

import os
import sys
import zipfile

import pytest

# Make the plugin package importable without installing it.
PLUGIN_ROOT = os.path.abspath(
    os.path.join(os.path.dirname(__file__), "..", "plugins")
)
if PLUGIN_ROOT not in sys.path:
    sys.path.insert(0, PLUGIN_ROOT)


# A minimal but realistic legacy v5 symbol library.
SAMPLE_LIB = """EESchema-LIBRARY Version 2.4
#encoding utf-8
#
# LM358
#
DEF LM358 U 0 40 Y Y 2 F N
F0 "U" -100 200 50 H V L CNN
F1 "LM358" 100 -200 50 H V L CNN
F2 "" 0 0 50 H I C CNN
F3 "" 0 0 50 H I C CNN
DRAW
S -200 150 200 -150 0 1 10 f
X ~ 1 -300 100 100 R 50 50 1 1 I
X ~ 2 -300 -100 100 R 50 50 1 1 I
X ~ 3 300 0 100 L 50 50 1 1 O
C 0 0 20 0 1 0 N
ENDDRAW
ENDDEF
#
#End Library
"""


# A minimal modern v6 symbol library (SnapEDA / Ultra Librarian style).
SAMPLE_KICAD_SYM = """(kicad_symbol_lib (version 20211014) (generator kicad_symbol_editor)
  (symbol "ESP32-WROOM" (in_bom yes) (on_board yes)
    (property "Reference" "U" (id 0) (at 0 0 0)
      (effects (font (size 1.27 1.27)))
    )
    (property "Value" "ESP32-WROOM" (id 1) (at 0 -2.54 0)
      (effects (font (size 1.27 1.27)))
    )
    (property "Footprint" "" (id 2) (at 0 0 0)
      (effects (font (size 1.27 1.27)) hide)
    )
    (symbol "ESP32-WROOM_0_1"
      (rectangle (start -5 5) (end 5 -5)
        (stroke (width 0.254) (type default)) (fill (type background))
      )
    )
    (symbol "ESP32-WROOM_1_1"
      (pin input line (at -7.62 2.54 0) (length 2.54)
        (name "IO0" (effects (font (size 1.27 1.27))))
        (number "1" (effects (font (size 1.27 1.27))))
      )
    )
  )
)
"""


SAMPLE_KICAD_MOD = """(footprint "ORIGINAL_FP" (layer "F.Cu")
  (fp_text reference "REF**" (at 0 0) (layer "F.SilkS")
    (effects (font (size 1.5 1.5) (thickness 0.3)))
  )
  (pad "1" smd rect (at -1 0) (size 1 1) (layers "F.Cu"))
)
"""


def _write(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as fh:
        fh.write(text)


@pytest.fixture
def sample_lib():
    return SAMPLE_LIB


@pytest.fixture
def sample_kicad_sym():
    return SAMPLE_KICAD_SYM


@pytest.fixture
def sample_kicad_mod():
    return SAMPLE_KICAD_MOD


@pytest.fixture
def kicad_project(tmp_path):
    """A temp directory that looks like a KiCad project root."""
    proj = tmp_path / "proj"
    proj.mkdir()
    (proj / "proj.kicad_pro").write_text("(kicad_project)\n")
    return str(proj)


def make_snapeda_zip(dest_zip, name="ESP32-WROOM",
                     sym_text=SAMPLE_KICAD_SYM, mod_text=SAMPLE_KICAD_MOD,
                     model=True):
    """Create a SnapEDA-style archive (flat layout)."""
    with zipfile.ZipFile(dest_zip, "w") as zf:
        zf.writestr("%s.kicad_sym" % name, sym_text)
        zf.writestr("%s.kicad_mod" % name, mod_text)
        if model:
            zf.writestr("%s.step" % name, "ISO-10303-21;\ndummy step\n")
    return dest_zip


def make_cse_zip(dest_zip, name="ATMEGA328", lib_text=SAMPLE_LIB,
                 mod_text=SAMPLE_KICAD_MOD):
    """Create a Component Search Engine-style archive (nested KiCad dir,
    legacy .lib symbol)."""
    with zipfile.ZipFile(dest_zip, "w") as zf:
        zf.writestr("%s/KiCad/%s.lib" % (name, name), lib_text)
        zf.writestr("%s/KiCad/%s.kicad_mod" % (name, name), mod_text)
        zf.writestr("%s/3D/%s.step" % (name, name), "ISO-10303-21;\n")
        zf.writestr("%s/%s.pdf" % (name, name), "%PDF-1.4 dummy")
    return dest_zip


@pytest.fixture
def snapeda_zip(tmp_path):
    return make_snapeda_zip(str(tmp_path / "ESP32-WROOM.zip"))


@pytest.fixture
def cse_zip(tmp_path):
    return make_cse_zip(str(tmp_path / "LIB_ATMEGA328.zip"))
