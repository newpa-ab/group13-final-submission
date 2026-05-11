/**
 * @file AsteroidsEngine.h
 * @brief Asteroids-style game engine for STM32 + LCD.
 */

#ifndef ASTEROIDS_ENGINE_H
#define ASTEROIDS_ENGINE_H

#include <stdint.h>
#include "Joystick.h"
#include "SpaceShipTypes.h"

#define ASTEROIDS_MAX_BULLETS 10
#define ASTEROIDS_MAX_ROCKS 12
#define ASTEROIDS_MAX_PARTICLES 24
#define ASTEROIDS_MAX_BURSTS 6
#define ASTEROIDS_MAX_POWERUPS 4
#define ASTEROIDS_STAR_COUNT 18

typedef enum {
    AST_EVENT_NONE = 0,
    AST_EVENT_FIRE = 1 << 0,
    AST_EVENT_ROCK_HIT = 1 << 1,
    AST_EVENT_SHIP_HIT = 1 << 2,
    AST_EVENT_WAVE_CLEAR = 1 << 3,
    AST_EVENT_POWERUP = 1 << 4,
    AST_EVENT_SHIELD_BLOCK = 1 << 5
} AsteroidsEventFlags;

typedef enum {
    AST_DIFFICULTY_EASY = 0,
    AST_DIFFICULTY_NORMAL,
    AST_DIFFICULTY_HARD
} AsteroidsDifficulty_t;

typedef enum {
    AST_POWERUP_HEALTH = 0,
    AST_POWERUP_DOUBLE_SHOT,
    AST_POWERUP_SHIELD,
    AST_POWERUP_MAGNET,
    AST_POWERUP_COUNT
} AsteroidsPowerupType_t;

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    uint8_t ttl;
    uint8_t damage;
    uint8_t pierce_remaining;
    uint8_t type;
    uint8_t active;
} AsteroidBullet_t;

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    uint8_t radius;
    uint8_t active;
} AsteroidRock_t;

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    uint8_t ttl;
    uint8_t colour;
    uint8_t active;
} AsteroidParticle_t;

typedef struct {
    uint8_t x;
    uint8_t y;
    uint8_t colour;
} AsteroidStar_t;

typedef struct {
    float x;
    float y;
    uint8_t frame;
    uint8_t tick;
    uint8_t active;
} AsteroidBurst_t;

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    uint16_t ttl;
    uint8_t type;
    uint8_t active;
} AsteroidPowerup_t;

typedef struct {
    float ship_x;
    float ship_y;
    float ship_vx;
    float ship_vy;
    float ship_angle_deg;

    uint16_t score;
    uint8_t lives;
    uint8_t wave;
    uint8_t game_over;
    AsteroidsDifficulty_t difficulty;

    uint8_t fire_cooldown;
    uint8_t invuln_frames;
    uint8_t thrust_active;
    uint8_t shield_active;
    uint8_t ship_type;
    uint16_t weapon_timer;
    uint16_t magnet_timer;
    uint16_t frame_counter;
    uint32_t survival_frames;
    uint16_t rocks_destroyed;
    uint8_t max_wave_reached;

    AsteroidBullet_t bullets[ASTEROIDS_MAX_BULLETS];
    AsteroidRock_t rocks[ASTEROIDS_MAX_ROCKS];
    AsteroidParticle_t particles[ASTEROIDS_MAX_PARTICLES];
    AsteroidBurst_t bursts[ASTEROIDS_MAX_BURSTS];
    AsteroidPowerup_t powerups[ASTEROIDS_MAX_POWERUPS];
    AsteroidStar_t stars[ASTEROIDS_STAR_COUNT];
} AsteroidsEngine_t;

void AsteroidsEngine_Init(AsteroidsEngine_t* engine);
void AsteroidsEngine_StartGame(AsteroidsEngine_t* engine);
void AsteroidsEngine_SetDifficulty(AsteroidsEngine_t* engine, AsteroidsDifficulty_t difficulty);
void AsteroidsEngine_SetShipType(AsteroidsEngine_t* engine, SpaceShipType_t type);
SpaceShipType_t AsteroidsEngine_GetShipType(AsteroidsEngine_t* engine);
uint8_t AsteroidsEngine_Update(AsteroidsEngine_t* engine, UserInput input, uint8_t fire_pressed, uint8_t hyperspace_pressed);
void AsteroidsEngine_Draw(AsteroidsEngine_t* engine);

uint8_t AsteroidsEngine_IsGameOver(AsteroidsEngine_t* engine);
uint16_t AsteroidsEngine_GetScore(AsteroidsEngine_t* engine);
uint8_t AsteroidsEngine_GetLives(AsteroidsEngine_t* engine);
uint8_t AsteroidsEngine_GetWave(AsteroidsEngine_t* engine);
uint8_t AsteroidsEngine_HasShield(AsteroidsEngine_t* engine);
uint16_t AsteroidsEngine_GetWeaponTimer(AsteroidsEngine_t* engine);
uint16_t AsteroidsEngine_GetMagnetTimer(AsteroidsEngine_t* engine);
AsteroidsDifficulty_t AsteroidsEngine_GetDifficulty(AsteroidsEngine_t* engine);
uint16_t AsteroidsEngine_GetSurvivalSeconds(AsteroidsEngine_t* engine);
uint16_t AsteroidsEngine_GetRocksDestroyed(AsteroidsEngine_t* engine);
uint8_t AsteroidsEngine_GetMaxWaveReached(AsteroidsEngine_t* engine);

#endif
