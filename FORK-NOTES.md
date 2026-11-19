# Fork notes: `codex/cui-titlebar-testing`

Open problems and untested parts of this branch. It is not proposed upstream.
Before a proposal, split it into the title bar and the two general fixes below.

## What the branch holds

- **Title bar**: `cui_title_bar` and `cui_app_desc.custom_title_bar` on Win32
  and X11, including XWayland. A sidebar can reach up into the title bar
  with a segment of its own (`.no_window_buttons`), and
  `cui_app_set_custom_title_bar` switches it at run time. The gallery draws its
  own title bar, and its Window page switches it and shows and changes the
  window's state. `cui_split` takes `.fixed_first` for a sidebar that keeps
  its width; the gallery's page list uses it while its own title bar is in
  effect. Platform layers use
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
  and hiding and showing the sidebar. The gallery's title bar and Window page:
  switching the title bar off and on keeps the content in place, and the
  Maximize and Restore buttons work. The page list's divider drags its width,
  keeps it when the window grows and stops at its 180 pt minimum.
- Debian, KDE KWin 6.7 X11, both renderers: the same interactions, a
  120-launch startup check, the edge hairline and idle CPU. CTest 26/26.
  The gallery's split title bar, divider and Window page buttons, and the
  run-time switch: the window is withdrawn and mapped again so that KWin reads
  the changed Motif hint, keeps its place and its maximized state, and blinks
  for under 50 ms.
- KDE KWin 6.7 Wayland through XWayland, both renderers: 40/40 launches with
  window buttons and the hairline, and `--system-frame` without flashes.
  CTest 26/26. Input at display scale 1.3 (OpenGL): double click in both
  segments maximizes and restores, Minimize, Maximize, Restore and Close work,
  Close turns red on hover, and the edges show the sizing cursors. By hand in
  the gallery: moving the window from the bar's empty space (not over its
  widgets), with the bar on and off; resizing at every edge and corner;
  restore-on-drag; the run-time switch, also while maximized; the divider with
  its 180 pt minimum; the panel button; and the Window page's controls and
  readouts.

## Open problems

- **Emulated input cannot move windows under KWin Wayland.** KWin ignores
  `_NET_WM_MOVERESIZE` from emulated input, also for its own title bar, so
  moving and resizing there can only be tested by hand. One automated run lost
  a Close click after minimizing and activating the window through client
  messages; by hand it did not happen.
- **`3e08e5f` not exercised.** It re-checks the frame on every
  `ReparentNotify`, for window managers that map a window before reparenting
  it. Every later KWin build ran it, but no window manager that reparents
  late has been tried.
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
