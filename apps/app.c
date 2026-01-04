#include "app.h"
#include "../gui.h"
#include "../widgets.h"

static AppContext ctx;
AppContext* app = &ctx;

void app_begin(int x, int y, int w, int h, int selected) {
    ctx.x = x;
    ctx.y = y;
    ctx.w = w;
    ctx.h = h;
    ctx.selected = selected;
}

void app_end() {
    // reserved for future cleanup
}

void app_clear() {
    draw_box(app->x + 1, app->y + 1, app->w - 2, app->h - 2, 0x70);
}

void app_label(int x, int y, const char* text) {
    draw_text_at(app->x + x, app->y + y, text, 0x70);
}

int app_button(int x, int y, const char* text) {
    int px = app->x + x;
    int py = app->y + y;

    widget_button(px, py, text, NULL);
    return check_button_click(
        get_mouse_x(),
        get_mouse_y(),
        px,
        py,
        text
    );
}
