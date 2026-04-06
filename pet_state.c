/**
 * pet_render.h - Dot-matrix display renderer
 *
 * Renders the pet sprite grid in the upper 120px of the screen.
 * UI bar handles all text/menus in the lower 48px.
 */

#pragma once
#include <pebble.h>
#include "pet_state.h"

void pet_render_init(void);
void pet_render_deinit(void);
void pet_render_draw(GContext *ctx, GRect bounds,
                     const PetState *pet, uint32_t frame);
void pet_render_advance_frame(const PetState *pet, uint32_t frame);