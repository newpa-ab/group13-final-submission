#ifndef VERSUS_ENGINE_H
#define VERSUS_ENGINE_H

#include <stdint.h>
#include "Joystick.h"
#include "SpaceShipTypes.h"

#define VERSUS_MAX_BULLETS 18
#define VERSUS_MAX_POWERUPS 3

typedef enum {
    VERSUS_EVENT_NONE = 0,
    VERSUS_EVENT_P1_FIRE = 1 << 0,
    VERSUS_EVENT_P2_FIRE = 1 << 1,
    VERSUS_EVENT_HIT = 1 << 2,
    VERSUS_EVENT_POWERUP = 1 << 3,
    VERSUS_EVENT_SHIELD_BLOCK = 1 << 4,
    VERSUS_EVENT_DASH = 1 << 5
} VersusEventFlags;

typedef enum {
    VERSUS_WINNER_NONE = 0,
    VERSUS_WINNER_P1,
    VERSUS_WINNER_P2
} VersusWinner_t;

typedef struct {
    float x;
    float y;
    float angle;
    uint8_t hp;
    uint8_t shield;
    uint16_t double_timer;
    uint16_t dash_cooldown;
    uint16_t hits;
    uint8_t hit_flash;
    uint8_t ship;
} VersusPlayer_t;

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    uint8_t owner;
    uint8_t damage;
    uint8_t type;
    uint8_t ttl;
    uint8_t active;
} VersusBullet_t;

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    uint8_t type;
    uint16_t ttl;
    uint8_t active;
} VersusPowerup_t;

typedef struct {
    VersusPlayer_t p1;
    VersusPlayer_t p2;
    VersusBullet_t bullets[VERSUS_MAX_BULLETS];
    VersusPowerup_t powerups[VERSUS_MAX_POWERUPS];
    uint16_t frame_counter;
    uint16_t powerup_timer;
    uint8_t powerup_flash;
    uint8_t game_over;
    uint8_t winner;
} VersusEngine_t;

void VersusEngine_Init(VersusEngine_t* engine);
void VersusEngine_StartGame(VersusEngine_t* engine);
void VersusEngine_SetShips(VersusEngine_t* engine, SpaceShipType_t p1_ship, SpaceShipType_t p2_ship);
uint8_t VersusEngine_Update(VersusEngine_t* engine,
                            UserInput p1_input,
                            UserInput p2_input,
                            uint8_t p1_fire,
                            uint8_t p2_fire,
                            uint8_t p1_dash,
                            uint8_t p2_dash);
void VersusEngine_Draw(VersusEngine_t* engine);

uint8_t VersusEngine_IsGameOver(VersusEngine_t* engine);
VersusWinner_t VersusEngine_GetWinner(VersusEngine_t* engine);
uint8_t VersusEngine_GetP1Hp(VersusEngine_t* engine);
uint8_t VersusEngine_GetP2Hp(VersusEngine_t* engine);
uint16_t VersusEngine_GetP1Hits(VersusEngine_t* engine);
uint16_t VersusEngine_GetP2Hits(VersusEngine_t* engine);

#endif
