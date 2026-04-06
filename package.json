/**
 * ui.h - UI system: status bar, action menu, stats screen
 *
 * The bottom 48px of the 168px screen is the UI area (pet gets 120px).
 * Three modes:
 *   UI_MODE_STATUS  — default: clock, mood icon, mini stat bars
 *   UI_MODE_MENU    — action menu: UP/DOWN scroll, SELECT confirm
 *   UI_MODE_STATS   — full stat readout with age/weight
 *
 * SELECT toggles menu open/closed. BACK from menu returns to status.
 */

#pragma once
#include <pebble.h>
#include "pet_state.h"

// ── UI Modes ──
typedef enum {
    UI_MODE_STATUS,
    UI_MODE_MENU,
    UI_MODE_STATS,
} UIMode;

// ── Menu Items ──
typedef enum {
    MENU_FEED,
    MENU_PLAY,
    MENU_HEAL,
    MENU_STATS,
    MENU_COUNT,    // total number of items
} MenuItem;

// ── UI State (managed in main.c, passed by pointer) ──
typedef struct {
    UIMode   mode;
    int8_t   menu_sel;     // currently highlighted menu item
    uint8_t  flash_timer;  // countdown for action feedback flash
} UIState;

// Initialize UI state
void ui_init(UIState *ui);

// Draw the UI bar into the given context and bounds
void ui_draw(GContext *ctx, GRect bounds, const PetState *pet,
             const UIState *ui);

// Menu navigation
void ui_menu_open(UIState *ui);
void ui_menu_close(UIState *ui);
void ui_menu_up(UIState *ui);
void ui_menu_down(UIState *ui);
MenuItem ui_menu_select(UIState *ui);  // returns selected item, closes menu

// Get label for menu item
const char* ui_menu_label(MenuItem item);