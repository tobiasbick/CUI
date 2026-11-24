/* [Documentation](../docs/apps.md#your-own-title-bar)
 * Drive the gallery's Window page and check what the page reports against
 * the native window. Optional output paths save the default and shared title bars. */
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

static void drag_divider(float x, float y, float dx) {
    cui_input_mouse_move(ui, x, y);
    draw();
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, true);
    draw();
    cui_input_mouse_move(ui, x + dx, y);
    draw();
    cui_input_mouse_button(ui, CUI_MOUSE_LEFT, false);
    draw();
    draw();
}

static cui_rect gallery_rect(const char *id) { return cui_get_rect(ui, cui_make_id(ui, id)); }

static bool exercise(const char *image, const char *shared_image) {
    const char *toggle = "Draw the title bar in the app";
    const char *below = "Start sidebar below the title bar";
    cui_set_reduced_motion(ui, true);
    for (int i = 0; i < 4; i++) draw();
    /* Window is in the Extended section, below the initial scroll position. */
    cui_input_mouse_move(ui, 100, 200);
    cui_input_scroll(ui, 0, -80);
    for (int i = 0; i < 40; i++) draw();
    EXPECT(click("Window"));
    EXPECT(strcmp(gallery_page_name(g.page), "Window") == 0);
    EXPECT(locate("In effect", NULL) && locate("Normal", NULL));
    cui_rect left = gallery_rect("pages"), title = gallery_rect("title");
    EXPECT(left.x == 0 && left.y == 0 && left.w > 0 && left.h == title.h && title.h > 0);
    EXPECT(title.y == 0 && title.x >= left.w && gallery_rect("pages-split").y == 0);
    EXPECT(cui_title_bar_hit_test(ui, left.w, title.h / 2) == CUI_TITLE_BAR_NONE);
    if (image) EXPECT(screenshot(image));

    float sidebar_width = left.w;
    EXPECT(click(below));
    title = gallery_rect("title");
    EXPECT(title.x == 0 && title.y == 0 && title.w == cui_viewport_size(ui).x);
    EXPECT(gallery_rect("pages-split").y == title.h && title.h > 0);
    EXPECT(cui_title_bar_hit_test(ui, sidebar_width, title.h / 2) == CUI_TITLE_BAR_CAPTION);
    if (shared_image) EXPECT(screenshot(shared_image));
    float page_x = gallery_rect("main").x;
    drag_divider(sidebar_width, title.h + 80, 30);
    EXPECT(gallery_rect("main").x > page_x + 25);
    page_x = gallery_rect("main").x;
    EXPECT(click(CUI_ICON_PANEL_LEFT) && gallery_rect("main").x == 0);
    EXPECT(click(CUI_ICON_PANEL_LEFT) && gallery_rect("main").x == page_x);
    cui_vec2 wide = cui_app_size(app);
    cui_app_set_size(app, 700, (int)wide.y);
    draw();
    EXPECT(click(CUI_ICON_MENU) && g.drawer);
    cui_app_set_size(app, (int)wide.x, (int)wide.y);
    draw();
    draw();
    EXPECT(!g.drawer && gallery_rect("main").x == page_x);
    EXPECT(click(below));
    left = gallery_rect("pages");
    EXPECT(left.y == 0 && left.w > sidebar_width + 25 && gallery_rect("title").x == page_x);
    EXPECT(gallery_rect("pages-split").y == 0);
    EXPECT(cui_title_bar_hit_test(ui, left.w, left.h / 2) == CUI_TITLE_BAR_NONE);
    cui_vec2 panel_before, panel_after;
    EXPECT(locate(CUI_ICON_PANEL_LEFT, &panel_before));
    drag_divider(left.w, left.h / 2, 55);
    EXPECT(locate(CUI_ICON_PANEL_LEFT, &panel_after) && panel_after.x > panel_before.x + 45);
    EXPECT(click(CUI_ICON_PANEL_LEFT) && g.sidebar_hidden);
    EXPECT(click(CUI_ICON_PANEL_LEFT) && !g.sidebar_hidden);
    EXPECT(click(CUI_ICON_MOON) && g.dark && cui_get_theme_mode(ui) == CUI_THEME_DARK);
    EXPECT(click(CUI_ICON_SUN) && !g.dark && cui_get_theme_mode(ui) == CUI_THEME_LIGHT);
    EXPECT(click(toggle) && !cui_app_custom_title_bar(app));
    EXPECT(locate("System title bar", NULL));
    EXPECT(gallery_rect("title").x > 0);
    EXPECT(click(below) && gallery_rect("title").w == cui_viewport_size(ui).x);
    EXPECT(click("Maximize") && IsZoomed(hwnd));
    EXPECT(locate("Maximized", NULL));
    EXPECT(click(toggle) && cui_app_custom_title_bar(app) && IsZoomed(hwnd));
    EXPECT(locate("In effect", NULL));
    EXPECT(gallery_rect("pages-split").y == gallery_rect("title").h);
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
    bool ok = exercise(argc > 1 ? argv[1] : NULL, argc > 2 ? argv[2] : NULL);
    cui_app_destroy(app);
    return ok ? 0 : 1;
}
