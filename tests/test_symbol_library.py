import pytest

from kandle_core.symbol_library import (
    SymbolLibrary,
    extract_first_symbol,
    set_symbol_footprint,
)
from kandle_core.symbol_library import _symbol_name


def test_list_symbols(sample_kicad_sym):
    lib = SymbolLibrary.from_text(sample_kicad_sym)
    assert lib.symbol_names() == ["ESP32-WROOM"]


def test_add_symbol(sample_kicad_sym):
    lib = SymbolLibrary.empty()
    sym = extract_first_symbol(sample_kicad_sym)
    lib.add_symbol(sym)
    assert lib.symbol_names() == ["ESP32-WROOM"]


def test_add_duplicate_raises(sample_kicad_sym):
    lib = SymbolLibrary.from_text(sample_kicad_sym)
    sym = extract_first_symbol(sample_kicad_sym)
    with pytest.raises(ValueError):
        lib.add_symbol(sym)


def test_add_overwrite(sample_kicad_sym):
    lib = SymbolLibrary.from_text(sample_kicad_sym)
    sym = extract_first_symbol(sample_kicad_sym)
    lib.add_symbol(sym, overwrite=True)
    assert lib.symbol_names() == ["ESP32-WROOM"]


def test_rename_symbol_renames_child_units(sample_kicad_sym):
    lib = SymbolLibrary.from_text(sample_kicad_sym)
    lib.rename_symbol("ESP32-WROOM", "ESP32_C3")
    assert lib.symbol_names() == ["ESP32_C3"]
    # Child unit symbols must be renamed too.
    top = lib.get_symbol("ESP32_C3")
    child_names = [
        _symbol_name(c)
        for c in top.children
        if getattr(c, "head", None) == "symbol"
    ]
    assert "ESP32_C3_0_1" in child_names
    assert "ESP32_C3_1_1" in child_names


def test_rename_missing_raises(sample_kicad_sym):
    lib = SymbolLibrary.from_text(sample_kicad_sym)
    with pytest.raises(KeyError):
        lib.rename_symbol("NOPE", "X")


def test_rename_to_existing_raises(sample_kicad_sym):
    lib = SymbolLibrary.from_text(sample_kicad_sym)
    sym = extract_first_symbol(sample_kicad_sym)
    from kandle_core.symbol_library import rename_symbol_node
    rename_symbol_node(sym, "ESP32-WROOM", "OTHER")
    lib.add_symbol(sym)
    with pytest.raises(ValueError):
        lib.rename_symbol("OTHER", "ESP32-WROOM")


def test_delete_symbol(sample_kicad_sym):
    lib = SymbolLibrary.from_text(sample_kicad_sym)
    assert lib.delete_symbol("ESP32-WROOM") is True
    assert lib.symbol_names() == []
    assert lib.delete_symbol("ESP32-WROOM") is False


def test_set_footprint(sample_kicad_sym):
    lib = SymbolLibrary.from_text(sample_kicad_sym)
    lib.set_footprint("ESP32-WROOM", "mylib:ESP32")
    sym = lib.get_symbol("ESP32-WROOM")
    from kandle_core.symbol_library import _find_property
    prop = _find_property(sym, "Footprint")
    assert prop.children[2].value == "mylib:ESP32"


def test_set_footprint_when_absent():
    text = (
        '(kicad_symbol_lib (version 1)'
        ' (symbol "X" (property "Value" "X" (id 1))))'
    )
    lib = SymbolLibrary.from_text(text)
    set_symbol_footprint(lib.get_symbol("X"), "lib:X")
    from kandle_core.symbol_library import _find_property
    assert _find_property(lib.get_symbol("X"), "Footprint") is not None


def test_save_and_reload(tmp_path, sample_kicad_sym):
    path = str(tmp_path / "lib.kicad_sym")
    lib = SymbolLibrary.from_text(sample_kicad_sym)
    lib.rename_symbol("ESP32-WROOM", "RENAMED")
    lib.save(path)
    reloaded = SymbolLibrary.load(path)
    assert reloaded.symbol_names() == ["RENAMED"]
