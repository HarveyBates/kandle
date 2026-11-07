import os

from kandle_core import project
from kandle_core.manager import ComponentManager
from kandle_core.symbol_library import SymbolLibrary, _find_property
from conftest import make_snapeda_zip, make_cse_zip


def _mgr(kicad_project):
    return ComponentManager(kicad_project)


def test_import_snapeda_full(kicad_project, tmp_path):
    zip_path = make_snapeda_zip(str(tmp_path / "ESP32-WROOM.zip"))
    mgr = _mgr(kicad_project)
    res = mgr.import_zip(zip_path, "microcontrollers")

    assert res.component_name == "ESP32_WROOM"
    assert res.symbol_imported
    assert res.footprint_imported
    assert res.model_imported
    assert res.symbol_lib_registered
    assert res.footprint_lib_registered

    # Symbol file created and contains renamed component.
    lib_path = os.path.join(
        kicad_project, project.SYMBOLS_DIR, "microcontrollers.kicad_sym")
    lib = SymbolLibrary.load(lib_path)
    assert lib.symbol_names() == ["ESP32_WROOM"]

    # Footprint linked.
    prop = _find_property(lib.get_symbol("ESP32_WROOM"), "Footprint")
    assert prop.children[2].value == "microcontrollers:ESP32_WROOM"

    # Footprint + model files exist.
    assert os.path.exists(os.path.join(
        kicad_project, project.FOOTPRINTS_DIR,
        "microcontrollers.pretty", "ESP32_WROOM.kicad_mod"))
    assert os.path.exists(os.path.join(
        kicad_project, project.MODELS_DIR,
        "microcontrollers", "ESP32_WROOM.step"))

    # Lib tables written.
    assert os.path.exists(os.path.join(kicad_project, "sym-lib-table"))
    assert os.path.exists(os.path.join(kicad_project, "fp-lib-table"))


def test_import_uses_legacy_3dmodels_dir(kicad_project, tmp_path):
    # A project that already has the legacy "3dmodels" directory should keep
    # using it instead of creating a new "models" directory.
    legacy = os.path.join(kicad_project, project.MODELS_DIR_LEGACY)
    os.makedirs(legacy)

    zip_path = make_snapeda_zip(str(tmp_path / "ESP32-WROOM.zip"))
    mgr = _mgr(kicad_project)
    res = mgr.import_zip(zip_path, "microcontrollers")
    assert res.model_imported

    assert os.path.exists(os.path.join(
        legacy, "microcontrollers", "ESP32_WROOM.step"))
    assert not os.path.exists(os.path.join(
        kicad_project, project.MODELS_DIR, "microcontrollers"))


def test_import_with_custom_name(kicad_project, tmp_path):
    zip_path = make_snapeda_zip(str(tmp_path / "ESP32-WROOM.zip"))
    mgr = _mgr(kicad_project)
    res = mgr.import_zip(zip_path, "mcus", component_name="My ESP-32")
    assert res.component_name == "My_ESP_32"
    assert mgr.components("mcus") == ["My_ESP_32"]


def test_import_cse_legacy_conversion(kicad_project, tmp_path):
    zip_path = make_cse_zip(str(tmp_path / "LIB_ATMEGA328.zip"))
    mgr = _mgr(kicad_project)
    res = mgr.import_zip(zip_path, "microcontrollers")
    assert res.component_name == "ATMEGA328"
    assert res.symbol_imported
    # Legacy symbol converted; the symbol id keeps the .lib DEF name (LM358).
    assert mgr.components("microcontrollers") == ["ATMEGA328"]


def test_import_two_into_same_library(kicad_project, tmp_path):
    mgr = _mgr(kicad_project)
    z1 = make_snapeda_zip(str(tmp_path / "PartA.zip"), name="PartA")
    z2 = make_snapeda_zip(str(tmp_path / "PartB.zip"), name="PartB")
    mgr.import_zip(z1, "misc")
    mgr.import_zip(z2, "misc")
    assert mgr.components("misc") == ["PartA", "PartB"]
    # Only one sym-lib-table entry for the shared library.
    text = open(os.path.join(kicad_project, "sym-lib-table")).read()
    assert text.count('(name "misc")') == 1


def test_duplicate_import_warns(kicad_project, tmp_path):
    mgr = _mgr(kicad_project)
    z = make_snapeda_zip(str(tmp_path / "Dup.zip"), name="Dup")
    mgr.import_zip(z, "misc")
    res = mgr.import_zip(z, "misc")
    assert any("already exists" in w for w in res.warnings)


def test_libraries_and_components_listing(kicad_project, tmp_path):
    mgr = _mgr(kicad_project)
    assert mgr.libraries() == []
    z = make_snapeda_zip(str(tmp_path / "X.zip"), name="X")
    mgr.import_zip(z, "connectors")
    assert mgr.libraries() == ["connectors"]
    assert mgr.components("connectors") == ["X"]


def test_rename_component(kicad_project, tmp_path):
    mgr = _mgr(kicad_project)
    z = make_snapeda_zip(str(tmp_path / "Old.zip"), name="Old")
    mgr.import_zip(z, "misc")
    mgr.rename_component("misc", "Old", "New-Name")
    assert mgr.components("misc") == ["New_Name"]

    # Footprint file renamed, old gone.
    fp_dir = os.path.join(
        kicad_project, project.FOOTPRINTS_DIR, "misc.pretty")
    assert os.path.exists(os.path.join(fp_dir, "New_Name.kicad_mod"))
    assert not os.path.exists(os.path.join(fp_dir, "Old.kicad_mod"))

    # Model renamed.
    model_dir = os.path.join(kicad_project, project.MODELS_DIR, "misc")
    assert os.path.exists(os.path.join(model_dir, "New_Name.step"))

    # Footprint link updated.
    lib = SymbolLibrary.load(os.path.join(
        kicad_project, project.SYMBOLS_DIR, "misc.kicad_sym"))
    prop = _find_property(lib.get_symbol("New_Name"), "Footprint")
    assert prop.children[2].value == "misc:New_Name"


def test_delete_component(kicad_project, tmp_path):
    mgr = _mgr(kicad_project)
    z = make_snapeda_zip(str(tmp_path / "Doomed.zip"), name="Doomed")
    mgr.import_zip(z, "misc")
    assert mgr.delete_component("misc", "Doomed") is True
    assert mgr.components("misc") == []

    fp = os.path.join(
        kicad_project, project.FOOTPRINTS_DIR, "misc.pretty",
        "Doomed.kicad_mod")
    assert not os.path.exists(fp)
    model = os.path.join(
        kicad_project, project.MODELS_DIR, "misc", "Doomed.step")
    assert not os.path.exists(model)
    # Deleting again is a no-op.
    assert mgr.delete_component("misc", "Doomed") is False


def test_delete_one_of_many(kicad_project, tmp_path):
    mgr = _mgr(kicad_project)
    mgr.import_zip(make_snapeda_zip(
        str(tmp_path / "A.zip"), name="A"), "misc")
    mgr.import_zip(make_snapeda_zip(
        str(tmp_path / "B.zip"), name="B"), "misc")
    mgr.delete_component("misc", "A")
    assert mgr.components("misc") == ["B"]
