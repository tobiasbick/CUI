/* X11: the title bar drawn by the app, on EWMH window managers that honor the Motif hints. */
#include "cui_platform.h"
#include "cui_title_bar_x11.h"
#include <X11/Xatom.h>
#include <X11/cursorfont.h>

/* _NET_WM_MOVERESIZE directions, clockwise from the top-left corner */
enum { CUI__X11_MOVE = 8, CUI__X11_NO_EDGE = -1 };
enum { CUI__X11_EDGE = 5, CUI__X11_DOUBLE_CLICK_MS = 400, CUI__X11_MOTIF_DECORATIONS = 1 << 1 };

/* A 32-bit property; Xlib hands those over as arrays of long, also on LP64. */
static unsigned long *cui__x11_tb_property(Display *dpy, Window win, Atom property, Atom type, unsigned long *count) {
    unsigned char *data = NULL;
    Atom actual;
    int format;
    unsigned long rest;
    if (XGetWindowProperty(dpy, win, property, 0, 1024, False, type, &actual, &format, count, &rest, &data) == Success &&
        actual == type && format == 32 && rest == 0)
        return (unsigned long *)data;
    if (data) XFree(data);
    *count = 0;
    return NULL;
}

static bool cui__x11_tb_has(const unsigned long *atoms, unsigned long count, Atom wanted) {
    for (unsigned long i = 0; i < count; i++) if (atoms[i] == wanted) return true;
    return false;
}

bool cui__x11_title_bar_request(cui__x11_title_bar *bar, Display *dpy, Window win, int screen, bool resizable) {
    memset(bar, 0, sizeof *bar);
    Window root = RootWindow(dpy, screen);
    Atom move_resize = XInternAtom(dpy, "_NET_WM_MOVERESIZE", False);
    Atom frame_extents = XInternAtom(dpy, "_NET_FRAME_EXTENTS", False);
    Atom state = XInternAtom(dpy, "_NET_WM_STATE", False);
    unsigned long count;
    unsigned long *atoms = cui__x11_tb_property(dpy, root, XInternAtom(dpy, "_NET_SUPPORTED", False), XA_ATOM, &count);
    /* moving and sizing, and for a resizable window maximizing, are the window manager's */
    bool usable = cui__x11_tb_has(atoms, count, move_resize) && cui__x11_tb_has(atoms, count, frame_extents) &&
                  (!resizable || (cui__x11_tb_has(atoms, count, state) &&
                                  cui__x11_tb_has(atoms, count, XInternAtom(dpy, "_NET_WM_STATE_MAXIMIZED_VERT", False)) &&
                                  cui__x11_tb_has(atoms, count, XInternAtom(dpy, "_NET_WM_STATE_MAXIMIZED_HORZ", False))));
    if (atoms) XFree(atoms);
    if (!usable) return false;
    bar->dpy = dpy;
    bar->win = win;
    bar->root = root;
    bar->move_resize = move_resize;
    bar->frame_extents = frame_extents;
    bar->net_wm_state = state;
    bar->resizable = resizable;
    Atom motif = XInternAtom(dpy, "_MOTIF_WM_HINTS", False);
    unsigned long hints[5] = { CUI__X11_MOTIF_DECORATIONS, 0, 0, 0, 0 };   /* decorations: none */
    XChangeProperty(dpy, win, motif, motif, 32, PropModeReplace, (unsigned char *)hints, 5);
    return true;
}

void cui__x11_title_bar_release(cui__x11_title_bar *bar) {
    if (!bar->dpy) return;
    for (int i = 0; i < 8; i++) if (bar->cursors[i]) XFreeCursor(bar->dpy, bar->cursors[i]);
}

/* 1 when the window manager took the decorations away, 0 when it kept them, -1 while its frame
 * is not laid out yet: KWin maps the frame at 1 by 1 and sizes it afterwards. A reparenting
 * window manager can keep a shadow around the window, so its frame may be larger, but no
 * taller on top than on the other sides; XWayland reports frame extents even for a frame that
 * is only a shadow. A window manager that does not reparent reports a frame it draws in
 * _NET_FRAME_EXTENTS; KWin under Wayland leaves the property out for a window without one. */
static int cui__x11_tb_undecorated(const cui__x11_title_bar *bar) {
    Window root, parent, *children = NULL;
    unsigned int n;
    if (!XQueryTree(bar->dpy, bar->win, &root, &parent, &children, &n)) return -1;
    if (children) XFree(children);
    if (parent != root) {
        int x, y, px, py;
        unsigned int w, h, pw, ph, border, depth;
        if (!XGetGeometry(bar->dpy, bar->win, &root, &x, &y, &w, &h, &border, &depth) ||
            !XGetGeometry(bar->dpy, parent, &root, &px, &py, &pw, &ph, &border, &depth) || pw < w || ph < h)
            return -1;
        return y <= x && y <= (int)ph - (int)h - y && y <= (int)pw - (int)w - x;
    }
    unsigned long count;
    unsigned long *extents = cui__x11_tb_property(bar->dpy, bar->win, bar->frame_extents, XA_CARDINAL, &count);
    int undecorated = count != 4 || (!extents[0] && !extents[1] && !extents[2] && !extents[3]);
    if (extents) XFree(extents);
    return undecorated;
}

void cui__x11_title_bar_event(cui__x11_title_bar *bar, cui_app *app, const XEvent *ev) {
    if (!bar->dpy) return;
    bool mapped = ev->type == MapNotify;
    bool property = ev->type == PropertyNotify;
    if (mapped) bar->frame_known = false;
    /* until the frame is laid out, each move or resize of it asks again */
    bool placed = !bar->frame_known && (ev->type == ConfigureNotify || ev->type == ReparentNotify || ev->type == Expose);
    if (mapped || placed || (property && ev->xproperty.atom == bar->frame_extents)) {
        int undecorated = cui__x11_tb_undecorated(bar);
        bar->frame_known = undecorated >= 0;
        app->title_bar = undecorated > 0;
        app->title_bar_edge = app->title_bar;   /* without decorations a window manager draws no border */
        cui_request_redraw(app->ui);
    }
    if (mapped || (property && ev->xproperty.atom == bar->net_wm_state)) {
        app->title_bar_maximized = cui__plat_window_state(app) == CUI_WINDOW_MAXIMIZED;
        cui_request_redraw(app->ui);
    }
}

/* The _NET_WM_MOVERESIZE direction of the edge band at x, y, or CUI__X11_NO_EDGE. */
static int cui__x11_tb_edge(const cui__x11_title_bar *bar, const cui_app *app, int x, int y, float scale, int w, int h) {
    if (!app->title_bar || !bar->resizable || app->title_bar_maximized) return CUI__X11_NO_EDGE;
    float band = CUI__X11_EDGE * scale;
    bool left = x < band, right = w - x <= band;
    if (y < band) return left ? 0 : right ? 2 : 1;
    if (h - y <= band) return left ? 6 : right ? 4 : 5;
    return left ? 7 : right ? 3 : CUI__X11_NO_EDGE;
}

static void cui__x11_tb_send(const cui__x11_title_bar *bar, long a, long b, long c, long d, long e) {
    XEvent ev;
    memset(&ev, 0, sizeof ev);
    ev.xclient.type = ClientMessage;
    ev.xclient.window = bar->win;
    ev.xclient.message_type = bar->move_resize;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = a;
    ev.xclient.data.l[1] = b;
    ev.xclient.data.l[2] = c;
    ev.xclient.data.l[3] = d;
    ev.xclient.data.l[4] = e;
    XSendEvent(bar->dpy, bar->root, False, SubstructureRedirectMask | SubstructureNotifyMask, &ev);
    XFlush(bar->dpy);
}

/* Lets the window manager move (CUI__X11_MOVE) or size the window from a press at x, y. */
static void cui__x11_tb_hand_over(cui__x11_title_bar *bar, int x_root, int y_root, int dir, Time time) {
    /* the window manager grabs the pointer: give up the grab the press started */
    XUngrabPointer(bar->dpy, time);
    cui__x11_tb_send(bar, x_root, y_root, dir, Button1, 1 /* source: application */);
    bar->press = CUI__X11_PRESS_TAKEN;
}

bool cui__x11_title_bar_button(cui__x11_title_bar *bar, cui_app *app, const XButtonEvent *ev, float scale, int w, int h) {
    if (!bar->dpy || !app->title_bar || ev->button != Button1) return false;
    if (ev->type == ButtonRelease) {
        bool took = bar->press != CUI__X11_PRESS_NONE;
        bar->press = CUI__X11_PRESS_NONE;
        return took;
    }
    /* a window manager that moved or sized the window kept the last release in its grab */
    bar->press = CUI__X11_PRESS_NONE;
    int dir = cui__x11_tb_edge(bar, app, ev->x, ev->y, scale, w, h);
    if (dir != CUI__X11_NO_EDGE) {
        bar->last_click = 0;
        cui__x11_tb_hand_over(bar, ev->x_root, ev->y_root, dir, ev->time);
        return true;
    }
    if (cui_title_bar_hit_test(app->ui, ev->x / scale, ev->y / scale) != CUI_TITLE_BAR_CAPTION) {
        bar->last_click = 0;
        return false;
    }
    bool twice = bar->last_click && (Time)(ev->time - bar->last_click) <= CUI__X11_DOUBLE_CLICK_MS &&
                 abs(ev->x_root - bar->press_x) <= 4 * scale && abs(ev->y_root - bar->press_y) <= 4 * scale;
    if (twice) {
        /* a double click on the caption maximizes or restores, as on the window manager's own */
        bar->last_click = 0;
        bar->press = CUI__X11_PRESS_TAKEN;
        if (bar->resizable) {
            if (app->title_bar_maximized) cui_app_restore(app);
            else cui_app_maximize(app);
        }
        return true;
    }
    /* the window manager gets the press only once it turns into a drag: a click, and the
     * second click of a double click, stay with the app */
    bar->last_click = ev->time;
    bar->press_x = ev->x_root;
    bar->press_y = ev->y_root;
    bar->press = CUI__X11_PRESS_CAPTION;
    return true;
}

void cui__x11_title_bar_motion(cui__x11_title_bar *bar, cui_app *app, const XMotionEvent *ev, float scale, int w, int h, Cursor app_cursor) {
    if (!bar->dpy) return;
    if (bar->press == CUI__X11_PRESS_CAPTION) {
        if (!(ev->state & Button1Mask)) {
            bar->press = CUI__X11_PRESS_NONE;   /* its release went elsewhere */
        } else if (abs(ev->x_root - bar->press_x) > 4 * scale || abs(ev->y_root - bar->press_y) > 4 * scale) {
            /* from the press: the window follows the pointer from where it was taken */
            bar->last_click = 0;
            cui__x11_tb_hand_over(bar, bar->press_x, bar->press_y, CUI__X11_MOVE, ev->time);
            return;
        }
    }
    int dir = cui__x11_tb_edge(bar, app, ev->x, ev->y, scale, w, h);
    if (dir == CUI__X11_NO_EDGE) {
        if (bar->shown) XDefineCursor(bar->dpy, bar->win, app_cursor);
        bar->shown = None;
        return;
    }
    static const unsigned int shapes[8] = {
        XC_top_left_corner, XC_top_side, XC_top_right_corner, XC_right_side,
        XC_bottom_right_corner, XC_bottom_side, XC_bottom_left_corner, XC_left_side
    };
    if (!bar->cursors[dir]) bar->cursors[dir] = XCreateFontCursor(bar->dpy, shapes[dir]);
    if (bar->cursors[dir] != bar->shown) XDefineCursor(bar->dpy, bar->win, bar->cursors[dir]);
    bar->shown = bar->cursors[dir];
}
