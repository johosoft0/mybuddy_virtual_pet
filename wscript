/**
 * ui.c - UI system: status bar, action menu, stats view
 *
 * Bottom 48px of screen. Three modes:
 *   STATUS — clock + mood text + mini stat icons
 *   MENU   — scrollable action list, UP/DOWN/SELECT
 *   STATS  — full stat readout
 */

#include "ui.h"
#include <pebble.h>

// ── Layout ──
#define UI_HEIGHT     48
#define MENU_ITEM_H   16
#define BAR_MINI_W    30
#define BAR_MINI_H    5

// ── Menu labels ──
static const char* s_menu_labels[] = {
    "Feed",
    "Play",
    "Medicine",
    "Stats",
};

const char* ui_menu_label(MenuItem item) {
    if (item < MENU_COUNT) return s_menu_labels[item];
    return "???";
}

// ═══════════════════════════════════════════
//  UI STATE MANAGEMENT
// ═══════════════════════════════════════════

void ui_init(UIState *ui) {
    ui->mode = UI_MODE_STATUS;
    ui->menu_sel = 0;
    ui->flash_timer = 0;
}

void ui_menu_open(UIState *ui) {
    ui->mode = UI_MODE_MENU;
    ui->menu_sel = 0;
}

void ui_menu_close(UIState *ui) {
    ui->mode = UI_MODE_STATUS;
}

void ui_menu_up(UIState *ui) {
    if (ui->menu_sel > 0) ui->menu_sel--;
    else ui->menu_sel = MENU_COUNT - 1;  // wrap
}

void ui_menu_down(UIState *ui) {
    if (ui->menu_sel < MENU_COUNT - 1) ui->menu_sel++;
    else ui->menu_sel = 0;  // wrap
}

MenuItem ui_menu_select(UIState *ui) {
    MenuItem sel = (MenuItem)ui->menu_sel;
    if (sel == MENU_STATS) {
        ui->mode = UI_MODE_STATS;
    } else {
        ui->mode = UI_MODE_STATUS;
        ui->flash_timer = 4;  // flash feedback for 2 seconds
    }
    return sel;
}

// ═══════════════════════════════════════════
//  DRAWING HELPERS
// ═══════════════════════════════════════════

static GColor bar_color(int16_t val) {
    if (val > 60) return GColorGreen;
    if (val > 30) return GColorChromeYellow;
    return GColorRed;
}

static void draw_mini_bar(GContext *ctx, int x, int y, int16_t val) {
    // Background
    graphics_context_set_fill_color(ctx, GColorDarkGray);
    graphics_fill_rect(ctx, GRect(x, y, BAR_MINI_W, BAR_MINI_H), 1, GCornersAll);
    // Fill
    int fw = (val * BAR_MINI_W) / STAT_MAX;
    if (fw > 0) {
        graphics_context_set_fill_color(ctx, bar_color(val));
        graphics_fill_rect(ctx, GRect(x, y, fw, BAR_MINI_H), 1, GCornersAll);
    }
}

// ═══════════════════════════════════════════
//  MODE: STATUS (default view)
// ═══════════════════════════════════════════

static void draw_status(GContext *ctx, GRect bounds, const PetState *pet) {
    GFont font_sm = fonts_get_system_font(FONT_KEY_GOTHIC_14);
    GFont font_time = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);

    // ── Clock (left side) ──
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_buf[8];
    strftime(time_buf, sizeof(time_buf), "%I:%M", t);
    // Strip leading zero
    char *tstr = time_buf;
    if (tstr[0] == '0') tstr++;

    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, tstr, font_time,
        GRect(4, 2, 64, 28),
        GTextOverflowModeTrailingEllipsis,
        GTextAlignmentLeft, NULL);

    // AM/PM
    char ap_buf[4];
    strftime(ap_buf, sizeof(ap_buf), "%p", t);
    graphics_context_set_text_color(ctx, GColorDarkGray);
    graphics_draw_text(ctx, ap_buf, font_sm,
        GRect(4, 26, 30, 16),
        GTextOverflowModeTrailingEllipsis,
        GTextAlignmentLeft, NULL);

    // ── Mood text (center) ──
    const char *mood_str = "";
    GColor mood_color = GColorLightGray;
    if (!pet->is_alive) {
        mood_str = "R.I.P."; mood_color = GColorDarkGray;
    } else {
        switch ((PetMood)pet->mood) {
            case MOOD_HAPPY:  mood_str = "Happy";  mood_color = GColorGreen; break;
            case MOOD_CONTENT:mood_str = "Content"; mood_color = GColorLightGray; break;
            case MOOD_HUNGRY: mood_str = "Hungry!"; mood_color = GColorYellow; break;
            case MOOD_SAD:    mood_str = "Sad";     mood_color = GColorPictonBlue; break;
            case MOOD_SICK:   mood_str = "Sick!";   mood_color = GColorRed; break;
            default: break;
        }
    }
    graphics_context_set_text_color(ctx, mood_color);
    graphics_draw_text(ctx, mood_str, font_sm,
        GRect(40, 28, 64, 16),
        GTextOverflowModeTrailingEllipsis,
        GTextAlignmentCenter, NULL);

    // ── Mini stat bars (right side) ──
    int bx = bounds.size.w - BAR_MINI_W - 6;

    // Labels
    graphics_context_set_text_color(ctx, GColorDarkGray);
    int lx = bx - 18;
    graphics_draw_text(ctx, "H", font_sm, GRect(lx, 2, 16, 14),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
    graphics_draw_text(ctx, "J", font_sm, GRect(lx, 14, 16, 14),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
    graphics_draw_text(ctx, "+", font_sm, GRect(lx, 26, 16, 14),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);

    draw_mini_bar(ctx, bx, 6,  pet->hunger);
    draw_mini_bar(ctx, bx, 18, pet->happiness);
    draw_mini_bar(ctx, bx, 30, pet->health);

    // ── "Press SELECT" hint ──
    graphics_context_set_text_color(ctx, GColorDarkGray);
    GFont font_tiny = fonts_get_system_font(FONT_KEY_GOTHIC_14);
    graphics_draw_text(ctx, "[ MENU ]", font_tiny,
        GRect(44, 8, 56, 14),
        GTextOverflowModeTrailingEllipsis,
        GTextAlignmentCenter, NULL);
}

// ═══════════════════════════════════════════
//  MODE: MENU (action selector)
// ═══════════════════════════════════════════

static void draw_menu(GContext *ctx, GRect bounds, const UIState *ui) {
    GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    GFont font_hint = fonts_get_system_font(FONT_KEY_GOTHIC_14);

    // Menu background
    graphics_context_set_fill_color(ctx, GColorOxfordBlue);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);

    // Draw visible items (show up to 3 at a time, centered on selection)
    int start = ui->menu_sel - 1;
    if (start < 0) start = 0;
    if (start > MENU_COUNT - 3) start = MENU_COUNT - 3;
    if (start < 0) start = 0;

    for (int i = 0; i < 3 && (start + i) < MENU_COUNT; i++) {
        int idx = start + i;
        int y = 2 + i * MENU_ITEM_H;
        bool selected = (idx == ui->menu_sel);

        if (selected) {
            // Highlight bar
            graphics_context_set_fill_color(ctx, GColorWhite);
            graphics_fill_rect(ctx, GRect(4, y, bounds.size.w - 8, MENU_ITEM_H - 2),
                               3, GCornersAll);
            graphics_context_set_text_color(ctx, GColorBlack);
        } else {
            graphics_context_set_text_color(ctx, GColorLightGray);
        }

        graphics_draw_text(ctx, s_menu_labels[idx],
            selected ? font : font_hint,
            GRect(12, y - 2, bounds.size.w - 24, MENU_ITEM_H),
            GTextOverflowModeTrailingEllipsis,
            GTextAlignmentLeft, NULL);
    }

    // Scroll indicators
    graphics_context_set_text_color(ctx, GColorCadetBlue);
    if (start > 0) {
        graphics_draw_text(ctx, "\x18", font_hint,
            GRect(bounds.size.w - 16, 0, 14, 14),
            GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    }
    if (start + 3 < MENU_COUNT) {
        graphics_draw_text(ctx, "\x19", font_hint,
            GRect(bounds.size.w - 16, 34, 14, 14),
            GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    }
}

// ═══════════════════════════════════════════
//  MODE: STATS (full readout)
// ═══════════════════════════════════════════

static void draw_stats(GContext *ctx, GRect bounds, const PetState *pet) {
    GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_14);

    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);

    char buf[32];

    // Row 1: Hunger + Happiness
    snprintf(buf, sizeof(buf), "HGR:%d  HPY:%d", pet->hunger, pet->happiness);
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, buf, font, GRect(4, 0, bounds.size.w - 8, 16),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

    // Row 2: Health + Age
    snprintf(buf, sizeof(buf), "HP:%d  Age:%dd", pet->health, pet->age_days);
    graphics_draw_text(ctx, buf, font, GRect(4, 14, bounds.size.w - 8, 16),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

    // Row 3: Sleeping status
    const char *sleep_str = pet->is_sleeping ? "Sleeping" : "Awake";
    snprintf(buf, sizeof(buf), "%s", sleep_str);
    graphics_context_set_text_color(ctx, GColorCadetBlue);
    graphics_draw_text(ctx, buf, font, GRect(4, 28, bounds.size.w - 8, 16),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

    // Back hint
    graphics_context_set_text_color(ctx, GColorDarkGray);
    graphics_draw_text(ctx, "BACK", font,
        GRect(bounds.size.w - 36, 28, 32, 16),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
}

// ═══════════════════════════════════════════
//  PUBLIC API
// ═══════════════════════════════════════════

void ui_draw(GContext *ctx, GRect bounds, const PetState *pet,
             const UIState *ui) {
    // Background
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);

    // Divider
    graphics_context_set_stroke_color(ctx, GColorDarkGray);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, GPoint(0, 0), GPoint(bounds.size.w, 0));

    // Dead override
    if (!pet->is_alive && ui->mode != UI_MODE_MENU) {
        GFont big = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
        graphics_context_set_text_color(ctx, GColorRed);
        graphics_draw_text(ctx, "Press SELECT to revive!", big,
            GRect(2, 12, bounds.size.w - 4, 24),
            GTextOverflowModeTrailingEllipsis,
            GTextAlignmentCenter, NULL);
        return;
    }

    switch (ui->mode) {
        case UI_MODE_MENU:
            draw_menu(ctx, bounds, ui);
            break;
        case UI_MODE_STATS:
            draw_stats(ctx, bounds, pet);
            break;
        default:
            draw_status(ctx, bounds, pet);
            break;
    }
}