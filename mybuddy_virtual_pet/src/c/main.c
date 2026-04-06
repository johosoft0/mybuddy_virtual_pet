/**
 * PebblePet - Virtual Pet for Pebble Color Watch
 * main.c - App lifecycle, window, menu-driven input, timers
 *
 * Button mapping:
 *   STATUS mode: SELECT opens menu
 *   MENU mode:   UP/DOWN scroll, SELECT confirms, BACK closes
 *   STATS mode:  any button returns to status
 *
 * Layout: pet layer 0-119 (120px), UI layer 120-167 (48px)
 */

#include <pebble.h>
#include "pet_state.h"
#include "pet_render.h"
#include "ui.h"

// ── Window & Layers ──
static Window *s_main_window;
static Layer  *s_pet_layer;
static Layer  *s_ui_layer;

// ── Timers ──
static AppTimer *s_anim_timer;

// ── State ──
static PetState s_pet;
static UIState  s_ui;
static uint32_t s_frame_counter;

// ── Forward Declarations ──
static void schedule_anim_timer(void);
static void redraw_all(void);

// ═══════════════════════════════════════════
//  REDRAW HELPER
// ═══════════════════════════════════════════

static void redraw_all(void) {
    layer_mark_dirty(s_pet_layer);
    layer_mark_dirty(s_ui_layer);
}

// ═══════════════════════════════════════════
//  TICK TIMER - once per minute for stat decay
// ═══════════════════════════════════════════

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    pet_state_tick(&s_pet, tick_time);
    redraw_all();
}

// ═══════════════════════════════════════════
//  ANIMATION TIMER - 2fps (500ms)
// ═══════════════════════════════════════════

static void anim_timer_callback(void *data) {
    s_frame_counter++;
    pet_state_anim_tick(&s_pet);
    pet_render_advance_frame(&s_pet, s_frame_counter);
    layer_mark_dirty(s_pet_layer);

    // Flash timer for action feedback
    if (s_ui.flash_timer > 0) {
        s_ui.flash_timer--;
        layer_mark_dirty(s_ui_layer);
    }

    schedule_anim_timer();
}

static void schedule_anim_timer(void) {
    s_anim_timer = app_timer_register(500, anim_timer_callback, NULL);
}

// ═══════════════════════════════════════════
//  LAYER UPDATE PROCS
// ═══════════════════════════════════════════

static void pet_layer_update(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    pet_render_draw(ctx, bounds, &s_pet, s_frame_counter);
}

static void ui_layer_update(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    ui_draw(ctx, bounds, &s_pet, &s_ui);
}

// ═══════════════════════════════════════════
//  MENU ACTION EXECUTOR
// ═══════════════════════════════════════════

static void execute_menu_action(MenuItem item) {
    switch (item) {
        case MENU_FEED:
            pet_state_feed(&s_pet);
            break;
        case MENU_PLAY:
            pet_state_play(&s_pet);
            break;
        case MENU_HEAL:
            pet_state_heal(&s_pet);
            break;
        case MENU_STATS:
            // Mode switch handled in ui_menu_select
            break;
        default:
            break;
    }
    redraw_all();
}

// ═══════════════════════════════════════════
//  BUTTON HANDLERS
// ═══════════════════════════════════════════

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    switch (s_ui.mode) {
        case UI_MODE_STATUS:
            if (!s_pet.is_alive) {
                // Dead: SELECT revives
                pet_state_feed(&s_pet);
            } else {
                ui_menu_open(&s_ui);
            }
            break;
        case UI_MODE_MENU: {
            MenuItem sel = ui_menu_select(&s_ui);
            execute_menu_action(sel);
            break;
        }
        case UI_MODE_STATS:
            ui_menu_close(&s_ui);
            break;
    }
    redraw_all();
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    switch (s_ui.mode) {
        case UI_MODE_MENU:
            ui_menu_up(&s_ui);
            break;
        case UI_MODE_STATS:
            ui_menu_close(&s_ui);
            break;
        default:
            break;
    }
    redraw_all();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    switch (s_ui.mode) {
        case UI_MODE_MENU:
            ui_menu_down(&s_ui);
            break;
        case UI_MODE_STATS:
            ui_menu_close(&s_ui);
            break;
        default:
            break;
    }
    redraw_all();
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
    switch (s_ui.mode) {
        case UI_MODE_MENU:
            ui_menu_close(&s_ui);
            redraw_all();
            break;
        case UI_MODE_STATS:
            ui_menu_close(&s_ui);
            redraw_all();
            break;
        default:
            // Default BACK behavior: exit app
            window_stack_pop(true);
            break;
    }
}

static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
    window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
}

// ═══════════════════════════════════════════
//  WINDOW LOAD / UNLOAD
// ═══════════════════════════════════════════

static void main_window_load(Window *window) {
    Layer *root = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(root);

    window_set_background_color(window, GColorBlack);

    // Pet layer: upper 120px (48 rows * 3px pitch = 144x120)
    s_pet_layer = layer_create(GRect(0, 0, bounds.size.w, 120));
    layer_set_update_proc(s_pet_layer, pet_layer_update);
    layer_add_child(root, s_pet_layer);

    // UI layer: bottom 48px
    s_ui_layer = layer_create(GRect(0, 120, bounds.size.w, 48));
    layer_set_update_proc(s_ui_layer, ui_layer_update);
    layer_add_child(root, s_ui_layer);

    pet_render_init();
}

static void main_window_unload(Window *window) {
    pet_render_deinit();
    layer_destroy(s_pet_layer);
    layer_destroy(s_ui_layer);
}

// ═══════════════════════════════════════════
//  APP LIFECYCLE
// ═══════════════════════════════════════════

static void init(void) {
    pet_state_load(&s_pet);

    // Check sleep state on launch
    pet_state_check_sleep(&s_pet);

    ui_init(&s_ui);
    s_frame_counter = 0;

    s_main_window = window_create();
    window_set_click_config_provider(s_main_window, click_config_provider);
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload,
    });
    window_stack_push(s_main_window, true);

    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    schedule_anim_timer();
}

static void deinit(void) {
    pet_state_save(&s_pet);
    tick_timer_service_unsubscribe();
    if (s_anim_timer) {
        app_timer_cancel(s_anim_timer);
    }
    window_destroy(s_main_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}