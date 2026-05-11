#ifndef STRIKER_ENGINE_H
#define STRIKER_ENGINE_H

#include <stdint.h>
#include "Joystick.h"
#include "SpaceShipTypes.h"

#define STRIKER_MAX_BULLETS 14
#define STRIKER_MAX_ENEMY_BULLETS 40
#define STRIKER_MAX_ENEMIES 10
#define STRIKER_MAX_POWERUPS 4
#define STRIKER_MAX_BURSTS 6
#define STRIKER_STAR_COUNT 28

typedef enum {
    STRIKER_EVENT_NONE = 0,
    STRIKER_EVENT_FIRE = 1 << 0,
    STRIKER_EVENT_ENEMY_HIT = 1 << 1,
    STRIKER_EVENT_PLAYER_HIT = 1 << 2,
    STRIKER_EVENT_POWERUP = 1 << 3,
    STRIKER_EVENT_SHIELD_BLOCK = 1 << 4,
    STRIKER_EVENT_WAVE = 1 << 5
} StrikerEventFlags;

typedef enum {
    STRIKER_BULLET_LINEAR = 0,
    STRIKER_BULLET_SPREAD,
    STRIKER_BULLET_PIERCE
} StrikerBulletType_t;

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    uint8_t damage;
    uint8_t pierce_remaining;
    uint8_t type;
    uint8_t active;
} StrikerBullet_t;

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    uint8_t type;
    uint8_t active;
} StrikerEnemyBullet_t;

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    uint16_t fire_timer;
    uint8_t hp;
    uint8_t type;
    uint8_t active;
} StrikerEnemy_t;

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    uint16_t ttl;
    uint8_t type;
    uint8_t active;
} StrikerPowerup_t;

typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t speed;
    uint8_t colour;
} StrikerStar_t;

typedef struct {
    float x;
    float y;
    uint8_t tick;
    uint8_t frame;
    uint8_t active;
} StrikerBurst_t;

typedef struct {
    float player_x;
    float player_y;
    uint16_t score;
    uint8_t hp;
    uint8_t max_hp;
    uint8_t lives;
    uint8_t wave;
    uint8_t game_over;
    uint8_t fire_cooldown;
    uint8_t shield_active;
    uint8_t ship_type;
    uint16_t weapon_timer;
    uint16_t magnet_timer;
    uint16_t spawn_timer;
    uint16_t frame_counter;
    uint32_t survival_frames;
    uint16_t enemy_kills;
    uint16_t boss_kills;
    uint8_t boss_active;
    float boss_x;
    float boss_y;
    uint16_t boss_hp;
    uint16_t boss_max_hp;
    uint16_t boss_timer;
    uint16_t boss_attack_timer;
    uint8_t boss_attack_phase;
    uint16_t laser_warning_timer;
    uint16_t laser_fire_timer;
    uint8_t laser_hit_done;
    float laser_x;
    uint8_t mission_complete;
    uint8_t signal_core;
    uint8_t escape_progress;
    uint8_t final_boss_spawned;

    StrikerBullet_t bullets[STRIKER_MAX_BULLETS];
    StrikerEnemyBullet_t enemy_bullets[STRIKER_MAX_ENEMY_BULLETS];
    StrikerEnemy_t enemies[STRIKER_MAX_ENEMIES];
    StrikerPowerup_t powerups[STRIKER_MAX_POWERUPS];
    StrikerBurst_t bursts[STRIKER_MAX_BURSTS];
    StrikerStar_t stars[STRIKER_STAR_COUNT];
} StrikerEngine_t;

void StrikerEngine_Init(StrikerEngine_t* engine);
void StrikerEngine_StartGame(StrikerEngine_t* engine);
void StrikerEngine_SetShipType(StrikerEngine_t* engine, SpaceShipType_t type);
SpaceShipType_t StrikerEngine_GetShipType(StrikerEngine_t* engine);
uint8_t StrikerEngine_Update(StrikerEngine_t* engine, UserInput input, uint8_t fire_pressed);
void StrikerEngine_Draw(StrikerEngine_t* engine);

uint8_t StrikerEngine_IsGameOver(StrikerEngine_t* engine);
uint16_t StrikerEngine_GetScore(StrikerEngine_t* engine);
uint8_t StrikerEngine_GetHp(StrikerEngine_t* engine);
uint8_t StrikerEngine_GetMaxHp(StrikerEngine_t* engine);
uint8_t StrikerEngine_GetLives(StrikerEngine_t* engine);
uint8_t StrikerEngine_GetWave(StrikerEngine_t* engine);
uint8_t StrikerEngine_HasShield(StrikerEngine_t* engine);
uint16_t StrikerEngine_GetWeaponTimer(StrikerEngine_t* engine);
uint16_t StrikerEngine_GetMagnetTimer(StrikerEngine_t* engine);
uint16_t StrikerEngine_GetSurvivalSeconds(StrikerEngine_t* engine);
uint16_t StrikerEngine_GetEnemyKills(StrikerEngine_t* engine);
uint16_t StrikerEngine_GetBossKills(StrikerEngine_t* engine);
uint8_t StrikerEngine_GetSignalCore(StrikerEngine_t* engine);
uint8_t StrikerEngine_GetEscapeProgress(StrikerEngine_t* engine);
uint8_t StrikerEngine_IsMissionComplete(StrikerEngine_t* engine);

#endif
