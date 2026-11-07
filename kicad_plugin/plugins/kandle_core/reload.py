"""Best-effort library reload for KiCad 9+ via the IPC API (``kipy``).

KiCad 9 introduced a cross-editor IPC API exposed through the ``kipy`` package
(``kicad-python``).  After we edit ``sym-lib-table`` / ``fp-lib-table`` and the
library files on disk, we ask KiCad to refresh so the new components show up
without a manual restart.

The IPC surface for reloading library tables is still evolving, so this module
degrades gracefully: it attempts the richest refresh available and otherwise
returns a message telling the user what (if anything) they need to do.  It is
deliberately isolated from the rest of the backend and is not unit tested,
because it requires a running KiCad instance.
"""

from __future__ import annotations

from dataclasses import dataclass


@dataclass
class ReloadResult:
    ok: bool
    message: str


def reload_libraries() -> ReloadResult:
    """Attempt to refresh KiCad's libraries over IPC.

    Returns a :class:`ReloadResult` describing the outcome.  Never raises.
    """
    try:
        from kipy import KiCad  # type: ignore
    except Exception:
        return ReloadResult(
            ok=False,
            message=(
                "Auto-refresh needs the KiCad API, which is disabled.\n"
                "Requires KiCad 9 or newer.\n\n"
                "To enable it:\n"
                "  1. Open Preferences > Plugins.\n"
                "  2. Tick 'Enable the KiCad API / IPC server'.\n"
                "  3. Click OK and restart KiCad.\n\n"
                "Until then, reload the project (or restart KiCad) to see "
                "the new components."
            ),
        )

    try:
        kicad = KiCad()
        # The IPC API auto-connects to the running KiCad instance.  A ping
        # confirms connectivity; richer 'reload library table' calls are used
        # when the running KiCad version exposes them.
        _ = kicad.get_version()

        refreshed = False
        # Newer kipy builds may expose an explicit refresh/reload.  Probe for
        # it without hard-failing on older ones.
        for attr in ("refresh", "reload_libraries", "refresh_libraries"):
            fn = getattr(kicad, attr, None)
            if callable(fn):
                fn()
                refreshed = True
                break

        if refreshed:
            return ReloadResult(ok=True, message="Libraries refreshed.")
        return ReloadResult(
            ok=True,
            message=(
                "Connected to KiCad. Library tables were updated on disk; "
                "new components appear on the next project reload."
            ),
        )
    except Exception as exc:  # pragma: no cover - needs running KiCad
        return ReloadResult(
            ok=False,
            message=(
                "Could not reach the KiCad API server (KiCad 9+ required).\n\n"
                "Check it is enabled:\n"
                "  1. Open Preferences > Plugins.\n"
                "  2. Tick 'Enable the KiCad API / IPC server'.\n"
                "  3. Click OK and restart KiCad.\n\n"
                "Until then, reload the project (or restart KiCad) to see "
                "the new components.\n\n"
                "Details: %s" % exc
            ),
        )
