/**
 * @file PongEngine.h
 * @brief Signal Thief PVE arena engine
 *
 * The public function names are kept for compatibility with the original
 * Pong template and menu integration code.
 */

#ifndef PONGENGINE_H
#define PONGENGINE_H

#include <stdint.h>
#include "Utils.h"

#define SIGNAL_MAX_ENEMIES 5
#define SIGNAL_MAX_PROJECTILES 16
#define SIGNAL_MAX_BOSS_SHOTS 6
#define SIGNAL_MAX_PARTICLES 14
#define SIGNAL_MAX_TRAPS 3

typedef struct {
    int16_t x;
    int16_t y;
    int16_t dx;
    int16_t dy;
    uint8_t damage;
    uint8_t size;
    uint8_t active;
} SignalProjectile_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t speed;
    uint8_t active;
    uint8_t elite;
    uint8_t type;
    uint8_t hp;
    uint8_t flash_frames;
    uint16_t fire_cooldown_frames;
} SignalEnemy_t;

typedef struct {
    int16_t x;
    int16_t y;
    int8_t dx;
    int8_t dy;
    uint8_t life;
    uint8_t colour;
} SignalParticle_t;

typedef struct {
    int16_t x;
    int16_t y;
    uint8_t active;
    uint8_t armed_frames;
    uint8_t pulse_frames;
} SignalTrap_t;

typedef struct {
    int16_t player_x;
    int16_t player_y;
    int16_t player_size;
    int16_t player_speed;

    SignalEnemy_t enemies[SIGNAL_MAX_ENEMIES];
    SignalProjectile_t projectiles[SIGNAL_MAX_PROJECTILES];
    SignalProjectile_t boss_shots[SIGNAL_MAX_BOSS_SHOTS];
    SignalParticle_t particles[SIGNAL_MAX_PARTICLES];
    SignalTrap_t traps[SIGNAL_MAX_TRAPS];

    int16_t boss_x;
    int16_t boss_y;
    uint8_t boss_active;
    uint8_t boss_spawned;
    uint8_t boss_tier;
    uint8_t boss_hp;
    uint8_t boss_max_hp;
    uint8_t boss_flash_frames;
    uint8_t boss_move_frames;
    uint16_t boss_fire_cooldown_frames;
    uint8_t boss_title_frames;
    uint8_t boss_warning_frames;
    uint8_t boss_pending_tier;
    uint8_t boss_phase;
    uint8_t boss_phase_title_frames;
    uint8_t boss_charge_frames;
    uint8_t boss_charge_cooldown_frames;
    int8_t boss_charge_dx;
    int8_t boss_charge_dy;
    uint8_t boss_summon_cooldown_frames;

    uint8_t lives;
    uint8_t max_lives;
    uint16_t score;
    uint16_t time_left_s;
    uint8_t wave;
    uint8_t first_boss_defeated;
    uint8_t elite_kills;
    uint8_t elite_boss_spawned;
    uint8_t signal_x;
    uint8_t signal_y;
    uint8_t heal_active;
    uint8_t heal_x;
    uint8_t heal_y;
    uint8_t chest_active;
    int16_t chest_x;
    int16_t chest_y;
    uint8_t victory_animation_frames;
    uint8_t victory_complete;
    uint8_t signals_collected;
    uint8_t total_signals_collected;
    uint8_t weapon_level;
    uint8_t fire_rate_level;
    uint8_t bullet_size_level;
    uint8_t vitality_level;
    uint8_t upgrade_choice_active;
    uint8_t upgrade_choice_index;
    uint8_t upgrade_choice_message_frames;
    uint8_t ultimate_charges;
    uint8_t signal_decode_active;
    Direction decode_sequence[3];
    uint8_t decode_index;
    Direction last_decode_dir;
    Direction last_input_dir;
    uint8_t upgrade_message_frames;
    uint8_t ultimate_ready_message_frames;
    uint8_t ultimate_flash_frames;
    uint8_t ultimate_freeze_frames;
    uint8_t dash_frames;
    uint8_t dash_cooldown_frames;
    uint8_t dash_evade_message_frames;
    Direction dash_dir;
    Direction aim_dir;
    uint8_t aim_indicator_frames;
    uint8_t charge_frames;
    Direction charge_dir;
    uint8_t trap_cooldown_frames;
    uint8_t trap_message_frames;
    uint8_t combo_kills;
    uint8_t combo_multiplier;
    uint8_t combo_message_frames;
    uint16_t combo_timer_frames;
    uint8_t melee_frames;
    uint8_t melee_cooldown_frames;
    Direction melee_dir;
    uint16_t fire_cooldown_frames;
    uint16_t spawn_cooldown_frames;
    uint16_t invulnerable_frames;
    uint8_t hit_effect_frames;
} PongEngine_t;

void PongEngine_Init(PongEngine_t* engine,
                     int16_t paddle_x, int16_t paddle_y,
                     int16_t paddle_width, int16_t paddle_height,
                     int16_t ball_size, float ball_speed);

uint8_t PongEngine_Update(PongEngine_t* engine, UserInput input);

uint8_t PongEngine_UpdateDual(PongEngine_t* engine,
                              UserInput move_input,
                              UserInput aim_input,
                              uint8_t dash_pressed);

void PongEngine_UseUltimate(PongEngine_t* engine);

void PongEngine_HandleSecondStickButton(PongEngine_t* engine,
                                        UserInput aim_input,
                                        uint8_t button_down,
                                        uint8_t button_pressed);

void PongEngine_MeleeAttack(PongEngine_t* engine);

void PongEngine_Draw(PongEngine_t* engine);

uint8_t PongEngine_GetLives(PongEngine_t* engine);

uint16_t PongEngine_GetScore(PongEngine_t* engine);

uint8_t PongEngine_IsVictoryComplete(PongEngine_t* engine);

#endif // PONGENGINE_H
