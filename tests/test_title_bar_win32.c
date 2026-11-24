/* [Documentation](../docs/apps.md#your-own-title-bar)
 * Exercise the custom frame through its HWND. All input goes to the test's
 * own window, including the non-client press and captured client release. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cui/cui_app.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    cui_app *app;
    HWND hwnd;
    cui_rect tool;
    cui_id tool_id, body_id;
    int clicks;
    bool sidebar;
} window;

static int failures;
#define CHECK(what) do { if (!(what)) { fprintf(stderr, "line %d: %s\n", __LINE__, #what); failures++; } } while (0)

/* Step the public UI frame after dispatching a message synchronously. A real
 * pointer elsewhere on the desktop must not interleave a leave with a click. */
static void frame(window *w) {
    cui_ctx *ui = cui_app_ui(w->app);
    cui_vec2 size = cui_app_size(w->app);
    RECT r;
    GetClientRect(w->hwnd, &r);
    if (size.x <= 0 || size.y <= 0) return;
    cui_set_viewport(ui, size.x, size.y, (float)r.right / size.x);
    cui_begin_frame(ui, 1.0f / 60);
    cui_row(ui, .width = CUI_FILL, .height = CUI_FILL) {
        cui_column(ui, .width = CUI_PX(140), .height = CUI_FILL) {
            if (w->sidebar) {
                cui_title_bar(ui, "side", .no_window_buttons = true) { cui_spacer(ui); }
            }
        }
        cui_column(ui, .width = CUI_FILL, .height = CUI_FILL) {
            cui_title_bar(ui, "main") {
                if (cui_button(ui, "Tool", .size = CUI_SIZE_SM)) w->clicks++;
                w->tool_id = cui_last_id(ui);
                w->tool = cui_last_rect(ui);
            }
            cui_button(ui, "Body");
            w->body_id = cui_last_id(ui);
        }
    }
    cui_end_frame(ui);
}

static void settle(window *w) {
    for (int i = 0; i < 3; i++) frame(w);
}

static RECT content(HWND hwnd) {
    RECT r;
    GetClientRect(hwnd, &r);
    MapWindowPoints(hwnd, NULL, (POINT *)&r, 2);
    return r;
}

static LPARAM xy(POINT p) { return MAKELPARAM((WORD)p.x, (WORD)p.y); }

static POINT screen_point(HWND hwnd, POINT p) {
    ClientToScreen(hwnd, &p);
    return p;
}

static LRESULT hit(window *w, POINT p) {
    return SendMessageW(w->hwnd, WM_NCHITTEST, 0, xy(screen_point(w->hwnd, p)));
}

static POINT point(window *w, float x, float y) {
    float scale = cui_dpi_scale(cui_app_ui(w->app));
    return (POINT){ (LONG)lroundf(x * scale), (LONG)lroundf(y * scale) };
}

/* Locate the OS snap target and click the adjacent minimize/close controls
 * without assuming their private IDs or the width of a window button. */
static POINT maximize_point(window *w, POINT *minimize) {
    RECT r;
    GetClientRect(w->hwnd, &r);
    POINT p = point(w, 0, 20);
    LONG right = -1;
    for (p.x = r.right - 1; p.x >= 0; p.x--) {
        if (hit(w, p) == HTMAXBUTTON) {
            if (right < 0) right = p.x;
        } else if (right >= 0) {
            if (minimize) *minimize = (POINT){ p.x - (right - p.x) / 2, p.y };
            p.x = (right + p.x + 1) / 2;
            return p;
        }
    }
    return p;
}

static void mouse(window *w, UINT message, WPARAM buttons, POINT p) {
    SendMessageW(w->hwnd, message, buttons, xy(p));
    frame(w);
}

static void click(window *w, POINT p) {
    LRESULT area = hit(w, p);
    CHECK(area == HTCLIENT || area == HTMAXBUTTON);
    if (area == HTMAXBUTTON) {
        POINT screen = screen_point(w->hwnd, p);
        mouse(w, WM_NCMOUSEMOVE, HTMAXBUTTON, screen);
        mouse(w, WM_NCLBUTTONDOWN, HTMAXBUTTON, screen);
    } else {
        mouse(w, WM_MOUSEMOVE, 0, p);
        mouse(w, WM_LBUTTONDOWN, MK_LBUTTON, p);
    }
    mouse(w, WM_LBUTTONUP, 0, p);
}

static void double_caption(window *w, POINT p) {
    LRESULT area = hit(w, p);
    CHECK(area == HTCAPTION);
    if (area == HTCAPTION)
        SendMessageW(w->hwnd, WM_NCLBUTTONDBLCLK, area, xy(screen_point(w->hwnd, p)));
    settle(w);
}

static void check_edges(window *w) {
    RECT r;
    GetWindowRect(w->hwnd, &r);
    LONG x[] = { r.left + 1, (r.left + r.right) / 2, r.right - 2 };
    LONG y[] = { r.top + 1, (r.top + r.bottom) / 2, r.bottom - 2 };
    const int expected[3][3] = {
        { HTTOPLEFT, HTTOP, HTTOPRIGHT }, { HTLEFT, HTCLIENT, HTRIGHT },
        { HTBOTTOMLEFT, HTBOTTOM, HTBOTTOMRIGHT }
    };
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            int actual = (int)SendMessageW(w->hwnd, WM_NCHITTEST, 0, xy((POINT){ x[col], y[row] }));
            if (actual != expected[row][col]) {
                fprintf(stderr, "edge (%d,%d): got %d, expected %d\n", row, col, actual, expected[row][col]);
                failures++;
            }
        }
    }
}

static void toggle(window *w, bool custom) {
    cui_app_set_custom_title_bar(w->app, custom);
    settle(w);
    CHECK(cui_app_custom_title_bar(w->app) == custom);
    CHECK(hit(w, point(w, 68, 20)) == (custom ? HTCAPTION : HTCLIENT));
}

static void tab(window *w, cui_id expected) {
    SendMessageW(w->hwnd, WM_KEYDOWN, VK_TAB, 0);
    SendMessageW(w->hwnd, WM_KEYUP, VK_TAB, 0);
    frame(w);
    CHECK(cui_focused_id(cui_app_ui(w->app)) == expected);
}

static void check_hover_and_close(window *w) {
    cui_ctx *ui = cui_app_ui(w->app);
    cui_set_reduced_motion(ui, true);
    POINT close = point(w, cui_app_size(w->app).x - 12, 18);
    mouse(w, WM_MOUSEMOVE, 0, close);
    RECT r;
    GetClientRect(w->hwnd, &r);
    unsigned char *pixels = malloc((size_t)r.right * r.bottom * 4);
    bool captured = pixels && cui_app_screenshot(w->app, pixels, r.right, r.bottom);
    CHECK(captured);
    if (captured) {
        const unsigned char *rgb = pixels + 4 * ((size_t)close.y * r.right + close.x);
        CHECK(rgb[0] > rgb[1] + 40 && rgb[0] > rgb[2] + 40);
    }
    free(pixels);
    click(w, close);
    CHECK(!cui_app_frame(w->app));
}

static void exercise(window *w, bool fixed) {
    settle(w);
    POINT side = point(w, 68, 20), caption = point(w, 300, 20);
    POINT tool = point(w, w->tool.x + w->tool.w / 2, w->tool.y + w->tool.h / 2);
    RECT original = content(w->hwnd);
    cui_vec2 size = cui_app_size(w->app);
    if (fabsf(size.x - 560) > 1 || fabsf(size.y - 320) > 1)
        fprintf(stderr, "initial client %.1f x %.1f, expected 560 x 320\n", size.x, size.y);
    CHECK(fabsf(size.x - 560) <= 1 && fabsf(size.y - 320) <= 1);
    CHECK(cui_app_custom_title_bar(w->app));
    CHECK(hit(w, side) == HTCAPTION && hit(w, caption) == HTCAPTION);
    CHECK(w->tool.w > 0 && hit(w, tool) == HTCLIENT);
    CHECK(hit(w, point(w, 70, 70)) == HTCLIENT);
    w->sidebar = false;
    settle(w);
    CHECK(hit(w, side) == HTCLIENT);
    w->sidebar = true;
    settle(w);
    mouse(w, WM_MOUSEMOVE, 0, tool);
    mouse(w, WM_LBUTTONDOWN, MK_LBUTTON, tool);
    mouse(w, WM_MOUSEMOVE, MK_LBUTTON, caption);
    CHECK(hit(w, caption) == HTCLIENT);
    mouse(w, WM_LBUTTONUP, 0, caption);
    CHECK(w->clicks == 0);
    click(w, tool);
    CHECK(w->clicks == 1);
    tab(w, w->body_id);
    tab(w, w->tool_id);

    POINT minimize = {0};
    POINT maximize = maximize_point(w, &minimize);
    if (fixed) {
        CHECK(maximize.x < 0);
        CHECK(hit(w, point(w, 68, 1)) == HTCAPTION);
        double_caption(w, side);
        CHECK(!IsZoomed(w->hwnd));
    } else {
        check_edges(w);
        CHECK(maximize.x >= 0);
        if (maximize.x >= 0) {
            click(w, minimize);
            CHECK(IsIconic(w->hwnd));
            cui_app_restore(w->app);
            settle(w);
            click(w, maximize);
            CHECK(IsZoomed(w->hwnd));
            settle(w);
            maximize = maximize_point(w, NULL);
            CHECK(maximize.x >= 0);
            if (maximize.x >= 0) click(w, maximize);
            CHECK(!IsZoomed(w->hwnd));
            settle(w);
        }
        double_caption(w, side);
        CHECK(IsZoomed(w->hwnd));
        double_caption(w, caption);
        CHECK(!IsZoomed(w->hwnd));
    }

    toggle(w, false);
    RECT current = content(w->hwnd);
    CHECK(EqualRect(&current, &original));
    if (!fixed) {
        cui_app_maximize(w->app);
        settle(w);
        toggle(w, true);
        CHECK(IsZoomed(w->hwnd));
        cui_app_restore(w->app);
        settle(w);
    } else toggle(w, true);
    current = content(w->hwnd);
    if (!EqualRect(&current, &original)) fprintf(stderr, "restore: expected (%ld,%ld)-(%ld,%ld), got (%ld,%ld)-(%ld,%ld)\n",
        original.left, original.top, original.right, original.bottom, current.left, current.top, current.right, current.bottom);
    CHECK(EqualRect(&current, &original));
    if (!fixed) {
        /* A minimized window must still restore to maximized first, then to
         * its original content rectangle after changing the frame. */
        cui_app_maximize(w->app);
        cui_app_minimize(w->app);
        cui_app_set_custom_title_bar(w->app, false);
        CHECK(IsIconic(w->hwnd));
        cui_app_restore(w->app);
        CHECK(IsZoomed(w->hwnd));
        cui_app_restore(w->app);
        settle(w);
        current = content(w->hwnd);
        CHECK(EqualRect(&current, &original));
        toggle(w, true);
    }
    check_hover_and_close(w);
}

int main(void) {
    for (int fixed = 0; fixed < 2; fixed++) {
        window w = {0};
        w.sidebar = true;
        cui_window_placement at = { .x = 100, .y = 120, .width = 560, .height = 320, .has_position = true };
        w.app = cui_app_create(&(cui_app_desc){ .title = "CUI title bar test",
            .placement = &at, .custom_title_bar = true, .fixed_size = fixed != 0, .continuous = true });
        if (!w.app) { fprintf(stderr, "could not open the Win32 test window\n"); return 1; }
        w.hwnd = cui_app_native_window(w.app);
        printf("%s window, %s rendering\n", fixed ? "fixed" : "resizable",
               cui_app_is_software(w.app) ? "software" : "hardware API");
        exercise(&w, fixed != 0);
        cui_app_destroy(w.app);
    }
    printf("%d failures\n", failures);
    return failures ? 1 : 0;
}
