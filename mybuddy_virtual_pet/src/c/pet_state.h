/**
 * pet_state.h - Pet state data structures and manipulation
 *
 * All pet stats are 0-100 integers to keep math simple and RAM tiny.
 * Persistent storage keys are defined here for save/load.
 */

#pragma once
#include <pebble.h>

// ── Persistent Storage Keys ──
#define PERSIST_KEY_PET_DATA  0x0001
#define PERSIST_KEY_VERSION   0x0002
#define PERSIST_SAVE_VERSION  2

// ── Stat Bounds ──
#define STAT_MIN   0
#define STAT_MAX   100
#define STAT_START 70

// ── Decay Rates (per minute) ──
#define HUNGER_DECAY   1   // loses 1 hunger per minute (~1.5hr to starve)
#define HAPPY_DECAY    1
#define HEALTH_DECAY   0   // only decays when hungry/unhappy

// ── Action Amounts ──
#define FEED_AMOUNT    25
#define PLAY_AMOUNT    20
#define HEAL_AMOUNT    30

// ── Pet Mood (derived from stats, not persisted directly) ──
typedef enum {
    MOOD_HAPPY,       // all stats above 60
    MOOD_CONTENT,     // all stats above 30
    MOOD_HUNGRY,      // hunger below 30
    MOOD_SAD,         // happiness below 30
    MOOD_SICK,        // health below 30
    MOOD_DEAD,        // any stat hits 0 and stayed there
} PetMood;

// ── Animation State ──
typedef enum {
    ANIM_IDLE,
    ANIM_EATING,
    ANIM_PLAYING,
    ANIM_RUNNING,      // periodic run cycle from idle
    ANIM_SLEEPING,
    ANIM_SICK,
    ANIM_DEAD,
} PetAnim;

// ── Sleep Schedule (real clock hours) ──
#define SLEEP_HOUR_START  22   // 10 PM — pet falls asleep
#define SLEEP_HOUR_END     7   // 7 AM  — pet wakes up

// ── Core Pet State ──
// NOTE: This struct is written raw to persistent storage.
// Use fixed-width types only. Enums stored as uint8_t.
typedef struct {
    int16_t  hunger;       // 0 = starving, 100 = full
    int16_t  happiness;    // 0 = miserable, 100 = ecstatic
    int16_t  health;       // 0 = dead, 100 = perfect
    uint16_t age_days;     // days alive
    time_t   last_tick;    // timestamp of last stat update
    uint8_t  mood;         // PetMood value
    uint8_t  anim;         // PetAnim value
    uint8_t  anim_ticks;   // countdown for temporary animations (anim-timer units)
    uint8_t  is_alive;     // 1 = alive, 0 = dead
    uint8_t  idle_counter; // counts anim ticks in idle, triggers run
    uint8_t  is_sleeping;  // 1 = asleep (night time), 0 = awake
    uint8_t  _pad[2];      // alignment padding
} PetState;

// ── API ──

// Initialize a fresh pet
void pet_state_init(PetState *pet);

// Save/load to persistent storage
void pet_state_save(const PetState *pet);
void pet_state_load(PetState *pet);

// Called every minute by tick_handler (stat decay)
void pet_state_tick(PetState *pet, struct tm *tick_time);

// Called every 250ms by animation timer (anim countdown)
void pet_state_anim_tick(PetState *pet);

// Player actions
void pet_state_feed(PetState *pet);
void pet_state_play(PetState *pet);
void pet_state_heal(PetState *pet);

// Derived
PetMood pet_state_calc_mood(const PetState *pet);

// Check if current hour is sleep time and update state
void pet_state_check_sleep(PetState *pet);

// Clamp helper
int16_t clamp_stat(int16_t val);