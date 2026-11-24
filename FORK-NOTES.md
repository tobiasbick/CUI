# Fork notes for `codex/cui-titlebar-testing`

The branch adds an app-drawn title bar on Win32 and X11, including XWayland.
The gallery's **Window** page switches between app and system title bars at
run time and shows the window state. The sidebar and its divider reach the
top by default. **Start sidebar below the title bar** places a single bar
above both columns. While the app title bar is active, the gallery uses
`cui_split` with `.fixed_first` to keep the chosen sidebar width.
[Title bar API and screenshots](docs/apps.md#your-own-title-bar).

## Open problems

- **Native SDL2 Wayland fractional scaling.** Start the
  `-DCUI_PLATFORM=wayland` gallery at 125% or 150% KWin display scaling with
  `CUI_SCALE` unset. A 240-point sidebar should measure 300 px or 360 px,
  respectively. It measured 240 px at both scales with OpenGL and software.
- **XWayland caption drag at 125% and 150%.** Enable **Start sidebar below the
  title bar** and drag the free app caption directly above the divider. The
  window should move and keep its size. The KDE input portal moved it at 100%.
  At 125% with OpenGL and 150% with software, the portal drag left the window
  in place. The KWin system caption moved the window at 125% and snapped it
  to maximized at 150%. A physical mouse check is still pending.

## Verification on KDE

Commit `6e64086` was tested on Debian with Plasma and KWin 6.7.4 in a Wayland
session. Each backend ran in its own Release build. Both sidebar layouts
were exercised with OpenGL and software at 100%, 125% and 150% KWin display
scaling, with `CUI_SCALE` unset.

| Backend | `CUI_TEXT_SHAPING=OFF` | `CUI_TEXT_SHAPING=ON` |
| --- | ---: | ---: |
| XWayland (`-DCUI_PLATFORM=x11`, including the X11 idle test) | 29/29 | 30/30 |
| Native SDL2 Wayland (`-DCUI_PLATFORM=wayland`) | 28/28 | 29/29 |

- **XWayland.** Both divider positions worked, and the sidebar kept its width
  after hiding, switching layouts and widening the window again. At 100%,
  setting the window to 740 layout points exposed the drawer menu. Caption
  double-click, minimize and restore worked. Portal edge drags resized the
  window with both title bars. Switching either way preserved the normal
  content rectangle with both renderers, including after restoring through
  KWin from a maximized window. While maximized, KWin reserved an extra 28 px
  for its system title bar at 100%. For the app frame, KWin leaves the window
  shadowless and CUI draws a 1 px edge.
- **Native Wayland.** SDL reported the `wayland` video driver. KWin kept the
  system title bar, and the Window page showed **Not available here** when the
  app title bar was requested. Both layouts kept a fixed 240-point sidebar,
  including after hiding and showing it. The navigation drawer opened at
  740 layout points. The Window page reported the global position as
  `unknown`.

The same X11 geometry fix was verified separately in a native KWin 6.7.4
X11 session at 100% with both renderers. The Release CTest suites passed
29/29 with text shaping off and 30/30 with it on.

## Windows coverage

Win32 CTest covers title bar hit testing and window buttons at 100% and 150%,
including the restored content rectangle when the frame is switched while
maximized or minimized. The gallery test drives the Window page controls
and checks divider dragging and sidebar width across layout and visibility
changes. [Win32 frame test](tests/test_title_bar_win32.c) ·
[Gallery Window test](tests/test_gallery_window.c).

## Coverage still needed

- Test an X11 window manager that maps before reparenting. The frame is
  checked again on `ReparentNotify` (`3e08e5f`). Other window managers still
  to cover include GNOME/Mutter, Xfwm, Openbox and tiling window managers.
- Check Win32 content geometry when a custom-framed window moves between
  monitors with different DPI. `WM_DPICHANGED` applies Windows' suggested
  outer rectangle.
