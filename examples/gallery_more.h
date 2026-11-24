/* Extended gallery pages: the newer components, one page per family. */
#ifndef GALLERY_MORE_H
#define GALLERY_MORE_H

#include <cui/cui.h>
#include <cui/cui_app.h>   /* the window's types; the calls go through gallery_window */

/* What the Window page does to the gallery's window. The gallery app passes the cui_app calls;
 * without them (tests, screenshots) the page says it needs a window. */
typedef struct gallery_window {
    bool (*title_bar)(void);                    /* the app's own title bar is in effect */
    void (*set_title_bar)(bool on);
    cui_window_state (*state)(void);
    bool (*placement)(cui_window_placement *out);
    void (*set_position)(int x, int y);
    void (*maximize)(void), (*minimize)(void), (*restore)(void);
} gallery_window;
/* Native actions used when the gallery owns a running app window. */
extern const gallery_window gallery_app_window;
void gallery_more_set_window(const gallery_window *window);
/* True while the gallery draws its own title bar; false without a window (tests, screenshots). */
bool gallery_more_title_bar(void);

int gallery_more_count(void);
const char *gallery_more_name(int page);
const char *gallery_more_icon(int page);
void gallery_more_page(cui_ctx *ui, int page, bool *sidebar_below_titlebar);

#endif
