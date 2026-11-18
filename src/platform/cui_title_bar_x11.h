/* X11: the title bar drawn by the app (cui_app_desc.custom_title_bar). The window asks the
 * window manager to drop its decorations with the Motif hints. Once it is mapped without them,
 * the window manager moves and resizes it through _NET_WM_MOVERESIZE: from the caption of
 * cui_title_bar, and from a band along the window's edges that stands in for the border. */
#ifndef CUI_TITLE_BAR_X11_H
#define CUI_TITLE_BAR_X11_H

#include <X11/Xlib.h>

typedef struct {
    Display *dpy;              /* NULL: the window keeps the window manager's title bar */
    Window win, root;
    Atom move_resize, frame_extents, net_wm_state;
    bool resizable;
    bool handed_over;          /* the last press went to the window manager: drop its release */
    Time last_press;           /* the last press on the caption, for a double click */
    int last_x, last_y;
    Cursor cursors[8];         /* the sizing cursors along the edges, made when first shown */
    Cursor shown;              /* the sizing cursor shown now, None over the content */
} cui__x11_title_bar;

/* Asks to drop the decorations when the window manager can move and resize the window for the
 * app. Returns false, and leaves bar unused, when it cannot. */
bool cui__x11_title_bar_request(cui__x11_title_bar *bar, Display *dpy, Window win, int screen, bool resizable);
void cui__x11_title_bar_release(cui__x11_title_bar *bar);
/* MapNotify and PropertyNotify: whether the decorations are gone, and whether it is maximized. */
void cui__x11_title_bar_event(cui__x11_title_bar *bar, cui_app *app, const XEvent *ev);
/* A press or release of a button. Returns true when the title bar took it from the app. */
bool cui__x11_title_bar_button(cui__x11_title_bar *bar, cui_app *app, const XButtonEvent *ev, float scale, int w, int h);
/* Shows a sizing cursor along the edges, and the app's cursor elsewhere. */
void cui__x11_title_bar_cursor(cui__x11_title_bar *bar, cui_app *app, const XMotionEvent *ev, float scale, int w, int h, Cursor app_cursor);

#endif
