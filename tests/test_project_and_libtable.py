import os

from kandle_core import lib_table, project


def test_sanitize_name():
    assert project.sanitize_name("ul_My Part-1.zip") == "My_Part_1"
    assert project.sanitize_name("LIB_PESD 0402-140.zip") == "PESD_0402_140"
    assert project.sanitize_name("/a/b/LM358.zip") == "LM358"


def test_constrain_footprint_text():
    line = "    (effects (font (size 1.5 1.5) (thickness 0.3)))"
    out = project.constrain_footprint_text(line)
    assert "(size 1 1)" in out
    assert "(thickness 0.15)" in out


def test_find_project_file(kicad_project):
    assert project.is_kicad_project(kicad_project)
    assert project.find_project_file(kicad_project).endswith(".kicad_pro")


def test_initialise_creates_dirs(kicad_project):
    created = project.initialise(kicad_project)
    assert project.SYMBOLS_DIR in created
    assert os.path.isdir(os.path.join(kicad_project, project.FOOTPRINTS_DIR))
    # Idempotent: second run creates nothing.
    assert project.initialise(kicad_project) == []


def test_register_symbol_lib(kicad_project):
    added = lib_table.register_symbol_lib(
        kicad_project, "op_amps",
        "components/extern/symbols/op_amps.kicad_sym",
    )
    assert added is True
    path = os.path.join(kicad_project, "sym-lib-table")
    text = open(path).read()
    assert "op_amps" in text
    assert "${KIPRJMOD}/components/extern/symbols/op_amps.kicad_sym" in text
    # Idempotent.
    assert lib_table.register_symbol_lib(
        kicad_project, "op_amps",
        "components/extern/symbols/op_amps.kicad_sym",
    ) is False


def test_register_footprint_lib(kicad_project):
    assert lib_table.register_footprint_lib(
        kicad_project, "op_amps",
        "components/extern/footprints/op_amps.pretty",
    )
    text = open(os.path.join(kicad_project, "fp-lib-table")).read()
    assert "op_amps.pretty" in text


def test_lib_table_remove(kicad_project):
    path = os.path.join(kicad_project, "sym-lib-table")
    lib_table.register_symbol_lib(
        kicad_project, "a", "components/extern/symbols/a.kicad_sym")
    lib_table.register_symbol_lib(
        kicad_project, "b", "components/extern/symbols/b.kicad_sym")
    table = lib_table.LibTable.load_or_empty(path, "sym_lib_table")
    assert set(table.lib_names()) == {"a", "b"}
    assert table.remove_lib("a") is True
    assert table.lib_names() == ["b"]
