/* [Documentation](../docs/apps.md#your-own-title-bar) */
#include "gallery_more.h"

static bool window_title_bar(void) { return cui_app_custom_title_bar(NULL); }
static void window_set_title_bar(bool on) { cui_app_set_custom_title_bar(NULL, on); }
static cui_window_state window_state(void) { return cui_app_window_state(NULL); }
static bool window_placement(cui_window_placement *out) { return cui_app_placement(NULL, out); }
static void window_set_position(int x, int y) { cui_app_set_position(NULL, x, y); }
static void window_maximize(void) { cui_app_maximize(NULL); }
static void window_minimize(void) { cui_app_minimize(NULL); }
static void window_restore(void) { cui_app_restore(NULL); }

const gallery_window gallery_app_window = {
    window_title_bar, window_set_title_bar, window_state, window_placement, window_set_position,
    window_maximize, window_minimize, window_restore
};
