# Fork notes: `codex/cui-titlebar-testing`

Open problems and untested parts of this branch. It is not proposed upstream.
Before a proposal, split it into the title bar and the two general fixes below.

## What the branch holds

- **Title bar**: `cui_title_bar` and `cui_app_desc.custom_title_bar` on Win32
  and X11, including XWayland. A sidebar can reach up into the title bar
  with a segment of its own (`.no_window_buttons`). Platform layers use
  `cui_platform_iface.window_frame` and `.window_button` together with
  `cui_title_bar_hit_test`.
- **X11 OpenGL idle** (`6ddb1ef`): an idle window presented continuously,
  because Mesa's Present events woke `cui__plat_wait` without an event for the
  app. Idle now costs 0 CPU.
- **Transitions after a rest** (`b019f9e`, `762fbb9`): a hover fade or spring
  that started on the first frame after an idle wait finished in that one frame.
  `ui->step` now caps that first step; `dt` and the clock keep the real time.

## Verified

- Windows 11, OpenGL and software: window buttons, Snap layouts, dragging,
  double click, edge resizing, dark mode, the inactive look, `--system-frame`,
  `--fixed-size`, the hover fade and idle CPU. All CTest tests pass. The split
  title bar (sidebar segment): dragging and double click in both segments,
  and hiding and showing the sidebar.
- Debian, KDE KWin 6.7 X11, both renderers: the same interactions, a
  120-launch startup check, the edge hairline and idle CPU. CTest 26/26.
- KDE KWin 6.7 Wayland through XWayland, both renderers: 40/40 launches with
  window buttons and the hairline, and `--system-frame` without flashes.
  CTest 26/26.

## Open problems

- **Split title bar on Linux untested.** The sidebar segment works through the
  same hit test on every backend, but it has only been tried on Windows.
- **Wayland input untested.** Under KWin Wayland, dragging, double click, the
  window buttons, edge resizing and hover have not been tried, because
  synthetic `xdotool` input does not reach KWin. Test them by hand.
- **`3e08e5f` untested.** It re-checks the frame on every `ReparentNotify`,
  for window managers that map a window before reparenting it. The change is
  small, but it has been neither compiled nor run.
- **SDL2 ignores fractional scaling on native Wayland.** At a display scale of
  1.3 the window comes out at scale 1, so the interface is about 23% too
  small, though sharp. GLFW scales correctly. The SDL2 backend probably needs
  SDL's Wayland scaling hint or drawable-size handling. This is unrelated to
  the title bar.
- **No shadow on X11 and XWayland.** Without decorations, KWin draws neither a
  border nor a shadow. The 1 px hairline marks the window's edge, but the
  window looks flatter than decorated ones.
- **DPI changes on Win32.** With the custom frame, `WM_DPICHANGED` still uses
  the size Windows suggests, which scales the outer rectangle. The content
  can drift by a few pixels after a move to a monitor with a different DPI.
  Untested.
- **Untested window managers and systems:** GNOME/Mutter (X11 and XWayland),
  Xfwm, Openbox (the old prototype only), tiling window managers, and macOS.
  macOS keeps the system title bar.
- **KWin restore-on-drag.** Dragging a maximized window restores it, but KWin
  places the pointer higher in the bar than where it was grabbed. That is
  KWin's behavior for frameless windows; it is cosmetic.
- **Close glyph.** Its antialiased diagonals look slightly heavier than the
  1 px minimize and maximize lines, as on Windows itself.
