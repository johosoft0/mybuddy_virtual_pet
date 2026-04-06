/**
 * pet_state.c - Pet state logic implementation
 *
 * Handles stat decay over time, player actions, persistence,
 * and mood derivation. All stats clamped to 0-100.
 *
 * IMPORTANT: anim_ticks counts down in ANIMATION timer units (250ms),
 * NOT minute units. pet_state_anim_tick() must be called from the
 * animation timer in main.c. pet_state_tick() is for minute-level
 * stat decay only.
 */

#include "pet_state.h"

// ═══════════════════════════════════════════
//  HELPERS
// ═══════════════════════════════════════════

int16_t clamp_stat(int16_t val) {
    if (val < STAT_MIN) return STAT_MIN;
    if (val > STAT_MAX) return STAT_MAX;
    return val;
}

// ═══════════════════════════════════════════
//  INIT
// ═══════════════════════════════════════════

void pet_state_init(PetState *pet) {
    pet->hunger     = STAT_START;
    pet->happiness  = STAT_START;
    pet->health     = STAT_START;
    pet->age_days   = 0;
    pet->last_tick  = time(NULL);
    pet->mood       = MOOD_HAPPY;
    pet->anim       = ANIM_IDLE;
    pet->anim_ticks = 0;
    pet->is_alive   = 1;
    pet->idle_counter = 0;
    pet->is_sleeping  = 0;
}

// ═══════════════════════════════════════════
//  PERSISTENCE
// ═══════════════════════════════════════════

void pet_state_save(const PetState *pet) {
    persist_write_int(PERSIST_KEY_VERSION, PERSIST_SAVE_VERSION);
    persist_write_data(PERSIST_KEY_PET_DATA, pet, sizeof(PetState));
}

void pet_state_load(PetState *pet) {
    if (persist_exists(PERSIST_KEY_VERSION) &&
        persist_read_int(PERSIST_KEY_VERSION) == PERSIST_SAVE_VERSION &&
        persist_exists(PERSIST_KEY_PET_DATA)) {

        persist_read_data(PERSIST_KEY_PET_DATA, pet, sizeof(PetState));

        // Apply offline decay: figure out how many minutes passed
        time_t now = time(NULL);
        int elapsed_minutes = (int)(now - pet->last_tick) / 60;
        if (elapsed_minutes > 0 && pet->is_alive) {
            // Cap at 6 hours to prevent instant death
            if (elapsed_minutes > 360) elapsed_minutes = 360;

            pet->hunger    = clamp_stat(pet->hunger - (elapsed_minutes * HUNGER_DECAY));
            pet->happiness = clamp_stat(pet->happiness - (elapsed_minutes * HAPPY_DECAY));

            // Health decays if hunger or happiness were critically low
            if (pet->hunger < 20 || pet->happiness < 20) {
                int health_loss = elapsed_minutes / 3;
                pet->health = clamp_stat(pet->health - health_loss);
            }

            pet->last_tick = now;
        }

        // Clear any stale animation state from before save
        pet->anim = ANIM_IDLE;
        pet->anim_ticks = 0;
        pet->idle_counter = 0;

        // Check death
        if (pet->health <= 0) {
            pet->is_alive = 0;
            pet->anim = ANIM_DEAD;
        }

        pet->mood = pet_state_calc_mood(pet);
    } else {
        // No save data or wrong version — new pet
        pet_state_init(pet);
    }
}

// ═══════════════════════════════════════════
//  TICK (called every MINUTE for stat decay)
// ═══════════════════════════════════════════

void pet_state_tick(PetState *pet, struct tm *tick_time) {
    if (!pet->is_alive) return;

    pet->last_tick = time(NULL);

    // Check day/night sleep cycle
    pet_state_check_sleep(pet);

    // Decay stats (halved while sleeping)
    if (pet->is_sleeping) {
        // Sleeping: decay every other minute only
        if (tick_time->tm_min % 2 == 0) {
            pet->hunger = clamp_stat(pet->hunger - HUNGER_DECAY);
        }
        // Happiness doesn't decay during sleep
        // Health recovers during sleep
        if (pet->health < STAT_MAX) {
            pet->health = clamp_stat(pet->health + 1);
        }
    } else {
        // Awake: normal decay
        pet->hunger    = clamp_stat(pet->hunger - HUNGER_DECAY);
        pet->happiness = clamp_stat(pet->happiness - HAPPY_DECAY);

        // Health decays when hungry or sad
        if (pet->hunger < 20 || pet->happiness < 20) {
            pet->health = clamp_stat(pet->health - 1);
        }
        // Health slowly recovers when well-fed and happy
        if (pet->hunger > 70 && pet->happiness > 70 && pet->health < STAT_MAX) {
            pet->health = clamp_stat(pet->health + 1);
        }
    }

    // Track age: increment on midnight
    if (tick_time->tm_hour == 0 && tick_time->tm_min == 0) {
        pet->age_days++;
    }

    // Check death
    if (pet->health <= 0) {
        pet->is_alive = 0;
        pet->anim = ANIM_DEAD;
        pet->mood = MOOD_DEAD;
        return;
    }

    pet->mood = pet_state_calc_mood(pet);
}

// ═══════════════════════════════════════════
//  ANIM TICK (called every 250ms from anim timer)
// ═══════════════════════════════════════════

// Run cycle: every ~15 seconds of idle, run for ~3 seconds
// 30 ticks idle @ 500ms = 15s, then 6 ticks running = 3s
#define IDLE_WALK_THRESHOLD  30
#define WALK_DURATION        6

void pet_state_anim_tick(PetState *pet) {
    if (!pet->is_alive) return;

    // Countdown temporary animations (eating, playing, healing, walking)
    if (pet->anim_ticks > 0) {
        pet->anim_ticks--;
        if (pet->anim_ticks == 0) {
            pet->anim = ANIM_IDLE;
            pet->idle_counter = 0;  // reset idle timer after any action
        }
        return;
    }

    // Idle run cycle: only when awake and truly idle
    if (pet->anim == ANIM_IDLE && !pet->is_sleeping) {
        pet->idle_counter++;
        if (pet->idle_counter >= IDLE_WALK_THRESHOLD) {
            pet->anim = ANIM_RUNNING;
            pet->anim_ticks = WALK_DURATION;
            pet->idle_counter = 0;
        }
    }
}

// ═══════════════════════════════════════════
//  MOOD CALCULATION
// ═══════════════════════════════════════════

PetMood pet_state_calc_mood(const PetState *pet) {
    if (!pet->is_alive)       return MOOD_DEAD;
    if (pet->health < 30)     return MOOD_SICK;
    if (pet->hunger < 30)     return MOOD_HUNGRY;
    if (pet->happiness < 30)  return MOOD_SAD;
    if (pet->hunger > 60 && pet->happiness > 60 && pet->health > 60)
        return MOOD_HAPPY;
    return MOOD_CONTENT;
}

// ═══════════════════════════════════════════
//  DAY/NIGHT SLEEP CYCLE
// ═══════════════════════════════════════════

void pet_state_check_sleep(PetState *pet) {
    if (!pet->is_alive) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    int hour = t->tm_hour;

    // Sleep between 10PM and 7AM
    bool should_sleep = (hour >= SLEEP_HOUR_START || hour < SLEEP_HOUR_END);

    if (should_sleep && !pet->is_sleeping) {
        // Falling asleep
        pet->is_sleeping = 1;
        pet->anim = ANIM_SLEEPING;
        pet->anim_ticks = 0;   // permanent until wake
        pet->idle_counter = 0;
    } else if (!should_sleep && pet->is_sleeping) {
        // Waking up
        pet->is_sleeping = 0;
        pet->anim = ANIM_IDLE;
        pet->anim_ticks = 0;
        pet->idle_counter = 0;
    }
}

// ═══════════════════════════════════════════
//  PLAYER ACTIONS
// ═══════════════════════════════════════════

void pet_state_feed(PetState *pet) {
    if (!pet->is_alive) {
        pet_state_init(pet);
        pet->hunger    = 50;
        pet->happiness = 30;
        pet->health    = 30;
        return;
    }

    // Waking pet to feed — grumpy
    if (pet->is_sleeping) {
        pet->is_sleeping = 0;
        pet->happiness = clamp_stat(pet->happiness - 5);
    }

    pet->hunger = clamp_stat(pet->hunger + FEED_AMOUNT);

    // Overfeeding makes pet unhappy
    if (pet->hunger > 95) {
        pet->happiness = clamp_stat(pet->happiness - 5);
    }

    pet->anim = ANIM_EATING;
    pet->anim_ticks = 4;   // 4 x 500ms = 2 seconds
    pet->mood = pet_state_calc_mood(pet);
}

void pet_state_play(PetState *pet) {
    if (!pet->is_alive) return;

    if (pet->is_sleeping) {
        pet->is_sleeping = 0;
        pet->happiness = clamp_stat(pet->happiness - 5);
    }

    pet->happiness = clamp_stat(pet->happiness + PLAY_AMOUNT);

    // Playing when hungry costs extra hunger
    pet->hunger = clamp_stat(pet->hunger - 5);

    pet->anim = ANIM_PLAYING;
    pet->anim_ticks = 6;   // 6 x 500ms = 3 seconds
    pet->mood = pet_state_calc_mood(pet);
}

void pet_state_heal(PetState *pet) {
    if (!pet->is_alive) return;

    pet->health = clamp_stat(pet->health + HEAL_AMOUNT);

    // Medicine is not fun
    pet->happiness = clamp_stat(pet->happiness - 10);

    pet->anim = ANIM_SLEEPING;
    pet->anim_ticks = 5;   // 5 x 500ms = 2.5 seconds
    pet->mood = pet_state_calc_mood(pet);
}