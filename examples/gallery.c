/* The CUI component gallery app. It draws its own title bar, which the Window
 * page turns off and on and shows the window's state beside. */
#include <cui/cui_app.h>
#include "gallery_ui.h"
#include "gallery_more.h"

static void frame(cui_ctx *ui, void *user) { gallery_frame(ui, user); }

int main(void) {
    static gallery g;
    gallery_init(&g);
    gallery_more_set_window(&gallery_app_window);
    return cui_app_run(&(cui_app_desc){ .title = "CUI Gallery", .width = 1240, .height = 820, .min_width = 720, .min_height = 480,
                                        .custom_title_bar = true }, frame, &g);
}
