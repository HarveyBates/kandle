"""High-level component manager API.

This is the backend entry point used by both the wxPython UI and the tests.  It
orchestrates extraction, legacy conversion, symbol/footprint/model import and
library-table registration, and exposes list/rename/delete operations so the UI
can act as a component manager.
"""

from __future__ import annotations

import os
import shutil
from dataclasses import dataclass, field
from typing import List, Optional

from . import file_handler, lib_table, project
from . import legacy_converter
from . import sexpr
from .symbol_library import SymbolLibrary, extract_first_symbol

__all__ = ["ComponentManager", "ImportResult"]


@dataclass
class ImportResult:
    component_name: str
    library: str
    symbol_imported: bool = False
    footprint_imported: bool = False
    model_imported: bool = False
    symbol_lib_registered: bool = False
    footprint_lib_registered: bool = False
    warnings: List[str] = field(default_factory=list)


class ComponentManager:
    def __init__(self, project_dir: str):
        self.project_dir = os.path.abspath(project_dir)

    # -- paths -----------------------------------------------------------------
    def _symbols_dir(self) -> str:
        return os.path.join(self.project_dir, project.SYMBOLS_DIR)

    def _footprints_dir(self) -> str:
        return os.path.join(self.project_dir, project.FOOTPRINTS_DIR)

    def _models_dir(self) -> str:
        return os.path.join(self.project_dir, project.MODELS_DIR)

    def _tmp_dir(self) -> str:
        return os.path.join(self.project_dir, project.TMP_DIR)

    def _symbol_lib_path(self, library: str) -> str:
        return os.path.join(self._symbols_dir(), library + ".kicad_sym")

    def _footprint_lib_dir(self, library: str) -> str:
        return os.path.join(self._footprints_dir(), library + ".pretty")

    def _model_lib_dir(self, library: str) -> str:
        return os.path.join(self._models_dir(), library)

    # -- setup -----------------------------------------------------------------
    def initialise(self) -> List[str]:
        return project.initialise(self.project_dir)

    # -- queries ---------------------------------------------------------------
    def libraries(self) -> List[str]:
        """Return existing component-type library names (sorted)."""
        symbols_dir = self._symbols_dir()
        if not os.path.isdir(symbols_dir):
            return []
        names = [
            os.path.splitext(f)[0]
            for f in os.listdir(symbols_dir)
            if f.endswith(".kicad_sym")
        ]
        return sorted(names)

    def components(self, library: str) -> List[str]:
        """Return symbol names within a library (sorted)."""
        path = self._symbol_lib_path(library)
        if not os.path.exists(path):
            return []
        return sorted(SymbolLibrary.load(path).symbol_names())

    # -- import ----------------------------------------------------------------
    def import_zip(self, zip_path: str, library: str,
                   component_name: Optional[str] = None,
                   overwrite: bool = False) -> ImportResult:
        self.initialise()

        extract_dir = file_handler.extract_zip(zip_path, self._tmp_dir())
        files = file_handler.classify(extract_dir)

        if component_name:
            name = project.sanitize_name(component_name)
        else:
            name = project.sanitize_name(zip_path)

        result = ImportResult(component_name=name, library=library)

        # Resolve the symbol file, converting legacy .lib if required.
        symbol_path = files.symbol
        if symbol_path is None and files.legacy_symbol is not None:
            symbol_path = legacy_converter.convert_lib_file(
                files.legacy_symbol
            )

        if symbol_path is not None:
            self._import_symbol(
                symbol_path, library, name, files.footprint is not None,
                overwrite, result,
            )

        if files.footprint is not None:
            self._import_footprint(files.footprint, library, name, result)

        if files.model is not None:
            self._import_model(files.model, library, name, result)

        self._register_libraries(library, files, result)
        return result

    def _import_symbol(self, src_path: str, library: str, name: str,
                       has_footprint: bool, overwrite: bool,
                       result: ImportResult) -> None:
        with open(src_path, "r", encoding="utf-8", errors="replace") as fh:
            text = fh.read()
        symbol_node = extract_first_symbol(text)

        # Rename the symbol (and child units) to the requested component name.
        from .symbol_library import rename_symbol_node, set_symbol_footprint
        current = _node_name(symbol_node)
        if current and current != name:
            rename_symbol_node(symbol_node, current, name)
        elif current is None:
            result.warnings.append("Source symbol had no name")

        if has_footprint:
            set_symbol_footprint(symbol_node, "%s:%s" % (library, name))

        lib_path = self._symbol_lib_path(library)
        os.makedirs(os.path.dirname(lib_path), exist_ok=True)
        if os.path.exists(lib_path):
            lib = SymbolLibrary.load(lib_path)
        else:
            lib = SymbolLibrary.empty()

        if lib.has_symbol(name) and not overwrite:
            result.warnings.append(
                "Component %r already exists in %s" % (name, library)
            )
            return
        lib.add_symbol(symbol_node, overwrite=True)
        lib.save(lib_path)
        result.symbol_imported = True

    def _import_footprint(self, src_path: str, library: str, name: str,
                          result: ImportResult) -> None:
        dest_dir = self._footprint_lib_dir(library)
        os.makedirs(dest_dir, exist_ok=True)
        dest = os.path.join(dest_dir, name + ".kicad_mod")

        with open(src_path, "r", encoding="utf-8", errors="replace") as fh:
            lines = fh.read().splitlines()

        out_lines = []
        for line in lines:
            if not line:
                continue
            if "(effects (font (size " in line:
                line = project.constrain_footprint_text(line)
            out_lines.append(line)
        text = "\n".join(out_lines) + "\n"

        # Best-effort: rename the internal footprint name to match the file.
        text = _rename_footprint_internal(text, name)

        with open(dest, "w", encoding="utf-8") as fh:
            fh.write(text)
        result.footprint_imported = True

    def _import_model(self, src_path: str, library: str, name: str,
                      result: ImportResult) -> None:
        dest_dir = self._model_lib_dir(library)
        os.makedirs(dest_dir, exist_ok=True)
        ext = os.path.splitext(src_path)[1]
        dest = os.path.join(dest_dir, name + ext)
        shutil.copyfile(src_path, dest)
        result.model_imported = True

    def _register_libraries(self, library, files, result: ImportResult) -> None:
        if result.symbol_imported:
            result.symbol_lib_registered = lib_table.register_symbol_lib(
                self.project_dir, library,
                "%s/%s.kicad_sym" % (project.SYMBOLS_DIR, library),
            )
        if result.footprint_imported:
            result.footprint_lib_registered = \
                lib_table.register_footprint_lib(
                    self.project_dir, library,
                    "%s/%s.pretty" % (project.FOOTPRINTS_DIR, library),
                )

    # -- rename ----------------------------------------------------------------
    def rename_component(self, library: str, old_name: str,
                         new_name: str) -> None:
        new_name = project.sanitize_name(new_name)
        if old_name == new_name:
            return

        lib_path = self._symbol_lib_path(library)
        if not os.path.exists(lib_path):
            raise FileNotFoundError("Library %r not found" % library)
        lib = SymbolLibrary.load(lib_path)
        lib.rename_symbol(old_name, new_name)

        # Keep the footprint link consistent if this component has a footprint.
        fp_old = os.path.join(
            self._footprint_lib_dir(library), old_name + ".kicad_mod"
        )
        if os.path.exists(fp_old):
            lib.set_footprint(new_name, "%s:%s" % (library, new_name))
        lib.save(lib_path)

        # Rename footprint file.
        if os.path.exists(fp_old):
            fp_new = os.path.join(
                self._footprint_lib_dir(library), new_name + ".kicad_mod"
            )
            with open(fp_old, "r", encoding="utf-8", errors="replace") as fh:
                text = fh.read()
            text = _rename_footprint_internal(text, new_name)
            with open(fp_new, "w", encoding="utf-8") as fh:
                fh.write(text)
            os.remove(fp_old)

        # Rename any model files.
        self._rename_models(library, old_name, new_name)

    def _rename_models(self, library, old_name, new_name) -> None:
        model_dir = self._model_lib_dir(library)
        if not os.path.isdir(model_dir):
            return
        for fn in os.listdir(model_dir):
            stem, ext = os.path.splitext(fn)
            if stem == old_name:
                os.rename(
                    os.path.join(model_dir, fn),
                    os.path.join(model_dir, new_name + ext),
                )

    # -- delete ----------------------------------------------------------------
    def delete_component(self, library: str, name: str) -> bool:
        removed_any = False

        lib_path = self._symbol_lib_path(library)
        if os.path.exists(lib_path):
            lib = SymbolLibrary.load(lib_path)
            if lib.delete_symbol(name):
                lib.save(lib_path)
                removed_any = True

        fp = os.path.join(
            self._footprint_lib_dir(library), name + ".kicad_mod"
        )
        if os.path.exists(fp):
            os.remove(fp)
            removed_any = True

        model_dir = self._model_lib_dir(library)
        if os.path.isdir(model_dir):
            for fn in os.listdir(model_dir):
                if os.path.splitext(fn)[0] == name:
                    os.remove(os.path.join(model_dir, fn))
                    removed_any = True

        return removed_any


# --------------------------------------------------------------------------- #
# Helpers
# --------------------------------------------------------------------------- #

def _node_name(symbol_node):
    from .symbol_library import _symbol_name
    return _symbol_name(symbol_node)


def _rename_footprint_internal(text: str, name: str) -> str:
    """Rename the top-level footprint identifier to ``name`` if parseable.

    Handles both the modern ``(footprint "name" ...)`` form and the legacy
    ``(module name ...)`` form.  Falls back to the original text on any error.
    """
    try:
        root = sexpr.parse(text)
    except Exception:
        return text
    if root.head not in ("footprint", "module"):
        return text
    for idx, child in enumerate(root.children[1:], start=1):
        if isinstance(child, sexpr.Atom):
            root.children[idx] = sexpr.Atom(name, quoted=True)
            break
    try:
        return sexpr.dumps(root)
    except Exception:
        return text
