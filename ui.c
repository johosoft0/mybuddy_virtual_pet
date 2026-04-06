/**
 * sprites.h - Dot-matrix sprite definitions (48x40, 4 frames)
 *
 * Each sprite is a 48-wide x 40-tall grid of color indices.
 * At 3px pitch (2px dot + 1px gap), this fills 144x120 of
 * the 144x130 pet display area.
 *
 * Color palette indices:
 *   0 = transparent (dim grid dot)
 *   1 = dark gray   (body outline)
 *   2 = gray        (body mid-tone)
 *   3 = light gray  (body highlights)
 *   4 = white       (belly, face stripe, chest)
 *   5 = pink        (ears, paws, nose, inner ear)
 *   6 = black       (eyes, eye outline)
 *   7 = brown/tan   (acorn, food props)
 *
 * 5 animations x 4 unique frames = 20 frames total
 * Storage: 48*40*20 = 38,400 bytes in const flash. Zero heap.
 */

#pragma once
#include <pebble.h>

#define SPRITE_COLS       48
#define SPRITE_ROWS       40
#define FRAMES_PER_ANIM   4
#define NUM_ANIMS         5
#define PAL_SIZE          8

#define ANIM_IDX_EAT      0
#define ANIM_IDX_RUN      1
#define ANIM_IDX_PLAY     2
#define ANIM_IDX_IDLE     3
#define ANIM_IDX_SLEEP    4

// Sprite data: [anim][frame][row][col]
// Defined across multiple .c files for editability
extern const uint8_t sprite_data[NUM_ANIMS][FRAMES_PER_ANIM][SPRITE_ROWS][SPRITE_COLS];
