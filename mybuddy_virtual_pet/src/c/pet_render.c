/**
 * pet_render.c - Dot-matrix display renderer (48x40 @ 3px pitch)
 *
 * Renders the pet sprite as a dot-matrix grid in the 144x120 pet area.
 * All text/UI is handled by ui.c in the bottom 48px.
 */

#include "pet_render.h"
#include "sprites.h"
#include <pebble.h>

#define DOT_SIZE    2
#define DOT_PITCH   3
#define GRID_X      0
#define GRID_Y      0

// ═══════════════════════════════════════════
//  COLOR PALETTE
// ═══════════════════════════════════════════

static GColor get_dot_color(uint8_t idx) {
    switch (idx) {
        case 1: return GColorDarkGray;
        case 2: return GColorLightGray;
        case 3: return GColorWhite;
        case 4: return GColorWhite;
        case 5: return GColorMelon;
        case 6: return GColorBlack;
        case 7: return GColorWindsorTan;
        default: return GColorClear;
    }
}

// ═══════════════════════════════════════════
//  ANIM MAPPING
// ═══════════════════════════════════════════

static int anim_to_sprite_index(PetAnim anim) {
    switch (anim) {
        case ANIM_EATING:   return ANIM_IDX_EAT;
        case ANIM_PLAYING:  return ANIM_IDX_PLAY;
        case ANIM_RUNNING:  return ANIM_IDX_RUN;
        case ANIM_IDLE:     return ANIM_IDX_IDLE;
        case ANIM_SLEEPING: return ANIM_IDX_SLEEP;
        case ANIM_SICK:     return ANIM_IDX_SLEEP;
        case ANIM_DEAD:     return ANIM_IDX_SLEEP;
        default:            return ANIM_IDX_IDLE;
    }
}

// ═══════════════════════════════════════════
//  BACKGROUND
// ═══════════════════════════════════════════

static GColor screen_bg_for_mood(PetMood mood) {
    switch (mood) {
        case MOOD_HAPPY:   return GColorOxfordBlue;
        case MOOD_CONTENT: return GColorDarkGreen;
        case MOOD_HUNGRY:  return GColorArmyGreen;
        case MOOD_SAD:     return GColorDarkGray;
        case MOOD_SICK:    return GColorBulgarianRose;
        case MOOD_DEAD:    return GColorBlack;
        default:           return GColorOxfordBlue;
    }
}

// ═══════════════════════════════════════════
//  DOT GRID
// ═══════════════════════════════════════════

static void draw_dot_grid(GContext *ctx, int anim_idx, int frame_idx) {
    const uint8_t (*grid)[SPRITE_COLS] =
        sprite_data[anim_idx][frame_idx];

    for (int row = 0; row < SPRITE_ROWS; row++) {
        int py = GRID_Y + row * DOT_PITCH;
        for (int col = 0; col < SPRITE_COLS; col++) {
            int px = GRID_X + col * DOT_PITCH;
            uint8_t cidx = grid[row][col];

            if (cidx == 0) {
                graphics_context_set_stroke_color(ctx, GColorDarkGray);
                graphics_draw_pixel(ctx, GPoint(px, py));
            } else {
                graphics_context_set_fill_color(ctx, get_dot_color(cidx));
                graphics_fill_rect(ctx,
                    GRect(px, py, DOT_SIZE, DOT_SIZE),
                    0, GCornerNone);
            }
        }
    }
}

// ═══════════════════════════════════════════
//  PUBLIC API
// ═══════════════════════════════════════════

void pet_render_init(void)  { }
void pet_render_deinit(void) { }

void pet_render_advance_frame(const PetState *pet, uint32_t frame) {
    (void)pet; (void)frame;
}

void pet_render_draw(GContext *ctx, GRect bounds,
                     const PetState *pet, uint32_t frame) {
    GColor bg = screen_bg_for_mood(pet->mood);
    graphics_context_set_fill_color(ctx, bg);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);

    int anim_idx = anim_to_sprite_index(pet->anim);
    int frame_idx = (int)(frame % FRAMES_PER_ANIM);

    draw_dot_grid(ctx, anim_idx, frame_idx);
}