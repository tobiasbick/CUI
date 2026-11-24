/* [Documentation](../docs/apps.md#your-own-title-bar)
 * Drive the gallery's Window page and check what the page reports against
 * the native window. An optional output path saves the page for inspection. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../examples/gallery_ui.h"
#include "../examples/gallery_more.h"
#include "../src/cui_internal.h"
#include <cui/cui_render.h>

#include <stdio.h>

static cui_app *app;
static cui_ctx *ui;
static HWND hwnd;
static gallery g;

static void draw(void) {
    cui_vec2 size = cui_app_size(app);
    RECT r;
    GetClientRect(hwnd, &r);
    if (size.x <= 0 || size.y <= 0) return;
    cui_set_viewport(ui, size.x, size.y, (float)r.right / size.x);
    cui_begin_frame(ui, 1.0f / 60);
    gallery_frame(ui, &g);
    cui_end_frame(ui);
}

/* Locate controls by the text or icon laid out in the gallery. */
static bool locate(const char *label, cui_vec2 *center) {
    int length = (int)strlen(label);
    for (int i = 0; i < ui->node_count; i++) {
        const cui_node *n = &ui->nodes[i];
        if (n->text_len != length || !n->text || memcmp(n->text, label, length)) continue;
        if (center) *center = (cui_vec2){ n->rect.x + n->rect.w / 2, n->rect.y + n->rect.h / 2 };
        return true;
    }
    return false;
}

static bool click(const char *label) {
    cui_vec2 p;
    if (!locate(label, &p)) { fprintf(stderr, "missing gallery label: %s\n", label); return false; }
    cui_input_mouse_move(ui, p.x, p.y);
    draw();
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    draw();
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    draw();
    draw();
    return true;
}

static bool screenshot(const char *path) {
    const cui_draw_data *dd = cui_get_draw_data(ui);
    int w = dd->width_px, h = dd->height_px;
    unsigned char *pixels = malloc((size_t)w * h * 4);
    bool ok = pixels && cui_app_screenshot(app, pixels, w, h) && cui_write_png(path, pixels, w, h, w * 4);
    free(pixels);
    return ok;
}

#define EXPECT(test) do { if (!(test)) { fprintf(stderr, "line %d: %s\n", __LINE__, #test); return false; } } while (0)

static bool exercise(const char *image) {
    const char *toggle = "Draw the title bar in the app";
    cui_set_reduced_motion(ui, true);
    for (int i = 0; i < 4; i++) draw();
    /* Window is in the Extended section, below the initial scroll position. */
    cui_input_mouse_move(ui, 100, 200);
    cui_input_scroll(ui, 0, -80);
    for (int i = 0; i < 40; i++) draw();
    EXPECT(click("Window"));
    EXPECT(strcmp(gallery_page_name(g.page), "Window") == 0);
    EXPECT(locate("In effect", NULL) && locate("Normal", NULL));
    if (image) EXPECT(screenshot(image));

    cui_vec2 panel_before, panel_after;
    EXPECT(locate(CUI_ICON_PANEL_LEFT, &panel_before));
    cui_input_mouse_move(ui, g.sidebar_width, 120);
    draw();
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    draw();
    cui_input_mouse_move(ui, g.sidebar_width + 55, 120);
    draw();
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    draw();
    EXPECT(locate(CUI_ICON_PANEL_LEFT, &panel_after) && panel_after.x > panel_before.x + 45);
    EXPECT(click(CUI_ICON_PANEL_LEFT) && g.sidebar_hidden);
    EXPECT(click(CUI_ICON_PANEL_LEFT) && !g.sidebar_hidden);
    EXPECT(click(CUI_ICON_MOON) && g.dark && cui_get_theme_mode(ui) == CUI_THEME_DARK);
    EXPECT(click(CUI_ICON_SUN) && !g.dark && cui_get_theme_mode(ui) == CUI_THEME_LIGHT);
    EXPECT(click(toggle) && !cui_app_custom_title_bar(app));
    EXPECT(locate("System title bar", NULL));
    EXPECT(click("Maximize") && IsZoomed(hwnd));
    EXPECT(locate("Maximized", NULL));
    EXPECT(click(toggle) && cui_app_custom_title_bar(app) && IsZoomed(hwnd));
    EXPECT(locate("In effect", NULL));
    EXPECT(click("Restore") && !IsZoomed(hwnd) && locate("Normal", NULL));

    POINT saved = {0};
    ClientToScreen(hwnd, &saved);
    EXPECT(click("Remember this place"));
    cui_app_set_position(app, saved.x + 40, saved.y + 30);
    draw();
    POINT moved = {0};
    ClientToScreen(hwnd, &moved);
    EXPECT(moved.x != saved.x && moved.y != saved.y);
    EXPECT(click("Go back there"));
    POINT returned = {0};
    ClientToScreen(hwnd, &returned);
    EXPECT(saved.x == returned.x && saved.y == returned.y);
    EXPECT(click("Minimize") && IsIconic(hwnd));
    return true;
}

int main(int argc, char **argv) {
    cui_window_placement at = { .x = 90, .y = 110, .width = 1160, .height = 740, .has_position = true };
    app = cui_app_create(&(cui_app_desc){ .title = "CUI Gallery Window test", .custom_title_bar = true, .placement = &at });
    if (!app) return 1;
    ui = cui_app_ui(app);
    hwnd = cui_app_native_window(app);
    gallery_init(&g);
    gallery_more_set_window(&gallery_app_window);
    bool ok = exercise(argc > 1 ? argv[1] : NULL);
    cui_app_destroy(app);
    return ok ? 0 : 1;
}
