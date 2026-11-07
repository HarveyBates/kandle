"""The Kandle Component Manager window.

A modeless ``wx.Frame`` launched from the KiCad (PCBnew) toolbar.  It supports:

* drag-and-drop of a vendor ``.zip`` onto the window (with a visual drop zone
  that highlights while a file is dragged over it),
* a pre-import preview of which assets (symbol / footprint / 3D model) the
  archive contains,
* selecting an existing component type (library) or creating a new one, with a
  live preview of the sanitized component name and a duplicate-name warning,
* a searchable table of every imported component, showing which assets each has,
* renaming and deleting components (from buttons or a right-click menu) and
  revealing the components folder on disk.

All file operations are delegated to :class:`kandle_core.ComponentManager`.
After a successful mutation the window asks KiCad to refresh its libraries over
the IPC API (KiCad 9+).
"""

from __future__ import annotations

import os
import subprocess
import sys
import traceback

import wx

from kandle_core import ComponentManager, project
from kandle_core import reload as kandle_reload

_DROP_PROMPT = ("Drag a vendor .zip here (SnapEDA, Ultra Librarian, CSE)\n"
                "or click 'Browse...'")

_CHECK = "\u2713"   # ✓
_CROSS = "\u2717"   # ✗


def _reveal_in_file_manager(path: str) -> None:
    """Open ``path`` in the platform's file manager (best-effort)."""
    try:
        if sys.platform.startswith("darwin"):
            subprocess.Popen(["open", path])
        elif os.name == "nt":
            os.startfile(path)  # type: ignore[attr-defined]
        else:
            subprocess.Popen(["xdg-open", path])
    except Exception:
        pass


class _ZipDropTarget(wx.FileDropTarget):
    def __init__(self, frame):
        super().__init__()
        self.frame = frame

    def OnEnter(self, x, y, result):
        self.frame.set_drop_hover(True)
        return result

    def OnLeave(self):
        self.frame.set_drop_hover(False)

    def OnDropFiles(self, x, y, filenames):
        self.frame.set_drop_hover(False)
        zips = [f for f in filenames if f.lower().endswith(".zip")]
        if not zips:
            self.frame.log("Only .zip files can be dropped here.")
            return False
        for path in zips:
            self.frame.stage_zip(path)
        return True


class _DropZonePanel(wx.Panel):
    """A panel that paints a dashed-border drop zone with centred text."""

    def __init__(self, parent):
        super().__init__(parent, style=wx.FULL_REPAINT_ON_RESIZE)
        self._hover = False
        self._text = _DROP_PROMPT
        self.SetMinSize((-1, 90))
        self.SetBackgroundStyle(wx.BG_STYLE_PAINT)
        self.Bind(wx.EVT_PAINT, self._on_paint)

    def set_hover(self, hover: bool) -> None:
        if hover != self._hover:
            self._hover = hover
            self.Refresh()

    def set_text(self, text: str) -> None:
        self._text = text
        self.Refresh()

    def _on_paint(self, _evt) -> None:
        dc = wx.AutoBufferedPaintDC(self)
        gc = wx.GraphicsContext.Create(dc)
        if gc is None:
            return
        w, h = self.GetClientSize()

        parent_bg = self.GetParent().GetBackgroundColour()
        gc.SetBrush(wx.Brush(parent_bg))
        gc.SetPen(wx.TRANSPARENT_PEN)
        gc.DrawRectangle(0, 0, w, h)

        if self._hover:
            fill = wx.Colour(222, 238, 255)
            border = wx.Colour(64, 132, 220)
        else:
            fill = wx.Colour(246, 246, 246)
            border = wx.Colour(170, 170, 170)

        pen = wx.Pen(border, 2, wx.PENSTYLE_SHORT_DASH)
        gc.SetBrush(wx.Brush(fill))
        gc.SetPen(pen)
        gc.DrawRoundedRectangle(3, 3, max(w - 6, 1), max(h - 6, 1), 8)

        gc.SetFont(self.GetFont(), wx.Colour(70, 70, 70))
        lines = self._text.split("\n")
        line_heights = []
        total_h = 0.0
        for line in lines:
            _tw, th = gc.GetTextExtent(line)[:2]
            line_heights.append(th)
            total_h += th
        y = (h - total_h) / 2
        for line, th in zip(lines, line_heights):
            tw = gc.GetTextExtent(line)[0]
            gc.DrawText(line, (w - tw) / 2, y)
            y += th


class ManagerFrame(wx.Frame):
    def __init__(self, parent, project_dir: str):
        super().__init__(parent, title="Kandle - Component Manager",
                         size=(760, 620))
        self.SetMinSize((620, 520))
        self.manager = ComponentManager(project_dir)
        self.project_dir = project_dir
        self._staged_zip = None
        self._config = wx.Config("Kandle")
        # Cache of the selected library's component names for fast duplicate
        # checks while the user types a name.
        self._lib_component_cache = set()

        panel = wx.Panel(self)
        root = wx.BoxSizer(wx.VERTICAL)

        # The top "Import" section is a fixed size (proportion 0); only the
        # bottom "Existing components" section grows when the window is resized.
        import_panel = self._build_import_panel(panel)
        manager_panel = self._build_manager_panel(panel)

        root.Add(import_panel, 0, wx.EXPAND | wx.ALL, 8)
        root.Add(manager_panel, 1, wx.EXPAND | wx.LEFT | wx.RIGHT, 8)

        # -- Log ---------------------------------------------------------------
        # The log lives inside a collapsible pane that is hidden by default and
        # expands automatically when an error is logged (or on user request).
        self.log_pane = wx.CollapsiblePane(
            panel, label="Log", style=wx.CP_DEFAULT_STYLE | wx.CP_NO_TLW_RESIZE)
        log_win = self.log_pane.GetPane()
        self.log_ctrl = wx.TextCtrl(
            log_win, style=wx.TE_MULTILINE | wx.TE_READONLY, size=(-1, 90))
        log_sizer = wx.BoxSizer(wx.VERTICAL)
        log_sizer.Add(self.log_ctrl, 1, wx.EXPAND)
        log_win.SetSizer(log_sizer)
        self.log_pane.Collapse(True)
        self.log_pane.Bind(
            wx.EVT_COLLAPSIBLEPANE_CHANGED, self._on_log_pane_changed)
        root.Add(self.log_pane, 0, wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, 8)

        panel.SetSizer(root)

        self.SetDropTarget(_ZipDropTarget(self))

        self._restore_geometry()
        self.Bind(wx.EVT_CLOSE, self.on_close)

        self.refresh_libraries()
        self._restore_last_library()
        self._update_name_preview()
        self._update_preview(None)
        self._update_manager_buttons()
        self.log("Project: %s" % project_dir)

    # -- panel construction ----------------------------------------------------
    def _build_import_panel(self, parent) -> wx.Panel:
        panel = wx.Panel(parent)
        drop_box = wx.StaticBox(panel, label="Import")
        drop_sizer = wx.StaticBoxSizer(drop_box, wx.VERTICAL)

        self.drop_zone = _DropZonePanel(drop_box)
        drop_sizer.Add(self.drop_zone, 0, wx.EXPAND | wx.ALL, 8)

        file_row = wx.BoxSizer(wx.HORIZONTAL)
        browse_btn = wx.Button(drop_box, label="Browse...")
        browse_btn.Bind(wx.EVT_BUTTON, self.on_browse)
        self.clear_btn = wx.Button(drop_box, label="Clear")
        self.clear_btn.Bind(wx.EVT_BUTTON, self.on_clear_staged)
        self.clear_btn.Disable()
        file_row.Add(browse_btn, 0, wx.RIGHT, 6)
        file_row.Add(self.clear_btn, 0)
        drop_sizer.Add(file_row, 0, wx.LEFT | wx.BOTTOM, 8)

        # Asset preview for the staged archive.
        self.preview_label = wx.StaticText(drop_box, label="")
        drop_sizer.Add(self.preview_label, 0, wx.LEFT, 8)

        # -- Import form -------------------------------------------------------
        form = wx.FlexGridSizer(3, 2, 6, 8)
        form.AddGrowableCol(1, 1)

        form.Add(wx.StaticText(drop_box, label="Component type:"),
                 0, wx.ALIGN_CENTER_VERTICAL)
        self.library_choice = wx.ComboBox(drop_box, style=wx.CB_DROPDOWN)
        self.library_choice.SetHint("Pick existing or type a new type...")
        self.library_choice.Bind(wx.EVT_COMBOBOX, self.on_library_changed)
        self.library_choice.Bind(wx.EVT_TEXT, self.on_library_changed)
        form.Add(self.library_choice, 1, wx.EXPAND)

        form.Add(wx.StaticText(drop_box, label="Component name:"),
                 0, wx.ALIGN_CENTER_VERTICAL)
        self.name_field = wx.TextCtrl(drop_box)
        self.name_field.SetHint("Name this component...")
        self.name_field.Bind(wx.EVT_TEXT, self.on_name_changed)
        form.Add(self.name_field, 1, wx.EXPAND)

        # Spacer cell + live name/duplicate feedback.
        form.Add((0, 0))
        self.name_preview = wx.StaticText(drop_box, label="")
        form.Add(self.name_preview, 1, wx.EXPAND)

        drop_sizer.Add(form, 0, wx.EXPAND | wx.LEFT | wx.RIGHT | wx.TOP, 8)

        self.import_btn = wx.Button(drop_box, label="Import component")
        self.import_btn.Bind(wx.EVT_BUTTON, self.on_import)
        self.import_btn.SetDefault()
        self.import_btn.Disable()
        drop_sizer.Add(self.import_btn, 0, wx.ALIGN_RIGHT | wx.ALL, 8)

        outer = wx.BoxSizer(wx.VERTICAL)
        outer.Add(drop_sizer, 1, wx.EXPAND)
        panel.SetSizer(outer)
        return panel

    def _build_manager_panel(self, parent) -> wx.Panel:
        panel = wx.Panel(parent)
        mgr_box = wx.StaticBox(panel, label="Existing components")
        mgr_sizer = wx.StaticBoxSizer(mgr_box, wx.VERTICAL)

        search_row = wx.BoxSizer(wx.HORIZONTAL)
        search_row.Add(wx.StaticText(mgr_box, label="Search:"),
                       0, wx.ALIGN_CENTER_VERTICAL | wx.RIGHT, 6)
        self.search_field = wx.TextCtrl(mgr_box)
        self.search_field.SetHint("Filter by type or name...")
        self.search_field.Bind(wx.EVT_TEXT, self.on_search)
        search_row.Add(self.search_field, 1)
        mgr_sizer.Add(search_row, 0, wx.EXPAND | wx.ALL, 8)

        self.components_list = wx.ListCtrl(
            mgr_box, style=wx.LC_REPORT | wx.LC_SINGLE_SEL)
        self.components_list.InsertColumn(0, "Type", width=160)
        self.components_list.InsertColumn(1, "Name", width=220)
        self.components_list.InsertColumn(2, "Symbol",
                                          format=wx.LIST_FORMAT_CENTER, width=70)
        self.components_list.InsertColumn(3, "Footprint",
                                          format=wx.LIST_FORMAT_CENTER, width=80)
        self.components_list.InsertColumn(4, "3D model",
                                          format=wx.LIST_FORMAT_CENTER, width=80)
        self.components_list.Bind(wx.EVT_LIST_ITEM_SELECTED,
                                  self.on_component_selection)
        self.components_list.Bind(wx.EVT_LIST_ITEM_DESELECTED,
                                  self.on_component_selection)
        self.components_list.Bind(wx.EVT_LIST_ITEM_RIGHT_CLICK,
                                  self.on_component_context_menu)
        mgr_sizer.Add(self.components_list, 1, wx.EXPAND | wx.LEFT | wx.RIGHT, 8)

        self.empty_label = wx.StaticText(
            mgr_box,
            label="No components imported yet.\n"
                  "Drag a vendor .zip into the Import area above to get "
                  "started.",
            style=wx.ALIGN_CENTRE_HORIZONTAL)
        mgr_sizer.Add(self.empty_label, 1,
                      wx.EXPAND | wx.LEFT | wx.RIGHT | wx.TOP, 8)
        self.empty_label.Hide()

        btn_row = wx.BoxSizer(wx.HORIZONTAL)
        self.rename_btn = wx.Button(mgr_box, label="Rename")
        self.rename_btn.Bind(wx.EVT_BUTTON, self.on_rename)
        self.delete_btn = wx.Button(mgr_box, label="Delete")
        self.delete_btn.Bind(wx.EVT_BUTTON, self.on_delete)
        open_btn = wx.Button(mgr_box, label="Open components folder")
        open_btn.Bind(wx.EVT_BUTTON, self.on_open_folder)
        btn_row.Add(self.rename_btn, 0, wx.RIGHT, 6)
        btn_row.Add(self.delete_btn, 0, wx.RIGHT, 6)
        btn_row.AddStretchSpacer()
        btn_row.Add(open_btn, 0)
        mgr_sizer.Add(btn_row, 0, wx.EXPAND | wx.ALL, 8)

        outer = wx.BoxSizer(wx.VERTICAL)
        outer.Add(mgr_sizer, 1, wx.EXPAND)
        panel.SetSizer(outer)
        return panel

    # -- geometry / config -----------------------------------------------------
    def _restore_geometry(self) -> None:
        w = self._config.ReadInt("win_w", -1)
        h = self._config.ReadInt("win_h", -1)
        if w > 100 and h > 100:
            self.SetSize((w, h))
        x = self._config.ReadInt("win_x", -1)
        y = self._config.ReadInt("win_y", -1)
        if x >= 0 and y >= 0:
            self.SetPosition((x, y))

    def _restore_last_library(self) -> None:
        last = self._config.Read("last_library", "")
        if last and not self.library_choice.GetValue():
            self.library_choice.SetValue(last)
            self._update_lib_component_cache()

    def on_close(self, evt) -> None:
        size = self.GetSize()
        pos = self.GetPosition()
        self._config.WriteInt("win_w", size.width)
        self._config.WriteInt("win_h", size.height)
        self._config.WriteInt("win_x", max(pos.x, 0))
        self._config.WriteInt("win_y", max(pos.y, 0))
        self._config.Write("last_library", self.library_choice.GetValue())
        self._config.Flush()
        evt.Skip()

    # -- helpers ---------------------------------------------------------------
    def log(self, message: str, error: bool = False) -> None:
        self.log_ctrl.AppendText(message + "\n")
        if error:
            self.expand_log()

    def expand_log(self) -> None:
        """Reveal the log pane (used when an error needs attention)."""
        if self.log_pane.IsCollapsed():
            self.log_pane.Expand()
            self._on_log_pane_changed(None)

    def _on_log_pane_changed(self, evt) -> None:
        # Re-run layout so the frame grows/shrinks with the pane state.
        self.log_pane.GetParent().Layout()
        self.Layout()
        if evt is not None:
            evt.Skip()

    def set_drop_hover(self, hover: bool) -> None:
        self.drop_zone.set_hover(hover)

    def refresh_libraries(self) -> None:
        libs = self.manager.libraries()
        current = self.library_choice.GetValue()
        self.library_choice.Set(libs)
        if current:
            self.library_choice.SetValue(current)
        elif libs:
            self.library_choice.SetValue(libs[0])
        self._update_lib_component_cache()
        self.refresh_components()

    def selected_library(self):
        value = self.library_choice.GetValue().strip()
        return value or None

    def _update_lib_component_cache(self) -> None:
        lib = self.selected_library()
        if lib and lib in self.manager.libraries():
            self._lib_component_cache = set(self.manager.components(lib))
        else:
            self._lib_component_cache = set()

    # -- component table -------------------------------------------------------
    def refresh_components(self) -> None:
        """Repopulate the table with every component, honouring the filter."""
        self.components_list.DeleteAllItems()
        records = self.manager.all_components()
        needle = self.search_field.GetValue().strip().lower()
        if needle:
            records = [
                r for r in records
                if needle in r.name.lower() or needle in r.library.lower()
            ]

        if not records:
            self.components_list.Hide()
            self.empty_label.Show()
            self.empty_label.GetParent().Layout()
            self._update_manager_buttons()
            return

        self.empty_label.Hide()
        self.components_list.Show()
        self.components_list.GetParent().Layout()

        for row, rec in enumerate(records):
            self.components_list.InsertItem(row, rec.library)
            self.components_list.SetItem(row, 1, rec.name)
            self.components_list.SetItem(
                row, 2, _CHECK if rec.has_symbol else _CROSS)
            self.components_list.SetItem(
                row, 3, _CHECK if rec.has_footprint else _CROSS)
            self.components_list.SetItem(
                row, 4, _CHECK if rec.has_model else _CROSS)

        self._scroll_to_selected_type()
        self._update_manager_buttons()

    def _scroll_to_selected_type(self) -> None:
        """Scroll to (and select) the first row of the chosen component type."""
        lib = self.selected_library()
        if not lib:
            return
        count = self.components_list.GetItemCount()
        for row in range(count):
            if self.components_list.GetItemText(row, 0) == lib:
                self.components_list.Select(row)
                self.components_list.EnsureVisible(row)
                return

    def _selected_row(self):
        return self.components_list.GetFirstSelected()

    def _selected_component(self):
        """Return ``(library, name)`` for the selected row, or ``None``."""
        row = self._selected_row()
        if row == -1:
            return None
        lib = self.components_list.GetItemText(row, 0)
        name = self.components_list.GetItemText(row, 1)
        return lib, name

    def _update_manager_buttons(self) -> None:
        has_sel = self._selected_row() != -1
        self.rename_btn.Enable(has_sel)
        self.delete_btn.Enable(has_sel)

    def on_search(self, _evt) -> None:
        self.refresh_components()

    def on_component_selection(self, evt) -> None:
        self._update_manager_buttons()
        evt.Skip()

    def on_component_context_menu(self, evt) -> None:
        if self._selected_component() is None:
            self.components_list.Select(evt.GetIndex())
        menu = wx.Menu()
        rename_item = menu.Append(wx.ID_ANY, "Rename...")
        delete_item = menu.Append(wx.ID_ANY, "Delete")
        menu.AppendSeparator()
        reveal_item = menu.Append(wx.ID_ANY, "Reveal in file manager")
        self.Bind(wx.EVT_MENU, self.on_rename, rename_item)
        self.Bind(wx.EVT_MENU, self.on_delete, delete_item)
        self.Bind(wx.EVT_MENU, self.on_open_folder, reveal_item)
        self.components_list.PopupMenu(menu)
        menu.Destroy()

    # -- name / preview feedback -----------------------------------------------
    def on_name_changed(self, _evt) -> None:
        self._update_name_preview()

    def _update_name_preview(self) -> None:
        raw = self.name_field.GetValue().strip()
        if not raw:
            self.name_preview.SetLabel("")
            return
        sanitized = project.sanitize_name(raw)
        if sanitized in self._lib_component_cache:
            self.name_preview.SetForegroundColour(wx.Colour(185, 70, 30))
            self.name_preview.SetLabel(
                "Saves as: %s  \u2014  already exists (will be skipped)"
                % sanitized)
        else:
            self.name_preview.SetForegroundColour(wx.Colour(90, 90, 90))
            self.name_preview.SetLabel("Saves as: %s" % sanitized)
        self.name_preview.GetParent().Layout()

    def _update_preview(self, files) -> None:
        if files is None:
            self.preview_label.SetLabel("")
            return

        def mark(present):
            return _CHECK if present else _CROSS

        has_symbol = files.symbol is not None or files.legacy_symbol is not None
        self.preview_label.SetLabel(
            "Archive contents:   Symbol %s    Footprint %s    3D model %s" % (
                mark(has_symbol), mark(files.footprint is not None),
                mark(files.model is not None)))
        self.preview_label.GetParent().Layout()

    # -- import flow -----------------------------------------------------------
    def stage_zip(self, path: str) -> None:
        self._staged_zip = path
        self.drop_zone.set_text("Ready to import:\n%s" % os.path.basename(path))
        if not self.name_field.GetValue():
            self.name_field.SetValue(project.sanitize_name(path))
        self.clear_btn.Enable()
        self.import_btn.Enable()
        try:
            files = self.manager.peek_zip(path)
        except Exception:  # noqa: BLE001
            files = None
        self._update_preview(files)
        self._update_name_preview()
        self.log("Staged: %s" % path)

    def on_clear_staged(self, _evt) -> None:
        self._staged_zip = None
        self.drop_zone.set_text(_DROP_PROMPT)
        self.clear_btn.Disable()
        self.import_btn.Disable()
        self.name_field.SetValue("")
        self._update_preview(None)
        self._update_name_preview()

    def on_browse(self, _evt) -> None:
        with wx.FileDialog(
            self, "Select a vendor .zip",
            wildcard="Zip archives (*.zip)|*.zip",
            style=wx.FD_OPEN | wx.FD_FILE_MUST_EXIST,
        ) as dlg:
            if dlg.ShowModal() == wx.ID_OK:
                self.stage_zip(dlg.GetPath())

    def on_library_changed(self, _evt) -> None:
        self._update_lib_component_cache()
        self._update_name_preview()
        self._scroll_to_selected_type()

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
            self.log("Import failed: %s" % exc, error=True)
            self.log(traceback.format_exc())
            return

        self.log("Imported %r into %r (symbol=%s footprint=%s model=%s)" % (
            result.component_name, result.library,
            result.symbol_imported, result.footprint_imported,
            result.model_imported))
        for w in result.warnings:
            self.log("  warning: %s" % w)

        self._staged_zip = None
        self.clear_btn.Disable()
        self.import_btn.Disable()
        self.name_field.SetValue("")
        self.drop_zone.set_text(_DROP_PROMPT)
        self._update_preview(None)
        self.refresh_libraries()
        self._reload_kicad()

    # -- manager actions -------------------------------------------------------
    def on_rename(self, _evt) -> None:
        selected = self._selected_component()
        if selected is None:
            return
        lib, comp = selected
        dlg = wx.TextEntryDialog(self, "New name for %r:" % comp,
                                 "Rename component", comp)
        if dlg.ShowModal() == wx.ID_OK:
            new_name = dlg.GetValue().strip()
            if new_name:
                try:
                    self.manager.rename_component(lib, comp, new_name)
                    self.log("Renamed %r -> %r" % (comp, new_name))
                    self._update_lib_component_cache()
                    self.refresh_components()
                    self._reload_kicad()
                except Exception as exc:  # noqa: BLE001
                    self.log("Rename failed: %s" % exc, error=True)
        dlg.Destroy()

    def on_delete(self, _evt) -> None:
        selected = self._selected_component()
        if selected is None:
            return
        lib, comp = selected
        if wx.MessageBox(
            "Delete component %r from %r?\nThis removes its symbol, footprint "
            "and 3D model." % (comp, lib),
            "Confirm delete", wx.YES_NO | wx.ICON_WARNING,
        ) != wx.YES:
            return
        try:
            if self.manager.delete_component(lib, comp):
                self.log("Deleted %r" % comp)
            self._update_lib_component_cache()
            self.refresh_components()
            self._reload_kicad()
        except Exception as exc:  # noqa: BLE001
            self.log("Delete failed: %s" % exc, error=True)

    def on_open_folder(self, _evt) -> None:
        folder = self.manager.components_dir()
        if not os.path.isdir(folder):
            self.log("No components folder yet: %s" % folder)
            return
        _reveal_in_file_manager(folder)

    def _reload_kicad(self) -> None:
        result = kandle_reload.reload_libraries()
        self.log(result.message)
