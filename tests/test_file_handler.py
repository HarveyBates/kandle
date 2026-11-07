import os
import zipfile

import pytest

from kandle_core import file_handler
from conftest import make_snapeda_zip, make_cse_zip


def test_extract_snapeda(tmp_path):
    zip_path = make_snapeda_zip(str(tmp_path / "ESP32-WROOM.zip"))
    out = file_handler.extract_zip(zip_path, str(tmp_path / "tmp"))
    # Name is sanitized: '-' -> '_'.
    assert os.path.basename(out) == "ESP32_WROOM"
    files = file_handler.classify(out)
    assert files.symbol and files.symbol.endswith(".kicad_sym")
    assert files.footprint and files.footprint.endswith(".kicad_mod")
    assert files.model and files.model.endswith(".step")
    assert files.legacy_symbol is None


def test_extract_cse_prefers_kicad_dir(tmp_path):
    zip_path = make_cse_zip(str(tmp_path / "LIB_ATMEGA328.zip"))
    out = file_handler.extract_zip(zip_path, str(tmp_path / "tmp"))
    assert os.path.basename(out) == "ATMEGA328"
    files = file_handler.classify(out)
    # CSE ships a legacy .lib under KiCad/.
    assert files.symbol is None
    assert files.legacy_symbol and files.legacy_symbol.endswith(".lib")
    assert files.footprint and files.footprint.endswith(".kicad_mod")
    assert files.model and files.model.endswith(".step")


def test_extract_reuses_existing_dir(tmp_path):
    zip_path = make_snapeda_zip(str(tmp_path / "Part.zip"))
    dest = str(tmp_path / "tmp")
    first = file_handler.extract_zip(zip_path, dest)
    second = file_handler.extract_zip(zip_path, dest)
    assert first == second


def test_non_zip_rejected(tmp_path):
    bad = tmp_path / "not.zip"
    bad.write_text("hello")
    with pytest.raises(ValueError):
        file_handler.extract_zip(str(bad), str(tmp_path / "tmp"))


def test_missing_file(tmp_path):
    with pytest.raises(FileNotFoundError):
        file_handler.extract_zip(str(tmp_path / "nope.zip"), str(tmp_path))


def test_zip_slip_rejected(tmp_path):
    zip_path = str(tmp_path / "evil.zip")
    with zipfile.ZipFile(zip_path, "w") as zf:
        zf.writestr("../escape.kicad_sym", "x")
    with pytest.raises(ValueError):
        file_handler.extract_zip(zip_path, str(tmp_path / "tmp"))
