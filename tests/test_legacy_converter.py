from kandle_core import legacy_converter, sexpr
from kandle_core.legacy_converter import Legacy, _readlines, mils_to_mm


def test_mils_to_mm():
    assert abs(mils_to_mm(100) - 2.54) < 1e-9


def test_parse_definition(sample_lib):
    legacy = Legacy()
    legacy.convert(_readlines(sample_lib))
    assert legacy.definition.name == "LM358"
    assert legacy.definition.reference == "U"
    assert legacy.definition.num_units == 2


def test_parse_primitives(sample_lib):
    legacy = Legacy()
    legacy.convert(_readlines(sample_lib))
    assert len(legacy.pins) == 3
    assert len(legacy.rectangles) == 1
    assert len(legacy.circles) == 1
    # F0..F3 -> four information fields.
    assert len(legacy.info) == 4


def test_convert_produces_valid_kicad_sym(sample_lib):
    out = legacy_converter.convert_lib_text(sample_lib)
    node = sexpr.parse(out)
    assert node.head == "kicad_symbol_lib"
    symbol = node.find("symbol")
    assert symbol.children[1].value == "LM358"


def test_pins_converted(sample_lib):
    out = legacy_converter.convert_lib_text(sample_lib)
    # Three X lines become three (pin ...) entries.
    assert out.count("(pin ") == 3
    assert "input" in out and "output" in out


def test_property_keys_present(sample_lib):
    out = legacy_converter.convert_lib_text(sample_lib)
    for key in ("Reference", "Value", "Footprint", "Datasheet"):
        assert '"%s"' % key in out


def test_convert_file_roundtrip(tmp_path, sample_lib):
    src = tmp_path / "LM358.lib"
    src.write_text(sample_lib)
    dst = legacy_converter.convert_lib_file(str(src))
    assert dst.endswith(".kicad_sym")
    node = sexpr.parse(open(dst).read())
    assert node.find("symbol").children[1].value == "LM358"
