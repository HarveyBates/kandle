"""The Kandle Component Manager window.

A modeless ``wx.Frame`` launched from the KiCad (PCBnew) toolbar.  It supports:

* drag-and-drop of a vendor ``.zip`` onto the window,
* selecting an existing component type (library) or creating a new one,
* editing the component name before import,
* listing existing components and deleting them.

All file operations are delegated to :class:`kandle_core.ComponentManager`.
After a successful mutation the window asks KiCad to refresh its libraries over
the IPC API (KiCad 9+).
"""

from __future__ import annotations

import os
import traceback

import wx

from kandle_core import ComponentManager, project
from kandle_core import reload as kandle_reload

class _ZipDropTarget(wx.FileDropTarget):
    def __init__(self, frame):
        super().__init__()
        self.frame = frame

    def OnDropFiles(self, x, y, filenames):
        zips = [f for f in filenames if f.lower().endswith(".zip")]
        if not zips:
            self.frame.log("Only .zip files can be dropped here.")
            return False
        for path in zips:
            self.frame.stage_zip(path)
        return True


class ManagerFrame(wx.Frame):
    def __init__(self, parent, project_dir: str):
        super().__init__(parent, title="Kandle - Component Manager",
                         size=(720, 560))
        self.manager = ComponentManager(project_dir)
        self.project_dir = project_dir
        self._staged_zip = None

        panel = wx.Panel(self)
        root = wx.BoxSizer(wx.VERTICAL)

        # -- Drop zone ---------------------------------------------------------
        self.drop_zone = wx.StaticText(
            panel,
            label="Drag a vendor .zip here (SnapEDA, Ultra Librarian, CSE)\n"
                  "or click 'Browse...'",
            style=wx.ALIGN_CENTRE_HORIZONTAL | wx.ST_NO_AUTORESIZE,
        )
        self.drop_zone.SetMinSize((-1, 70))
        drop_box = wx.StaticBox(panel, label="Import")
        drop_sizer = wx.StaticBoxSizer(drop_box, wx.VERTICAL)
        drop_sizer.Add(self.drop_zone, 0, wx.EXPAND | wx.ALL, 8)

        browse_btn = wx.Button(panel, label="Browse...")
        browse_btn.Bind(wx.EVT_BUTTON, self.on_browse)
        drop_sizer.Add(browse_btn, 0, wx.ALIGN_LEFT | wx.LEFT | wx.BOTTOM, 8)

        # -- Import form -------------------------------------------------------
        form = wx.FlexGridSizer(2, 2, 6, 8)
        form.AddGrowableCol(1, 1)

        form.Add(wx.StaticText(panel, label="Component type:"),
                 0, wx.ALIGN_CENTER_VERTICAL)
        # Editable combo box: pick an existing type or type a new one.
        self.library_choice = wx.ComboBox(panel, style=wx.CB_DROPDOWN)
        self.library_choice.SetHint("Pick existing or type a new type...")
        self.library_choice.Bind(wx.EVT_COMBOBOX, self.on_library_changed)
        self.library_choice.Bind(wx.EVT_TEXT, self.on_library_changed)
        form.Add(self.library_choice, 1, wx.EXPAND)

        form.Add(wx.StaticText(panel, label="Component name:"),
                 0, wx.ALIGN_CENTER_VERTICAL)
        self.name_field = wx.TextCtrl(panel)
        self.name_field.SetHint("Name this component...")
        form.Add(self.name_field, 1, wx.EXPAND)

        drop_sizer.Add(form, 0, wx.EXPAND | wx.ALL, 8)

        self.import_btn = wx.Button(panel, label="Import component")
        self.import_btn.Bind(wx.EVT_BUTTON, self.on_import)
        self.import_btn.Disable()
        drop_sizer.Add(self.import_btn, 0, wx.ALIGN_RIGHT | wx.ALL, 8)

        root.Add(drop_sizer, 0, wx.EXPAND | wx.ALL, 8)

        # -- Component manager list -------------------------------------------
        mgr_box = wx.StaticBox(panel, label="Existing components")
        mgr_sizer = wx.StaticBoxSizer(mgr_box, wx.VERTICAL)

        self.components_list = wx.ListBox(panel, style=wx.LB_SINGLE)
        mgr_sizer.Add(self.components_list, 1, wx.EXPAND | wx.ALL, 8)

        btn_row = wx.BoxSizer(wx.HORIZONTAL)
        rename_btn = wx.Button(panel, label="Rename")
        rename_btn.Bind(wx.EVT_BUTTON, self.on_rename)
        delete_btn = wx.Button(panel, label="Delete")
        delete_btn.Bind(wx.EVT_BUTTON, self.on_delete)
        btn_row.Add(rename_btn, 0, wx.RIGHT, 6)
        btn_row.Add(delete_btn, 0)
        mgr_sizer.Add(btn_row, 0, wx.ALIGN_LEFT | wx.LEFT | wx.BOTTOM, 8)

        root.Add(mgr_sizer, 1, wx.EXPAND | wx.ALL, 8)

        # -- Log ---------------------------------------------------------------
        self.log_ctrl = wx.TextCtrl(
            panel, style=wx.TE_MULTILINE | wx.TE_READONLY, size=(-1, 90))
        root.Add(self.log_ctrl, 0, wx.EXPAND | wx.ALL, 8)

        panel.SetSizer(root)

        self.SetDropTarget(_ZipDropTarget(self))

        self.refresh_libraries()
        self.log("Project: %s" % project_dir)

    # -- helpers ---------------------------------------------------------------
    def log(self, message: str) -> None:
        self.log_ctrl.AppendText(message + "\n")

    def refresh_libraries(self) -> None:
        libs = self.manager.libraries()
        # Preserve whatever the user has typed/selected.
        current = self.library_choice.GetValue()
        self.library_choice.Set(libs)
        if current:
            self.library_choice.SetValue(current)
        elif libs:
            self.library_choice.SetValue(libs[0])
        self.refresh_components()

    def selected_library(self):
        # The type is whatever is in the (editable) field; a new name is fine
        # because import_zip creates the library on demand.
        value = self.library_choice.GetValue().strip()
        return value or None

    def refresh_components(self) -> None:
        self.components_list.Clear()
        lib = self.selected_library()
        if lib:
            for name in self.manager.components(lib):
                self.components_list.Append(name)

    # -- import flow -----------------------------------------------------------
    def stage_zip(self, path: str) -> None:
        self._staged_zip = path
        self.drop_zone.SetLabel("Ready to import:\n%s" % os.path.basename(path))
        if not self.name_field.GetValue():
            self.name_field.SetValue(project.sanitize_name(path))
        self.import_btn.Enable()
        self.log("Staged: %s" % path)

    def on_browse(self, _evt) -> None:
        with wx.FileDialog(
            self, "Select a vendor .zip",
            wildcard="Zip archives (*.zip)|*.zip",
            style=wx.FD_OPEN | wx.FD_FILE_MUST_EXIST,
        ) as dlg:
            if dlg.ShowModal() == wx.ID_OK:
                self.stage_zip(dlg.GetPath())

    def on_library_changed(self, _evt) -> None:
        # Refresh the component list for whichever (existing) type is shown.
        self.refresh_components()

    def on_import(self, _evt) -> None:
        if not self._staged_zip:
            return
        library = self.selected_library()
        if not library:
            wx.MessageBox(
                "Enter a component type first (pick one from the dropdown "
                "or type a new name).",
                "Kandle", wx.OK | wx.ICON_WARNING)
            return
        library = project.sanitize_name(library)
        name = self.name_field.GetValue().strip() or None
        try:
            result = self.manager.import_zip(
                self._staged_zip, library, component_name=name)
        except Exception as exc:  # noqa: BLE001
            self.log("Import failed: %s" % exc)
            self.log(traceback.format_exc())
            return

        self.log("Imported %r into %r (symbol=%s footprint=%s model=%s)" % (
            result.component_name, result.library,
            result.symbol_imported, result.footprint_imported,
            result.model_imported))
        for w in result.warnings:
            self.log("  warning: %s" % w)

        self._staged_zip = None
        self.import_btn.Disable()
        self.name_field.SetValue("")
        self.drop_zone.SetLabel("Drag a vendor .zip here or click 'Browse...'")
        self.refresh_libraries()
        self._reload_kicad()

    # -- manager actions -------------------------------------------------------
    def _selected_component(self):
        idx = self.components_list.GetSelection()
        if idx == wx.NOT_FOUND:
            return None
        return self.components_list.GetString(idx)

    def on_rename(self, _evt) -> None:
        lib = self.selected_library()
        comp = self._selected_component()
        if not (lib and comp):
            return
        dlg = wx.TextEntryDialog(self, "New name for %r:" % comp,
                                 "Rename component", comp)
        if dlg.ShowModal() == wx.ID_OK:
            new_name = dlg.GetValue().strip()
            if new_name:
                try:
                    self.manager.rename_component(lib, comp, new_name)
                    self.log("Renamed %r -> %r" % (comp, new_name))
                    self.refresh_components()
                    self._reload_kicad()
                except Exception as exc:  # noqa: BLE001
                    self.log("Rename failed: %s" % exc)
        dlg.Destroy()

    def on_delete(self, _evt) -> None:
        lib = self.selected_library()
        comp = self._selected_component()
        if not (lib and comp):
            return
        if wx.MessageBox(
            "Delete component %r from %r?\nThis removes its symbol, footprint "
            "and 3D model." % (comp, lib),
            "Confirm delete", wx.YES_NO | wx.ICON_WARNING,
        ) != wx.YES:
            return
        try:
            if self.manager.delete_component(lib, comp):
                self.log("Deleted %r" % comp)
            self.refresh_components()
            self._reload_kicad()
        except Exception as exc:  # noqa: BLE001
            self.log("Delete failed: %s" % exc)

    def _reload_kicad(self) -> None:
        result = kandle_reload.reload_libraries()
        self.log(result.message)
