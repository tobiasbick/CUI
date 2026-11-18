/* Title bar: a window that draws its own title bar with cui_title_bar, with
 * navigation, a search field and actions beside the window buttons.
 * --system-frame keeps the system's title bar (the same bar, without window
 * buttons), --fixed-size makes the window not resizable and --software draws
 * with the software renderer. */
#include <cui/cui.h>
#include <cui/cui_app.h>

#include <stdio.h>
#include <string.h>

typedef struct {
    char query[96];
    int page;
} demo;

static void frame(cui_ctx *ui, void *user) {
    demo *d = user;
    cui_title_bar(ui, "title", .icon = CUI_ICON_SQUARE_TERMINAL, .title = "Workspace") {
        if (cui_icon_button(ui, CUI_ICON_ARROW_LEFT, .size = CUI_SIZE_SM, .tooltip = "Back", .disabled = d->page == 0))
            d->page--;
        if (cui_icon_button(ui, CUI_ICON_ARROW_RIGHT, .size = CUI_SIZE_SM, .tooltip = "Forward"))
            d->page++;
        cui_spacer(ui);
        cui_search(ui, "find", d->query, sizeof d->query, .placeholder = "Search", .size = CUI_SIZE_SM, .width = CUI_PX(280));
        cui_spacer(ui);
        bool dark = cui_get_theme_mode(ui) == CUI_THEME_DARK;
        if (cui_icon_button(ui, dark ? CUI_ICON_SUN : CUI_ICON_MOON, .size = CUI_SIZE_SM, .tooltip = "Switch theme"))
            cui_set_theme(ui, dark ? CUI_THEME_LIGHT : CUI_THEME_DARK);
    }
    cui_column(ui, .pad = 24, .gap = 8) {
        char line[64];
        snprintf(line, sizeof line, "Page %d", d->page + 1);
        cui_text(ui, line, .size = 22, .font = CUI_FONT_MEDIUM);
        cui_text(ui, d->query[0] ? d->query : "Drag the empty part of the title bar to move the window.",
                 .color = CUI_COLOR_TEXT_MUTED);
    }
}

int main(int argc, char **argv) {
    cui_app_desc desc = { .title = "Title bar", .width = 860, .height = 480, .custom_title_bar = true };
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--system-frame") == 0) desc.custom_title_bar = false;
        if (strcmp(argv[i], "--fixed-size") == 0) desc.fixed_size = true;
        if (strcmp(argv[i], "--software") == 0) desc.renderer = CUI_RENDERER_SOFTWARE;
    }
    demo d;
    memset(&d, 0, sizeof d);
    return cui_app_run(&desc, frame, &d);
}
