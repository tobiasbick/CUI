/* The CUI component gallery app. It draws its own title bar, which the Window
 * page turns off and on and shows the window's state beside. */
#include <cui/cui_app.h>
#include "gallery_ui.h"
#include "gallery_more.h"

static void frame(cui_ctx *ui, void *user) { gallery_frame(ui, user); }

/* the running app's window, for the Window page */
static bool window_title_bar(void) { return cui_app_custom_title_bar(NULL); }
static void window_set_title_bar(bool on) { cui_app_set_custom_title_bar(NULL, on); }
static cui_window_state window_state(void) { return cui_app_window_state(NULL); }
static bool window_placement(cui_window_placement *out) { return cui_app_placement(NULL, out); }
static void window_set_position(int x, int y) { cui_app_set_position(NULL, x, y); }
static void window_maximize(void) { cui_app_maximize(NULL); }
static void window_minimize(void) { cui_app_minimize(NULL); }
static void window_restore(void) { cui_app_restore(NULL); }

int main(void) {
    static const gallery_window window = {
        window_title_bar, window_set_title_bar, window_state, window_placement, window_set_position,
        window_maximize, window_minimize, window_restore
    };
    static gallery g;
    gallery_init(&g);
    gallery_more_set_window(&window);
    return cui_app_run(&(cui_app_desc){ .title = "CUI Gallery", .width = 1240, .height = 820, .min_width = 720, .min_height = 480,
                                        .custom_title_bar = true }, frame, &g);
}
