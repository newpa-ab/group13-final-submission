/**
 * @file PongEngine.c
 * @brief Signal Thief PVE arena shooter implementation
 */

#include "PongEngine.h"
#include "LCD.h"
#include "Buzzer.h"
#include "AsteroidsAssets.h"

#include <stdio.h>

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240
#define ARENA_TOP 28
#define ARENA_BOTTOM 239
#define PLAYER_SIZE 16
#define ENEMY_SIZE 16
#define BOSS_SIZE 32
#define SHOT_SIZE 4
#define BOSS_SHOT_SIZE 5
#define SIGNAL_SIZE 8
#define HEAL_SIZE 8
#define CHEST_SIZE 16
#define GAME_SECONDS 90
#define MAX_LIVES 6
#define MAX_UPGRADED_LIVES 9
#define MAX_STAT_UPGRADE_LEVEL 3
#define FIRST_BOSS_MAX_HP 36
#define ELITE_BOSS_MAX_HP 72
#define ULTIMATE_SHOT_COUNT 16
#define ULTIMATE_DAMAGE 3
#define BOSS_WARNING_FRAMES 45
#define BOSS_PHASE_TITLE_FRAMES 55
#define BOSS_CHARGE_FRAMES 18
#define BOSS_CHARGE_COOLDOWN_FRAMES 90
#define BOSS_SUMMON_COOLDOWN_FRAMES 115
#define ULTIMATE_FREEZE_FRAMES 10
#define SHADOW_DASH_FRAMES 7
#define SHADOW_DASH_COOLDOWN_FRAMES 50
#define SHADOW_DASH_SPEED 9
#define SHADOW_DASH_INVULNERABLE_FRAMES 18
#define SHADOW_DASH_MIN_MAGNITUDE 0.72f
#define SHADOW_DASH_EVADE_FRAMES 28
#define VICTORY_ANIMATION_FRAMES 250
#define VICTORY_BOARDING_START_FRAME 138
#define COMBO_TIMER_FRAMES 95
#define AIM_ASSIST_MIN_MAGNITUDE 0.25f
#define AIM_FIRE_COOLDOWN_BONUS 5
#define SIGNAL_TRAP_SIZE 10
#define SIGNAL_TRAP_RADIUS 28
#define SIGNAL_TRAP_DAMAGE 3
#define SIGNAL_TRAP_BOSS_DAMAGE 2
#define SIGNAL_TRAP_COOLDOWN_FRAMES 70
#define SIGNAL_TRAP_LIFETIME_FRAMES 210
#define SIGNAL_TRAP_PULSE_FRAMES 18
#define MELEE_DAMAGE 4
#define MELEE_BOSS_DAMAGE 3
#define MELEE_FRAMES 10
#define MELEE_COOLDOWN_FRAMES 22
#define HIT_INVULNERABLE_FRAMES 78
#define HIT_FLASH_FRAMES 18
#define ENEMY_KNOCKBACK_PX 28

#define BUZZER_HIT_FREQ_HZ 260
#define BUZZER_SHOT_FREQ_HZ 1500
#define BUZZER_KILL_FREQ_HZ 1900
#define BUZZER_SIGNAL_FREQ_HZ 2100
#define BUZZER_UPGRADE_FREQ_HZ 2400
#define BUZZER_BOSS_FREQ_HZ 120
#define BUZZER_DASH_FREQ_HZ 1250
#define BUZZER_VOLUME 32
#define BUZZER_BEEP_MS 35

#define ENEMY_TYPE_CHASER 0
#define ENEMY_TYPE_SNIPER 1
#define ENEMY_TYPE_JAMMER 2

extern Buzzer_cfg_t buzzer_cfg;

static uint32_t buzzer_stop_tick = 0;
static uint32_t game_timer_last_tick = 0;

static void beep(uint32_t freq_hz);
static void clear_enemy_pressure(PongEngine_t* engine);
static void spawn_particles(PongEngine_t* engine, int16_t x, int16_t y, uint8_t colour);
static void direction_to_delta(Direction direction, int16_t speed, int16_t* dx, int16_t* dy);
static void award_enemy_kill(PongEngine_t* engine, uint8_t was_elite);
static void damage_boss(PongEngine_t* engine, uint8_t damage);

static uint8_t player_shot_size(PongEngine_t* engine)
{
    uint8_t size = (uint8_t)(SHOT_SIZE + engine->bullet_size_level);
    if (size > 8) {
        size = 8;
    }
    return size;
}

static const uint8_t player_agent_sprite[256] = {
    255,255,255,255,  0,255,255,255,255,255,255,  0,255,255,255,255,
    255,255,255,  0,  1,  0,255,255,255,255,  0,  1,  0,255,255,255,
    255,255,  0,  1,  1,  1,  0,255,255,  0,  1,  1,  1,  0,255,255,
    255,  0,  1,  1,  1,  1,  1,  0,  0,  1,  1,  1,  1,  1,  0,255,
      0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,
      0,  1,  1,  0,  0,  1,  1,  1,  1,  1,  0,  0,  1,  1,  1,  0,
      0,  1,  1,  1,  0,  1,  1,  0,  0,  1,  0,  1,  1,  1,  1,  0,
    255,  0,  1,  1,  1,  1,  0,  0,  0,  0,  1,  1,  1,  1,  0,255,
    255,255,  0,  1,  1,  1,  1,  0,  0,  1,  1,  1,  1,  0,255,255,
    255,  0,  0,255,  0,  9,  9,  9,  9,  9,  9,  0,255,  0,  0,255,
      0,  0,255,  0,  1,  0,  9,  9,  9,  9,  0,  1,  0,255,  0,  0,
      0,255,255,  0,  1,  0,  9, 14,  9, 14,  0,  1,  0,255,255,  0,
    255,255,255,  0,  0,  9,  9,  9,  9,  9,  9,  0,  0,255,255,255,
    255,255,255,255,  0,  9,  9,  0,  0,  9,  9,  0,255,255,255,255,
    255,255,255,  0,  1,  0,  0,255,255,  0,  0,  1,  0,255,255,255,
    255,255,255,  0,  0,255,255,255,255,255,255,  0,  0,255,255,255
};

static const uint8_t player_agent_hit_sprite[256] = {
    255,255,255,255,  6,255,255,255,255,255,255,  6,255,255,255,255,
    255,255,255,  6,  1,  6,255,255,255,255,  6,  1,  6,255,255,255,
    255,255,  6,  1,  1,  1,  6,255,255,  6,  1,  1,  1,  6,255,255,
    255,  6,  1,  1,  1,  1,  1,  6,  6,  1,  1,  1,  1,  1,  6,255,
      6,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  6,
      6,  1,  1,  2,  2,  1,  1,  1,  1,  1,  2,  2,  1,  1,  1,  6,
      6,  1,  1,  1,  2,  1,  1,  6,  6,  1,  2,  1,  1,  1,  1,  6,
    255,  6,  1,  1,  1,  1,  6,  6,  6,  6,  1,  1,  1,  1,  6,255,
    255,255,  6,  1,  1,  1,  1,  6,  6,  1,  1,  1,  1,  6,255,255,
    255,  6,  6,255,  6,  9,  9,  9,  9,  9,  9,  6,255,  6,  6,255,
      6,  6,255,  6,  1,  6,  9,  9,  9,  9,  6,  1,  6,255,  6,  6,
      6,255,255,  6,  1,  6,  9, 14,  9, 14,  6,  1,  6,255,255,  6,
    255,255,255,  6,  6,  9,  9,  9,  9,  9,  9,  6,  6,255,255,255,
    255,255,255,255,  6,  9,  9,  6,  6,  9,  9,  6,255,255,255,255,
    255,255,255,  6,  1,  6,  6,255,255,  6,  6,  1,  6,255,255,255,
    255,255,255,  6,  6,255,255,255,255,255,255,  6,  6,255,255,255
};

static const uint8_t enemy_sprite[64] = {
    255,255,  2,  2,  2,  2,255,255,
    255,  2,  0,  2,  2,  0,  2,255,
      2,  0,  2,  0,  0,  2,  0,  2,
      2,  2,  0,  2,  2,  0,  2,  2,
      2,  2,  0,  2,  2,  0,  2,  2,
      2,  0,  2,  0,  0,  2,  0,  2,
    255,  2,  0,  2,  2,  0,  2,255,
    255,255,  2,  2,  2,  2,255,255
};

static const uint8_t sniper_sprite[64] = {
    255,255,  2,  2,  2,  2,255,255,
    255,  2,  0,  2,  2,  0,  2,255,
      2,  2,  0,  2,  2,  0,  2,  2,
      2,  2,  2,  0,  0,  2,  2,  2,
      2,  0,  0,  6,  6,  0,  0,  2,
      2,  2,  2,  0,  0,  2,  2,  2,
    255,  2,255,  2,  2,255,  2,255,
    255,255,  2,255,255,  2,255,255
};

static const uint8_t jammer_sprite[64] = {
    255,  5,255,255,255,255,  5,255,
      5,  5,  0,  5,  5,  0,  5,  5,
      5,  0,  5,  0,  0,  5,  0,  5,
    255,  5,  0,  5,  5,  0,  5,255,
    255,  5,  0,  5,  5,  0,  5,255,
      5,  0,  5,  0,  0,  5,  0,  5,
      5,  5,  0,  5,  5,  0,  5,  5,
    255,  5,255,255,255,255,  5,255
};

static const uint8_t boss_sprite[64] = {
      2,255,  2,  2,  2,  2,255,  2,
    255,  2,  0,  2,  2,  0,  2,255,
      2,  0,  2,  0,  0,  2,  0,  2,
      2,  2,  0,  2,  2,  0,  2,  2,
      2,  2,  0,  6,  6,  0,  2,  2,
      2,  0,  2,  0,  0,  2,  0,  2,
    255,  2,  0,  2,  2,  0,  2,255,
      2,255,  2,  2,  2,  2,255,  2
};

static const uint8_t heal_sprite[64] = {
    255,255,255,  3,  3,255,255,255,
    255,255,255,  3,  3,255,255,255,
    255,  3,  3,  3,  3,  3,  3,255,
    255,  3,  3,  1,  1,  3,  3,255,
    255,  3,  3,  1,  1,  3,  3,255,
    255,  3,  3,  3,  3,  3,  3,255,
    255,255,255,  3,  3,255,255,255,
    255,255,255,  3,  3,255,255,255
};

static const uint8_t chest_sprite[64] = {
    255,  6,  6,  6,  6,  6,  6,255,
      6,  1,  1,  1,  1,  1,  1,  6,
      6,  1,  5,  5,  5,  5,  1,  6,
      6,  1,  5,  0,  0,  5,  1,  6,
      6,  1,  5,  0,  0,  5,  1,  6,
      6,  1,  5,  5,  5,  5,  1,  6,
      6,  1,  1,  6,  6,  1,  1,  6,
    255,  6,  6,  6,  6,  6,  6,255
};

static uint8_t rects_overlap(int16_t ax, int16_t ay, int16_t aw, int16_t ah,
                             int16_t bx, int16_t by, int16_t bw, int16_t bh)
{
    return (ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by);
}

static void draw_player_sprite_rotated(uint16_t x, uint16_t y,
                                       const uint8_t* sprite,
                                       uint8_t rotation,
                                       uint8_t scale)
{
    uint8_t rotated[PLAYER_SIZE * PLAYER_SIZE];
    uint8_t row = 0;
    uint8_t col = 0;

    for (row = 0; row < PLAYER_SIZE; row++) {
        for (col = 0; col < PLAYER_SIZE; col++) {
            uint8_t src_row = row;
            uint8_t src_col = col;

            switch (rotation & 3U) {
                case 1:
                    src_row = (uint8_t)(PLAYER_SIZE - 1U - col);
                    src_col = row;
                    break;
                case 2:
                    src_row = (uint8_t)(PLAYER_SIZE - 1U - row);
                    src_col = (uint8_t)(PLAYER_SIZE - 1U - col);
                    break;
                case 3:
                    src_row = col;
                    src_col = (uint8_t)(PLAYER_SIZE - 1U - row);
                    break;
                case 0:
                default:
                    break;
            }

            rotated[row * PLAYER_SIZE + col] = sprite[src_row * PLAYER_SIZE + src_col];
        }
    }

    LCD_Draw_Sprite_Scaled(x, y, PLAYER_SIZE, PLAYER_SIZE, rotated, scale);
}

static void draw_player_sprite_facing(uint16_t x, uint16_t y,
                                      const uint8_t* sprite,
                                      uint8_t face_left)
{
    uint8_t flipped[PLAYER_SIZE * PLAYER_SIZE];
    uint8_t row = 0;
    uint8_t col = 0;

    if (!face_left) {
        LCD_Draw_Sprite(x, y, PLAYER_SIZE, PLAYER_SIZE, sprite);
        return;
    }

    for (row = 0; row < PLAYER_SIZE; row++) {
        for (col = 0; col < PLAYER_SIZE; col++) {
            flipped[row * PLAYER_SIZE + col] =
                sprite[row * PLAYER_SIZE + (PLAYER_SIZE - 1U - col)];
        }
    }

    LCD_Draw_Sprite(x, y, PLAYER_SIZE, PLAYER_SIZE, flipped);
}

static uint16_t clamp_screen_x(int16_t value)
{
    if (value < 0) {
        return 0;
    }
    if (value >= (int16_t)SCREEN_WIDTH) {
        return (uint16_t)(SCREEN_WIDTH - 1);
    }
    return (uint16_t)value;
}

static uint16_t clamp_screen_y(int16_t value)
{
    if (value < (int16_t)ARENA_TOP) {
        return ARENA_TOP;
    }
    if (value >= (int16_t)SCREEN_HEIGHT) {
        return (uint16_t)(SCREEN_HEIGHT - 1);
    }
    return (uint16_t)value;
}

static uint8_t direction_faces_left(Direction direction)
{
    return direction == W || direction == NW || direction == SW;
}

static uint16_t weapon_screen_x(uint16_t x, int16_t rel_x, uint8_t face_left)
{
    int16_t value = face_left ? (int16_t)(x + PLAYER_SIZE) - rel_x
                              : (int16_t)x + rel_x;
    return clamp_screen_x(value);
}

static uint16_t weapon_screen_y(uint16_t y, int16_t rel_y, int16_t y_offset)
{
    return clamp_screen_y((int16_t)y + rel_y + y_offset);
}

static void draw_weapon_line(uint16_t x, uint16_t y,
                             int16_t x0, int16_t y0,
                             int16_t x1, int16_t y1,
                             uint8_t face_left,
                             int16_t y_offset,
                             uint8_t colour)
{
    LCD_Draw_Line(weapon_screen_x(x, x0, face_left), weapon_screen_y(y, y0, y_offset),
                  weapon_screen_x(x, x1, face_left), weapon_screen_y(y, y1, y_offset),
                  colour);
}

static void draw_agent_pistol(uint16_t x, uint16_t y,
                              Direction weapon_dir,
                              uint8_t alert_colour)
{
    uint8_t metal = alert_colour != 0 ? alert_colour : 1;
    uint8_t shadow = 0;
    uint8_t glow = alert_colour != 0 ? 1 : 6;
    uint8_t face_left = direction_faces_left(weapon_dir);
    int16_t unused_x = 0;
    int16_t y_offset = 0;

    if (weapon_dir == CENTRE) {
        weapon_dir = E;
    }
    direction_to_delta(weapon_dir, 3, &unused_x, &y_offset);

    draw_weapon_line(x, y, 10, 7, 22, 7, face_left, y_offset, metal);
    draw_weapon_line(x, y, 10, 8, 23, 8, face_left, y_offset, metal);
    draw_weapon_line(x, y, 12, 9, 21, 9, face_left, y_offset, shadow);
    draw_weapon_line(x, y, 21, 9, 24, 9, face_left, y_offset, metal);
    draw_weapon_line(x, y, 11, 6, 12, 6, face_left, y_offset, metal);
    draw_weapon_line(x, y, 20, 6, 21, 6, face_left, y_offset, metal);
    draw_weapon_line(x, y, 10, 10, 15, 10, face_left, y_offset, metal);
    draw_weapon_line(x, y, 10, 11, 14, 11, face_left, y_offset, metal);
    draw_weapon_line(x, y, 11, 12, 9, 16, face_left, y_offset, metal);
    draw_weapon_line(x, y, 13, 12, 12, 16, face_left, y_offset, metal);
    draw_weapon_line(x, y, 14, 12, 17, 12, face_left, y_offset, shadow);
    draw_weapon_line(x, y, 15, 11, 16, 13, face_left, y_offset, glow);
}

static void draw_agent_ak47(uint16_t x, uint16_t y,
                            Direction weapon_dir,
                            uint8_t alert_colour)
{
    uint8_t metal = alert_colour != 0 ? alert_colour : 1;
    uint8_t wood = alert_colour != 0 ? alert_colour : 5;
    uint8_t glow = alert_colour != 0 ? 1 : 6;
    uint8_t shadow = 0;
    uint8_t face_left = direction_faces_left(weapon_dir);
    int16_t unused_x = 0;
    int16_t y_offset = 0;

    if (weapon_dir == CENTRE) {
        weapon_dir = E;
    }
    direction_to_delta(weapon_dir, 4, &unused_x, &y_offset);

    draw_weapon_line(x, y, 5, 10, 10, 7, face_left, y_offset, wood);
    draw_weapon_line(x, y, 5, 11, 10, 10, face_left, y_offset, wood);
    draw_weapon_line(x, y, 6, 12, 10, 11, face_left, y_offset, wood);
    draw_weapon_line(x, y, 10, 7, 20, 7, face_left, y_offset, metal);
    draw_weapon_line(x, y, 9, 8, 21, 8, face_left, y_offset, metal);
    draw_weapon_line(x, y, 9, 9, 20, 9, face_left, y_offset, shadow);
    draw_weapon_line(x, y, 10, 10, 18, 10, face_left, y_offset, metal);
    draw_weapon_line(x, y, 18, 6, 24, 6, face_left, y_offset, wood);
    draw_weapon_line(x, y, 18, 7, 24, 7, face_left, y_offset, wood);
    draw_weapon_line(x, y, 20, 8, 31, 8, face_left, y_offset, metal);
    draw_weapon_line(x, y, 21, 9, 30, 9, face_left, y_offset, metal);
    draw_weapon_line(x, y, 29, 7, 29, 10, face_left, y_offset, metal);
    draw_weapon_line(x, y, 31, 8, 34, 8, face_left, y_offset, glow);
    draw_weapon_line(x, y, 13, 11, 16, 18, face_left, y_offset, metal);
    draw_weapon_line(x, y, 14, 11, 18, 17, face_left, y_offset, metal);
    draw_weapon_line(x, y, 17, 17, 19, 17, face_left, y_offset, metal);
    draw_weapon_line(x, y, 10, 11, 8, 15, face_left, y_offset, wood);
    draw_weapon_line(x, y, 11, 11, 10, 15, face_left, y_offset, wood);
}

static void draw_agent_gear(uint16_t x, uint16_t y,
                            uint8_t weapon_level,
                            Direction weapon_dir,
                            uint8_t alert_colour)
{
    uint16_t cx = (uint16_t)(x + PLAYER_SIZE / 2);
    uint16_t cy = (uint16_t)(y + PLAYER_SIZE / 2);
    uint8_t scarf_colour = alert_colour != 0 ? alert_colour : 6;

    LCD_Draw_Rect((uint16_t)(cx > 3 ? cx - 3 : cx), (uint16_t)(cy + 3), 6, 2, scarf_colour, 1);
    if (weapon_level >= 2) {
        LCD_Draw_Line((uint16_t)(x + 4), (uint16_t)(y + 6),
                      (uint16_t)(x + 12), (uint16_t)(y + 6), 1);
        LCD_Draw_Rect((uint16_t)(x + 4), (uint16_t)(y + 5), 3, 3, 6, 0);
        LCD_Draw_Rect((uint16_t)(x + 9), (uint16_t)(y + 5), 3, 3, 6, 0);
        if ((HAL_GetTick() / 140U) % 2U == 0U) {
            LCD_Draw_Rect((uint16_t)(x + 5), (uint16_t)(y + 6), 1, 1, 1, 1);
            LCD_Draw_Rect((uint16_t)(x + 10), (uint16_t)(y + 6), 1, 1, 1, 1);
        }
    }
    if (weapon_level >= 3) {
        uint8_t cape_colour = alert_colour != 0 ? alert_colour : 5;
        LCD_Draw_Line((uint16_t)(x > 3 ? x - 3 : 0), (uint16_t)(y + 7),
                      (uint16_t)(x > 7 ? x - 7 : 0), (uint16_t)(y + 15), cape_colour);
        LCD_Draw_Line((uint16_t)(x > 2 ? x - 2 : 0), (uint16_t)(y + 9),
                      (uint16_t)(x > 6 ? x - 6 : 0), (uint16_t)(y + 15), cape_colour);
        LCD_Draw_Rect((uint16_t)(x + 12), (uint16_t)(y + 8), 4, 5, 5, 0);
        LCD_Draw_Rect((uint16_t)(x + 13), (uint16_t)(y + 9), 2, 3, 6, 1);
    }

    if (weapon_level >= 3) {
        draw_agent_ak47(x, y, weapon_dir, alert_colour);
    } else {
        draw_agent_pistol(x, y, weapon_dir, alert_colour);
    }
}

static void draw_muzzle_flash(uint16_t x, uint16_t y,
                              Direction weapon_dir,
                              uint8_t weapon_level)
{
    int16_t dx = 0;
    int16_t dy = 0;
    int16_t px = (int16_t)(x + PLAYER_SIZE / 2);
    int16_t py = (int16_t)(y + PLAYER_SIZE / 2);
    int16_t muzzle_x = px;
    int16_t muzzle_y = py;
    int16_t tip_x = px;
    int16_t tip_y = py;
    int16_t side_x = 0;
    int16_t side_y = 0;
    uint8_t flash_colour = weapon_level >= 3 ? 6 : 1;

    if (weapon_dir == CENTRE) {
        weapon_dir = E;
    }

    direction_to_delta(weapon_dir, weapon_level >= 3 ? 22 : 17, &dx, &dy);
    muzzle_x = (int16_t)(px + dx);
    muzzle_y = (int16_t)(py + dy);

    direction_to_delta(weapon_dir, weapon_level >= 3 ? 7 : 5, &dx, &dy);
    tip_x = (int16_t)(muzzle_x + dx);
    tip_y = (int16_t)(muzzle_y + dy);

    side_x = (int16_t)(dy / 2);
    side_y = (int16_t)(-dx / 2);

    if (muzzle_x < 0) { muzzle_x = 0; }
    if (muzzle_y < ARENA_TOP) { muzzle_y = ARENA_TOP; }
    if (muzzle_x >= SCREEN_WIDTH) { muzzle_x = SCREEN_WIDTH - 1; }
    if (muzzle_y >= SCREEN_HEIGHT) { muzzle_y = SCREEN_HEIGHT - 1; }
    if (tip_x < 0) { tip_x = 0; }
    if (tip_y < ARENA_TOP) { tip_y = ARENA_TOP; }
    if (tip_x >= SCREEN_WIDTH) { tip_x = SCREEN_WIDTH - 1; }
    if (tip_y >= SCREEN_HEIGHT) { tip_y = SCREEN_HEIGHT - 1; }

    LCD_Draw_Line((uint16_t)muzzle_x, (uint16_t)muzzle_y,
                  (uint16_t)tip_x, (uint16_t)tip_y, flash_colour);
    LCD_Draw_Line((uint16_t)muzzle_x, (uint16_t)muzzle_y,
                  clamp_screen_x((int16_t)(tip_x + side_x)),
                  clamp_screen_y((int16_t)(tip_y + side_y)), 1);
    LCD_Draw_Line((uint16_t)muzzle_x, (uint16_t)muzzle_y,
                  clamp_screen_x((int16_t)(tip_x - side_x)),
                  clamp_screen_y((int16_t)(tip_y - side_y)), 1);
    LCD_Draw_Circle((uint16_t)muzzle_x, (uint16_t)muzzle_y,
                    weapon_level >= 3 ? 3 : 2, flash_colour, 0);
}

static void draw_upgrade_choice(PongEngine_t* engine)
{
    static const char* titles[3] = { "FIRE RATE", "BIG SHOT", "VITALITY" };
    static const char* desc[3] = { "faster", "larger", "heal+hp" };
    uint8_t i = 0;

    LCD_Draw_Rect(16, 58, 208, 126, 1, 0);
    LCD_Draw_Rect(20, 62, 200, 118, 13, 1);
    LCD_printString("SIGNAL UPGRADE", 36, 70, 6, 2);
    LCD_printString("Right joystick choose", 44, 96, 1, 1);
    LCD_printString("SW confirm", 84, 110, 1, 1);

    for (i = 0; i < 3; i++) {
        uint16_t x = (uint16_t)(28 + i * 64U);
        uint8_t selected = engine->upgrade_choice_index == i;
        uint8_t colour = selected ? 6 : 9;
        LCD_Draw_Rect(x, 132, 54, 36, colour, selected ? 0 : 1);
        LCD_printString(titles[i], (uint16_t)(x + 3), 138, selected ? 6 : 1, 1);
        LCD_printString(desc[i], (uint16_t)(x + 7), 154, selected ? 1 : 6, 1);
    }
}

static void draw_escape_ship_transition(PongEngine_t* engine, uint8_t progress)
{
    uint8_t local = (uint8_t)(progress - VICTORY_BOARDING_START_FRAME);
    int16_t ship_x = 126;
    int16_t ship_y = 96;
    int16_t cat_x = 112;
    int16_t cat_y = 128;
    int16_t cockpit_x = 0;
    int16_t cockpit_y = 0;
    uint8_t draw_cat = 1;

    LCD_Draw_Rect(0, ARENA_TOP, SCREEN_WIDTH, SCREEN_HEIGHT - ARENA_TOP, 13, 0);
    LCD_Draw_Circle(120, 126, (uint16_t)(28 + (local % 12U)), 6, 0);
    LCD_Draw_Circle(120, 126, (uint16_t)(46 + (local % 18U)), 1, 0);
    LCD_printString("SIGNAL SECURED", 34, 48, 6, 2);
    LCD_printString("ESCAPE ROUTE OPENED", 32, 70, 1, 1);

    if (local < 42U) {
        ship_x = (int16_t)(184 - local);
        ship_y = 96;
    } else if (local < 88U) {
        ship_x = 142;
        ship_y = 96;
    } else {
        ship_x = 142;
        ship_y = (int16_t)(96 - (local - 88U) * 2);
    }

    cockpit_x = (int16_t)(ship_x + 16);
    cockpit_y = (int16_t)(ship_y + 18);

    if (ship_x >= 0 && ship_x <= (int16_t)(SCREEN_WIDTH - STRIKER_PLAYER_SHIP_W) &&
        ship_y > (int16_t)(-STRIKER_PLAYER_SHIP_H) && ship_y < (int16_t)SCREEN_HEIGHT) {
        LCD_Draw_Sprite((uint16_t)ship_x, (uint16_t)ship_y,
                        STRIKER_PLAYER_SHIP_H,
                        STRIKER_PLAYER_SHIP_W,
                        STRIKER_PLAYER_SHIPS[0]);
        LCD_Draw_Circle((uint16_t)(ship_x + 24), (uint16_t)(ship_y + 24), 8, 14, 0);
    }

    if (local < 56U) {
        cat_x = (int16_t)(112 + ((cockpit_x - 112) * local) / 56);
        cat_y = (int16_t)(128 + ((cockpit_y - 128) * local) / 56);
    } else if (local < 76U) {
        uint8_t board_progress = (uint8_t)(local - 56U);
        cat_x = (int16_t)(cockpit_x + board_progress / 3U);
        cat_y = (int16_t)(cockpit_y + board_progress / 4U);
        if ((board_progress / 3U) % 2U == 0U) {
            LCD_printString("BOARDING", 92, 150, 6, 1);
        }
        LCD_Draw_Circle((uint16_t)(ship_x + 24), (uint16_t)(ship_y + 24),
                        (uint16_t)(10 + board_progress / 2U), 6, 0);
    } else {
        draw_cat = 0;
    }

    if (draw_cat) {
        if (cat_x < 0) { cat_x = 0; }
        if (cat_y < ARENA_TOP) { cat_y = ARENA_TOP; }
        if (cat_x + PLAYER_SIZE > SCREEN_WIDTH) { cat_x = SCREEN_WIDTH - PLAYER_SIZE; }
        if (cat_y + PLAYER_SIZE > SCREEN_HEIGHT) { cat_y = SCREEN_HEIGHT - PLAYER_SIZE; }
        draw_player_sprite_facing((uint16_t)cat_x, (uint16_t)cat_y,
                                  player_agent_sprite, 0);
        draw_agent_gear((uint16_t)cat_x, (uint16_t)cat_y,
                        engine->weapon_level, E, 0);
    } else {
        LCD_printString("TO MISSION 3", 68, 150, 1, 1);
    }

    if (local >= 76U && ship_y < SCREEN_HEIGHT && ship_y > (int16_t)(-STRIKER_PLAYER_SHIP_H)) {
        uint16_t flame_x = (uint16_t)(ship_x + 24);
        uint16_t flame_y = (uint16_t)(ship_y + STRIKER_PLAYER_SHIP_H - 4);
        LCD_Draw_Line(flame_x, flame_y,
                      flame_x,
                      (uint16_t)(flame_y + 24 < SCREEN_HEIGHT ? flame_y + 24 : SCREEN_HEIGHT - 1), 6);
        LCD_Draw_Line((uint16_t)(flame_x > 4 ? flame_x - 4 : 0), flame_y,
                      (uint16_t)(flame_x > 10 ? flame_x - 10 : 0),
                      (uint16_t)(flame_y + 18 < SCREEN_HEIGHT ? flame_y + 18 : SCREEN_HEIGHT - 1), 1);
        LCD_Draw_Line((uint16_t)(flame_x + 4 < SCREEN_WIDTH ? flame_x + 4 : SCREEN_WIDTH - 1), flame_y,
                      (uint16_t)(flame_x + 10 < SCREEN_WIDTH ? flame_x + 10 : SCREEN_WIDTH - 1),
                      (uint16_t)(flame_y + 18 < SCREEN_HEIGHT ? flame_y + 18 : SCREEN_HEIGHT - 1), 1);
    }
}

static void draw_signal_waveform(uint16_t x, uint16_t y)
{
    static const int8_t wave_a[12] = { -3,  2, -1,  5, -4,  1,  4, -2,  3, -5,  1, -1 };
    static const int8_t wave_b[12] = {  2, -4,  3, -2,  1,  5, -3,  2, -5,  4, -1,  2 };
    uint16_t cx = (uint16_t)(x + SIGNAL_SIZE / 2);
    uint16_t cy = (uint16_t)(y + SIGNAL_SIZE / 2);
    uint16_t pulse = (uint16_t)(10 + ((HAL_GetTick() / 120U) % 4U));
    uint8_t phase = (uint8_t)((HAL_GetTick() / 140U) % 12U);
    uint8_t i = 0;

    LCD_Draw_Circle(cx, cy, pulse, 6, 0);
    LCD_Draw_Circle(cx, cy, (uint16_t)(pulse + 5), 1, 0);
    LCD_Draw_Line((uint16_t)(cx > 17 ? cx - 17 : 0), cy,
                  (uint16_t)(cx + 17 < SCREEN_WIDTH ? cx + 17 : SCREEN_WIDTH - 1), cy, 9);

    for (i = 0; i < 11; i++) {
        uint8_t idx0 = (uint8_t)((i + phase) % 12U);
        uint8_t idx1 = (uint8_t)((i + phase + 1U) % 12U);
        int16_t x1 = (int16_t)(cx - 16 + i * 3);
        int16_t x2 = (int16_t)(x1 + 3);
        int16_t y1 = (int16_t)(cy + wave_a[idx0]);
        int16_t y2 = (int16_t)(cy + wave_a[idx1]);
        int16_t y1b = (int16_t)(cy + wave_b[idx0] / 2);
        int16_t y2b = (int16_t)(cy + wave_b[idx1] / 2);

        if (x1 < 0) { x1 = 0; }
        if (x2 < 0) { x2 = 0; }
        if (x1 >= SCREEN_WIDTH) { x1 = SCREEN_WIDTH - 1; }
        if (x2 >= SCREEN_WIDTH) { x2 = SCREEN_WIDTH - 1; }
        if (y1 < ARENA_TOP) { y1 = ARENA_TOP; }
        if (y2 < ARENA_TOP) { y2 = ARENA_TOP; }
        if (y1b < ARENA_TOP) { y1b = ARENA_TOP; }
        if (y2b < ARENA_TOP) { y2b = ARENA_TOP; }
        if (y1 >= SCREEN_HEIGHT) { y1 = SCREEN_HEIGHT - 1; }
        if (y2 >= SCREEN_HEIGHT) { y2 = SCREEN_HEIGHT - 1; }
        if (y1b >= SCREEN_HEIGHT) { y1b = SCREEN_HEIGHT - 1; }
        if (y2b >= SCREEN_HEIGHT) { y2b = SCREEN_HEIGHT - 1; }

        LCD_Draw_Line((uint16_t)x1, (uint16_t)y1, (uint16_t)x2, (uint16_t)y2, 6);
        if ((i % 2U) == 0U) {
            LCD_Draw_Line((uint16_t)x1, (uint16_t)y1b, (uint16_t)x2, (uint16_t)y2b, 1);
        }
    }

    LCD_Draw_Rect((uint16_t)(cx > 3 ? cx - 3 : 0), (uint16_t)(cy > 3 ? cy - 3 : ARENA_TOP),
                  6, 6, 6, 0);
}

static int16_t abs_i16(int16_t v)
{
    return v < 0 ? (int16_t)-v : v;
}

static void clamp_enemy_position(SignalEnemy_t* enemy)
{
    if (enemy->x < 0) {
        enemy->x = 0;
    }
    if (enemy->y < ARENA_TOP) {
        enemy->y = ARENA_TOP;
    }
    if (enemy->x + ENEMY_SIZE > SCREEN_WIDTH) {
        enemy->x = SCREEN_WIDTH - ENEMY_SIZE;
    }
    if (enemy->y + ENEMY_SIZE > ARENA_BOTTOM) {
        enemy->y = ARENA_BOTTOM - ENEMY_SIZE;
    }
}

static void knock_enemy_away_from_player(PongEngine_t* engine, SignalEnemy_t* enemy)
{
    int16_t enemy_cx = (int16_t)(enemy->x + ENEMY_SIZE / 2);
    int16_t enemy_cy = (int16_t)(enemy->y + ENEMY_SIZE / 2);
    int16_t player_cx = (int16_t)(engine->player_x + PLAYER_SIZE / 2);
    int16_t player_cy = (int16_t)(engine->player_y + PLAYER_SIZE / 2);
    int16_t dx = enemy_cx - player_cx;
    int16_t dy = enemy_cy - player_cy;

    if (abs_i16(dx) >= abs_i16(dy)) {
        enemy->x += dx >= 0 ? ENEMY_KNOCKBACK_PX : -ENEMY_KNOCKBACK_PX;
    } else {
        enemy->y += dy >= 0 ? ENEMY_KNOCKBACK_PX : -ENEMY_KNOCKBACK_PX;
    }

    enemy->flash_frames = 10;
    clamp_enemy_position(enemy);
}

static void clamp_player_position(PongEngine_t* engine)
{
    if (engine->player_x < 0) {
        engine->player_x = 0;
    }
    if (engine->player_y < ARENA_TOP) {
        engine->player_y = ARENA_TOP;
    }
    if (engine->player_x + PLAYER_SIZE > SCREEN_WIDTH) {
        engine->player_x = SCREEN_WIDTH - PLAYER_SIZE;
    }
    if (engine->player_y + PLAYER_SIZE > ARENA_BOTTOM) {
        engine->player_y = ARENA_BOTTOM - PLAYER_SIZE;
    }
}

static void direction_to_delta(Direction direction, int16_t speed, int16_t* dx, int16_t* dy)
{
    *dx = 0;
    *dy = 0;

    switch (direction) {
        case N:  *dy = (int16_t)-speed; break;
        case NE: *dx = speed; *dy = (int16_t)-speed; break;
        case E:  *dx = speed; break;
        case SE: *dx = speed; *dy = speed; break;
        case S:  *dy = speed; break;
        case SW: *dx = (int16_t)-speed; *dy = speed; break;
        case W:  *dx = (int16_t)-speed; break;
        case NW: *dx = (int16_t)-speed; *dy = (int16_t)-speed; break;
        case CENTRE: default: break;
    }
}

static void start_shadow_dash(PongEngine_t* engine, Direction direction)
{
    uint8_t i = 0;
    uint8_t nearby = 0;

    if (direction == CENTRE || engine->dash_cooldown_frames > 0 || engine->dash_frames > 0) {
        return;
    }

    for (i = 0; i < SIGNAL_MAX_ENEMIES; i++) {
        SignalEnemy_t* enemy = &engine->enemies[i];
        if (enemy->active &&
            abs_i16((int16_t)(enemy->x - engine->player_x)) < 42 &&
            abs_i16((int16_t)(enemy->y - engine->player_y)) < 42) {
            nearby++;
        }
    }

    engine->dash_dir = direction;
    engine->dash_frames = SHADOW_DASH_FRAMES;
    engine->dash_cooldown_frames = SHADOW_DASH_COOLDOWN_FRAMES;
    if (nearby >= 2) {
        engine->dash_evade_message_frames = SHADOW_DASH_EVADE_FRAMES;
    }
    if (engine->invulnerable_frames < SHADOW_DASH_INVULNERABLE_FRAMES) {
        engine->invulnerable_frames = SHADOW_DASH_INVULNERABLE_FRAMES;
    }

    for (i = 0; i < 4; i++) {
        spawn_particles(engine,
                        (int16_t)(engine->player_x + PLAYER_SIZE / 2),
                        (int16_t)(engine->player_y + PLAYER_SIZE / 2),
                        i % 2U == 0U ? 14 : 6);
    }
    beep(BUZZER_DASH_FREQ_HZ);
}

static uint8_t jammer_near_player(PongEngine_t* engine)
{
    uint8_t i = 0;

    for (i = 0; i < SIGNAL_MAX_ENEMIES; i++) {
        SignalEnemy_t* enemy = &engine->enemies[i];
        if (enemy->active && enemy->type == ENEMY_TYPE_JAMMER &&
            abs_i16((int16_t)(enemy->x - engine->player_x)) < 44 &&
            abs_i16((int16_t)(enemy->y - engine->player_y)) < 44) {
            return 1;
        }
    }

    return 0;
}

static void start_player_hit_feedback(PongEngine_t* engine)
{
    if (engine->lives > 0) {
        engine->lives--;
    }
    engine->invulnerable_frames = HIT_INVULNERABLE_FRAMES;
    engine->hit_effect_frames = HIT_FLASH_FRAMES;
    spawn_particles(engine,
                    (int16_t)(engine->player_x + PLAYER_SIZE / 2),
                    (int16_t)(engine->player_y + PLAYER_SIZE / 2),
                    2);
    beep(BUZZER_HIT_FREQ_HZ);
}

static uint8_t is_decode_direction(Direction direction)
{
    return (direction == N || direction == E || direction == S || direction == W);
}

static Direction random_decode_direction(void)
{
    uint16_t v = Random_U16(4);
    if (v == 0) {
        return N;
    }
    if (v == 1) {
        return E;
    }
    if (v == 2) {
        return S;
    }
    return W;
}

static char direction_to_symbol(Direction direction)
{
    if (direction == N) {
        return '^';
    }
    if (direction == E) {
        return '>';
    }
    if (direction == S) {
        return 'v';
    }
    if (direction == W) {
        return '<';
    }
    return '?';
}

static void beep(uint32_t freq_hz)
{
    buzzer_tone(&buzzer_cfg, freq_hz, BUZZER_VOLUME);
    buzzer_stop_tick = HAL_GetTick() + BUZZER_BEEP_MS;
}

static void update_buzzer(void)
{
    if (buzzer_stop_tick != 0 && (int32_t)(HAL_GetTick() - buzzer_stop_tick) >= 0) {
        buzzer_off(&buzzer_cfg);
        buzzer_stop_tick = 0;
    }
}

static void reset_player(PongEngine_t* engine)
{
    engine->player_x = (SCREEN_WIDTH - PLAYER_SIZE) / 2;
    engine->player_y = (ARENA_TOP + ARENA_BOTTOM - PLAYER_SIZE) / 2;
}

static void place_signal(PongEngine_t* engine)
{
    uint8_t tries = 0;

    while (tries < 30) {
        int16_t x = (int16_t)(12 + Random_U16(SCREEN_WIDTH - SIGNAL_SIZE - 24));
        int16_t y = (int16_t)(ARENA_TOP + 12 + Random_U16(ARENA_BOTTOM - ARENA_TOP - SIGNAL_SIZE - 24));

        if (!rects_overlap(x, y, SIGNAL_SIZE, SIGNAL_SIZE,
                           engine->player_x, engine->player_y, PLAYER_SIZE, PLAYER_SIZE)) {
            engine->signal_x = (uint8_t)x;
            engine->signal_y = (uint8_t)y;
            return;
        }
        tries++;
    }

    engine->signal_x = 110;
    engine->signal_y = 120;
}

static void start_signal_decode(PongEngine_t* engine)
{
    uint8_t i = 0;

    engine->signal_decode_active = 1;
    engine->decode_index = 0;
    engine->last_decode_dir = CENTRE;
    for (i = 0; i < 3; i++) {
        engine->decode_sequence[i] = random_decode_direction();
    }
    beep(BUZZER_SIGNAL_FREQ_HZ);
}

static void finish_signal_decode(PongEngine_t* engine)
{
    engine->signal_decode_active = 0;
    engine->decode_index = 0;
    engine->last_decode_dir = CENTRE;
    engine->signals_collected = 0;
    if (engine->total_signals_collected < 99) {
        engine->total_signals_collected++;
    }

    if (engine->weapon_level < 3) {
        engine->weapon_level++;
        engine->upgrade_message_frames = 42;
        beep(BUZZER_UPGRADE_FREQ_HZ);
    } else {
        engine->upgrade_choice_active = 1;
        engine->upgrade_choice_index = 0;
        engine->upgrade_choice_message_frames = 0;
        engine->fire_cooldown_frames = 10;
        beep(BUZZER_SIGNAL_FREQ_HZ);
    }

    place_signal(engine);
}

static uint8_t enemy_count(PongEngine_t* engine)
{
    uint8_t i = 0;
    uint8_t count = 0;
    for (i = 0; i < SIGNAL_MAX_ENEMIES; i++) {
        if (engine->enemies[i].active) {
            count++;
        }
    }
    return count;
}

static void apply_upgrade_choice(PongEngine_t* engine)
{
    if (!engine->upgrade_choice_active) {
        return;
    }

    if (engine->upgrade_choice_index == 0) {
        if (engine->fire_rate_level < MAX_STAT_UPGRADE_LEVEL) {
            engine->fire_rate_level++;
        } else if (engine->lives < engine->max_lives) {
            engine->lives++;
        }
    } else if (engine->upgrade_choice_index == 1) {
        if (engine->bullet_size_level < MAX_STAT_UPGRADE_LEVEL) {
            engine->bullet_size_level++;
        } else {
            engine->score++;
        }
    } else {
        if (engine->vitality_level < MAX_STAT_UPGRADE_LEVEL) {
            engine->vitality_level++;
        }
        if (engine->max_lives < MAX_UPGRADED_LIVES) {
            engine->max_lives++;
        }
        if (engine->lives < engine->max_lives) {
            engine->lives++;
        }
        if (engine->lives < engine->max_lives) {
            engine->lives++;
        }
    }

    engine->upgrade_choice_active = 0;
    engine->upgrade_choice_message_frames = 50;
    engine->upgrade_message_frames = 35;
    engine->ultimate_flash_frames = 12;
    spawn_particles(engine,
                    (int16_t)(engine->player_x + PLAYER_SIZE / 2),
                    (int16_t)(engine->player_y + PLAYER_SIZE / 2),
                    engine->upgrade_choice_index == 0 ? 6 : (engine->upgrade_choice_index == 1 ? 1 : 3));
    beep(BUZZER_UPGRADE_FREQ_HZ);
}

static void trigger_signal_trap(PongEngine_t* engine, SignalTrap_t* trap)
{
    uint8_t i = 0;
    int16_t trap_cx = (int16_t)(trap->x + SIGNAL_TRAP_SIZE / 2);
    int16_t trap_cy = (int16_t)(trap->y + SIGNAL_TRAP_SIZE / 2);

    for (i = 0; i < SIGNAL_MAX_ENEMIES; i++) {
        SignalEnemy_t* enemy = &engine->enemies[i];
        int16_t enemy_cx = 0;
        int16_t enemy_cy = 0;
        int16_t dist = 0;

        if (!enemy->active) {
            continue;
        }

        enemy_cx = (int16_t)(enemy->x + ENEMY_SIZE / 2);
        enemy_cy = (int16_t)(enemy->y + ENEMY_SIZE / 2);
        dist = (int16_t)(abs_i16(enemy_cx - trap_cx) + abs_i16(enemy_cy - trap_cy));
        if (dist <= SIGNAL_TRAP_RADIUS + 10) {
            uint8_t was_elite = enemy->elite;
            enemy->flash_frames = 8;
            enemy->speed = enemy->speed > 1 ? (int16_t)(enemy->speed - 1) : enemy->speed;
            if (enemy->hp > SIGNAL_TRAP_DAMAGE) {
                enemy->hp = (uint8_t)(enemy->hp - SIGNAL_TRAP_DAMAGE);
            } else {
                enemy->active = 0;
                enemy->hp = 0;
                award_enemy_kill(engine, was_elite);
                if (was_elite && engine->elite_kills < 255) {
                    engine->elite_kills++;
                }
            }
        }
    }

    if (engine->boss_active) {
        int16_t boss_cx = (int16_t)(engine->boss_x + BOSS_SIZE / 2);
        int16_t boss_cy = (int16_t)(engine->boss_y + BOSS_SIZE / 2);
        int16_t boss_dist = (int16_t)(abs_i16(boss_cx - trap_cx) + abs_i16(boss_cy - trap_cy));
        if (boss_dist <= SIGNAL_TRAP_RADIUS + 16) {
            damage_boss(engine, SIGNAL_TRAP_BOSS_DAMAGE);
        }
    }

    trap->active = 0;
    trap->pulse_frames = SIGNAL_TRAP_PULSE_FRAMES;
    spawn_particles(engine, trap_cx, trap_cy, 6);
    spawn_particles(engine, trap_cx, trap_cy, 1);
    engine->trap_message_frames = 28;
    beep(BUZZER_UPGRADE_FREQ_HZ);
}

static void place_signal_trap(PongEngine_t* engine)
{
    uint8_t i = 0;

    if (engine->lives == 0 || engine->signal_decode_active ||
        engine->upgrade_choice_active || engine->trap_cooldown_frames > 0) {
        return;
    }

    for (i = 0; i < SIGNAL_MAX_TRAPS; i++) {
        SignalTrap_t* trap = &engine->traps[i];
        if (trap->active) {
            continue;
        }

        trap->active = 1;
        trap->armed_frames = SIGNAL_TRAP_LIFETIME_FRAMES;
        trap->pulse_frames = 0;
        trap->x = (int16_t)(engine->player_x + PLAYER_SIZE / 2 - SIGNAL_TRAP_SIZE / 2);
        trap->y = (int16_t)(engine->player_y + PLAYER_SIZE / 2 - SIGNAL_TRAP_SIZE / 2);
        if (trap->x < 0) { trap->x = 0; }
        if (trap->y < ARENA_TOP) { trap->y = ARENA_TOP; }
        if (trap->x + SIGNAL_TRAP_SIZE > SCREEN_WIDTH) { trap->x = SCREEN_WIDTH - SIGNAL_TRAP_SIZE; }
        if (trap->y + SIGNAL_TRAP_SIZE > ARENA_BOTTOM) { trap->y = ARENA_BOTTOM - SIGNAL_TRAP_SIZE; }

        engine->trap_cooldown_frames = SIGNAL_TRAP_COOLDOWN_FRAMES;
        engine->trap_message_frames = 24;
        spawn_particles(engine, trap->x + SIGNAL_TRAP_SIZE / 2, trap->y + SIGNAL_TRAP_SIZE / 2, 6);
        beep(BUZZER_SIGNAL_FREQ_HZ);
        return;
    }
}

static void update_signal_traps(PongEngine_t* engine)
{
    uint8_t i = 0;

    for (i = 0; i < SIGNAL_MAX_TRAPS; i++) {
        SignalTrap_t* trap = &engine->traps[i];
        uint8_t j = 0;

        if (trap->pulse_frames > 0) {
            trap->pulse_frames--;
        }
        if (!trap->active) {
            continue;
        }
        if (trap->armed_frames > 0) {
            trap->armed_frames--;
        } else {
            trap->active = 0;
            trap->pulse_frames = 8;
            continue;
        }

        for (j = 0; j < SIGNAL_MAX_ENEMIES; j++) {
            SignalEnemy_t* enemy = &engine->enemies[j];
            int16_t trap_cx = (int16_t)(trap->x + SIGNAL_TRAP_SIZE / 2);
            int16_t trap_cy = (int16_t)(trap->y + SIGNAL_TRAP_SIZE / 2);
            int16_t enemy_cx = 0;
            int16_t enemy_cy = 0;

            if (!enemy->active) {
                continue;
            }
            enemy_cx = (int16_t)(enemy->x + ENEMY_SIZE / 2);
            enemy_cy = (int16_t)(enemy->y + ENEMY_SIZE / 2);
            if ((abs_i16(enemy_cx - trap_cx) + abs_i16(enemy_cy - trap_cy)) <= 18) {
                trigger_signal_trap(engine, trap);
                break;
            }
        }
    }

    if (engine->trap_cooldown_frames > 0) {
        engine->trap_cooldown_frames--;
    }
    if (engine->trap_message_frames > 0) {
        engine->trap_message_frames--;
    }
}

static void spawn_enemy(PongEngine_t* engine)
{
    uint8_t i = 0;
    uint8_t side = (uint8_t)Random_U16(4);
    uint8_t elite = 0;
    SignalEnemy_t* enemy = 0;

    for (i = 0; i < SIGNAL_MAX_ENEMIES; i++) {
        if (!engine->enemies[i].active) {
            enemy = &engine->enemies[i];
            break;
        }
    }

    if (enemy == 0) {
        return;
    }

    if (engine->first_boss_defeated && !engine->elite_boss_spawned) {
        elite = Random_U16(100) < 40;
    }

    enemy->active = 1;
    enemy->elite = elite;
    enemy->type = ENEMY_TYPE_CHASER;
    if (engine->wave >= 2 && Random_U16(100) < 28) {
        enemy->type = ENEMY_TYPE_SNIPER;
    }
    if (engine->wave >= 3 && Random_U16(100) < 22) {
        enemy->type = ENEMY_TYPE_JAMMER;
    }
    enemy->hp = elite ? (uint8_t)(2 + engine->wave / 4) : (uint8_t)(1 + (engine->wave >= 4 ? 1 : 0));
    if (enemy->type == ENEMY_TYPE_SNIPER) {
        enemy->hp++;
    }
    enemy->speed = elite ? (int16_t)(1 + (engine->wave >= 5 ? 1 : 0)) : (int16_t)(1 + (engine->wave >= 3 ? 1 : 0));
    if (enemy->type == ENEMY_TYPE_SNIPER && enemy->speed > 1) {
        enemy->speed--;
    }
    if (enemy->type == ENEMY_TYPE_JAMMER && enemy->speed < 2) {
        enemy->speed = 2;
    }
    enemy->flash_frames = 0;
    enemy->fire_cooldown_frames = (uint16_t)(45 + Random_U16(35));

    if (side == 0) {
        enemy->x = (int16_t)Random_U16(SCREEN_WIDTH - ENEMY_SIZE);
        enemy->y = ARENA_TOP;
    } else if (side == 1) {
        enemy->x = (int16_t)Random_U16(SCREEN_WIDTH - ENEMY_SIZE);
        enemy->y = ARENA_BOTTOM - ENEMY_SIZE;
    } else if (side == 2) {
        enemy->x = 0;
        enemy->y = (int16_t)(ARENA_TOP + Random_U16(ARENA_BOTTOM - ARENA_TOP - ENEMY_SIZE));
    } else {
        enemy->x = SCREEN_WIDTH - ENEMY_SIZE;
        enemy->y = (int16_t)(ARENA_TOP + Random_U16(ARENA_BOTTOM - ARENA_TOP - ENEMY_SIZE));
    }
}

static void spawn_boss(PongEngine_t* engine, uint8_t tier)
{
    engine->boss_active = 1;
    if (tier == 1) {
        engine->boss_spawned = 1;
    } else {
        engine->elite_boss_spawned = 1;
    }
    engine->boss_tier = tier;
    engine->boss_max_hp = tier == 2 ? ELITE_BOSS_MAX_HP : FIRST_BOSS_MAX_HP;
    engine->boss_hp = engine->boss_max_hp;
    engine->boss_x = (SCREEN_WIDTH - BOSS_SIZE) / 2;
    engine->boss_y = ARENA_TOP + 8;
    engine->boss_flash_frames = 0;
    engine->boss_move_frames = 0;
    engine->boss_fire_cooldown_frames = tier == 2 ? 35 : 60;
    engine->boss_title_frames = 60;
    engine->boss_phase = 1;
    engine->boss_phase_title_frames = 0;
    engine->boss_charge_frames = 0;
    engine->boss_charge_cooldown_frames = tier == 2 ? 55 : 80;
    engine->boss_charge_dx = 0;
    engine->boss_charge_dy = 0;
    engine->boss_summon_cooldown_frames = tier == 2 ? 70 : 95;
    beep(BUZZER_BOSS_FREQ_HZ);
}

static void spawn_heal_drop(PongEngine_t* engine, int16_t x, int16_t y)
{
    if (engine->heal_active) {
        return;
    }

    if (x < 0) {
        x = 0;
    }
    if (y < ARENA_TOP) {
        y = ARENA_TOP;
    }
    if (x + HEAL_SIZE > SCREEN_WIDTH) {
        x = SCREEN_WIDTH - HEAL_SIZE;
    }
    if (y + HEAL_SIZE > ARENA_BOTTOM) {
        y = ARENA_BOTTOM - HEAL_SIZE;
    }

    engine->heal_active = 1;
    engine->heal_x = (uint8_t)x;
    engine->heal_y = (uint8_t)y;
}

static void spawn_time_chest(PongEngine_t* engine, int16_t x, int16_t y)
{
    if (x < 0) {
        x = 0;
    }
    if (y < ARENA_TOP) {
        y = ARENA_TOP;
    }
    if (x + CHEST_SIZE > SCREEN_WIDTH) {
        x = SCREEN_WIDTH - CHEST_SIZE;
    }
    if (y + CHEST_SIZE > ARENA_BOTTOM) {
        y = ARENA_BOTTOM - CHEST_SIZE;
    }

    engine->chest_active = 1;
    engine->chest_x = x;
    engine->chest_y = y;
}

static void start_victory_animation(PongEngine_t* engine)
{
    engine->chest_active = 0;
    engine->victory_animation_frames = VICTORY_ANIMATION_FRAMES;
    engine->invulnerable_frames = VICTORY_ANIMATION_FRAMES;
    engine->ultimate_freeze_frames = VICTORY_ANIMATION_FRAMES;
    engine->heal_active = 0;
    clear_enemy_pressure(engine);
    spawn_particles(engine,
                    (int16_t)(engine->player_x + PLAYER_SIZE / 2),
                    (int16_t)(engine->player_y + PLAYER_SIZE / 2),
                    6);
    beep(BUZZER_UPGRADE_FREQ_HZ);
}

static void spawn_particles(PongEngine_t* engine, int16_t x, int16_t y, uint8_t colour)
{
    static const int8_t dirs[8][2] = {
        { 2,  0}, { 2,  1}, { 1,  2}, {-1,  2},
        {-2,  0}, {-2, -1}, {-1, -2}, { 1, -2}
    };
    uint8_t i = 0;
    uint8_t spawned = 0;

    for (i = 0; i < SIGNAL_MAX_PARTICLES && spawned < 8; i++) {
        SignalParticle_t* particle = &engine->particles[i];
        if (particle->life != 0) {
            continue;
        }
        particle->x = x;
        particle->y = y;
        particle->dx = dirs[spawned][0];
        particle->dy = dirs[spawned][1];
        particle->life = 12;
        particle->colour = colour;
        spawned++;
    }
}

static void update_particles(PongEngine_t* engine)
{
    uint8_t i = 0;

    for (i = 0; i < SIGNAL_MAX_PARTICLES; i++) {
        SignalParticle_t* particle = &engine->particles[i];
        if (particle->life == 0) {
            continue;
        }
        particle->x += particle->dx;
        particle->y += particle->dy;
        particle->life--;
    }
}

static void start_boss_warning(PongEngine_t* engine, uint8_t tier)
{
    engine->boss_pending_tier = tier;
    engine->boss_warning_frames = BOSS_WARNING_FRAMES;
    beep(BUZZER_BOSS_FREQ_HZ);
}

static void clear_enemy_pressure(PongEngine_t* engine)
{
    uint8_t i = 0;

    for (i = 0; i < SIGNAL_MAX_ENEMIES; i++) {
        engine->enemies[i].active = 0;
        engine->enemies[i].elite = 0;
        engine->enemies[i].type = ENEMY_TYPE_CHASER;
        engine->enemies[i].fire_cooldown_frames = 0;
    }
    for (i = 0; i < SIGNAL_MAX_BOSS_SHOTS; i++) {
        engine->boss_shots[i].active = 0;
    }
}

static void reset_combo(PongEngine_t* engine)
{
    engine->combo_kills = 0;
    engine->combo_multiplier = 1;
    engine->combo_timer_frames = 0;
}

static void award_enemy_kill(PongEngine_t* engine, uint8_t was_elite)
{
    uint8_t base_points = was_elite ? 2 : 1;

    if (engine->combo_timer_frames == 0) {
        engine->combo_kills = 0;
    }
    if (engine->combo_kills < 250) {
        engine->combo_kills++;
    }

    if (engine->combo_kills >= 6) {
        engine->combo_multiplier = 3;
    } else if (engine->combo_kills >= 3) {
        engine->combo_multiplier = 2;
    } else {
        engine->combo_multiplier = 1;
    }

    engine->combo_timer_frames = COMBO_TIMER_FRAMES;
    if (engine->combo_multiplier > 1) {
        engine->combo_message_frames = 45;
    }
    engine->score = (uint16_t)(engine->score + base_points * engine->combo_multiplier);
}

static void damage_boss(PongEngine_t* engine, uint8_t damage)
{
    uint8_t defeated_tier = 0;

    if (!engine->boss_active) {
        return;
    }

    engine->boss_flash_frames = 6;
    if (engine->boss_hp > damage) {
        engine->boss_hp = (uint8_t)(engine->boss_hp - damage);
        if (engine->boss_phase == 1 && engine->boss_hp <= engine->boss_max_hp / 2U) {
            engine->boss_phase = 2;
            engine->boss_phase_title_frames = BOSS_PHASE_TITLE_FRAMES;
            engine->boss_charge_cooldown_frames = 20;
            engine->boss_summon_cooldown_frames = 25;
            engine->boss_fire_cooldown_frames = engine->boss_tier == 2 ? 16 : 24;
            engine->ultimate_flash_frames = 18;
            spawn_particles(engine,
                            (int16_t)(engine->boss_x + BOSS_SIZE / 2),
                            (int16_t)(engine->boss_y + BOSS_SIZE / 2),
                            engine->boss_tier == 2 ? 5 : 2);
            beep(BUZZER_BOSS_FREQ_HZ);
        }
        beep(BUZZER_KILL_FREQ_HZ);
        return;
    }

    defeated_tier = engine->boss_tier;
    engine->boss_hp = 0;
    engine->boss_active = 0;
    engine->boss_tier = 0;
    engine->boss_phase = 0;
    engine->boss_phase_title_frames = 0;
    engine->boss_charge_frames = 0;
    engine->boss_charge_cooldown_frames = 0;
    engine->boss_summon_cooldown_frames = 0;
    if (defeated_tier == 1) {
        engine->first_boss_defeated = 1;
        engine->elite_kills = 0;
        engine->invulnerable_frames = 100;
        engine->spawn_cooldown_frames = 45;
        engine->time_left_s = engine->time_left_s + 30U > 120U ? 120U : (uint16_t)(engine->time_left_s + 30U);
        clear_enemy_pressure(engine);
        engine->score += 8;
    } else {
        engine->invulnerable_frames = 140;
        engine->spawn_cooldown_frames = 300;
        clear_enemy_pressure(engine);
        spawn_time_chest(engine,
                         (int16_t)(engine->boss_x + BOSS_SIZE / 2 - CHEST_SIZE / 2),
                         (int16_t)(engine->boss_y + BOSS_SIZE / 2 - CHEST_SIZE / 2));
        engine->score += 15;
    }
    engine->hit_effect_frames = 12;
    if (defeated_tier == 1) {
        spawn_heal_drop(engine, engine->boss_x + BOSS_SIZE / 2, engine->boss_y + BOSS_SIZE / 2);
    }
    spawn_particles(engine, engine->boss_x + BOSS_SIZE / 2, engine->boss_y + BOSS_SIZE / 2,
                    defeated_tier == 2 ? 5 : 2);
    beep(BUZZER_UPGRADE_FREQ_HZ);
}

static int8_t find_nearest_enemy(PongEngine_t* engine)
{
    uint8_t i = 0;
    int8_t best = -1;
    int16_t best_dist = 30000;

    for (i = 0; i < SIGNAL_MAX_ENEMIES; i++) {
        SignalEnemy_t* enemy = &engine->enemies[i];
        if (!enemy->active) {
            continue;
        }

        int16_t dist = (int16_t)(abs_i16(enemy->x - engine->player_x) + abs_i16(enemy->y - engine->player_y));
        if (dist < best_dist) {
            best_dist = dist;
            best = (int8_t)i;
        }
    }

    return best;
}

static uint8_t direction_matches_vector(Direction direction, int16_t dx, int16_t dy)
{
    switch (direction) {
        case N:  return dy < 0 && abs_i16(dx) <= abs_i16(dy) + 10;
        case NE: return dx > 0 && dy < 0;
        case E:  return dx > 0 && abs_i16(dy) <= abs_i16(dx) + 10;
        case SE: return dx > 0 && dy > 0;
        case S:  return dy > 0 && abs_i16(dx) <= abs_i16(dy) + 10;
        case SW: return dx < 0 && dy > 0;
        case W:  return dx < 0 && abs_i16(dy) <= abs_i16(dx) + 10;
        case NW: return dx < 0 && dy < 0;
        case CENTRE:
        default: return 0;
    }
}

static int8_t find_enemy_in_aim_sector(PongEngine_t* engine, Direction aim_dir)
{
    uint8_t i = 0;
    int8_t best = -1;
    int16_t best_dist = 30000;
    int16_t player_cx = (int16_t)(engine->player_x + PLAYER_SIZE / 2);
    int16_t player_cy = (int16_t)(engine->player_y + PLAYER_SIZE / 2);

    if (aim_dir == CENTRE) {
        return -1;
    }

    for (i = 0; i < SIGNAL_MAX_ENEMIES; i++) {
        SignalEnemy_t* enemy = &engine->enemies[i];
        int16_t dx = 0;
        int16_t dy = 0;
        int16_t dist = 0;

        if (!enemy->active) {
            continue;
        }

        dx = (int16_t)(enemy->x + ENEMY_SIZE / 2 - player_cx);
        dy = (int16_t)(enemy->y + ENEMY_SIZE / 2 - player_cy);
        if (!direction_matches_vector(aim_dir, dx, dy)) {
            continue;
        }

        dist = (int16_t)(abs_i16(dx) + abs_i16(dy));
        if (dist < best_dist) {
            best_dist = dist;
            best = (int8_t)i;
        }
    }

    return best;
}

static void fire_projectile(PongEngine_t* engine, UserInput aim_input)
{
    uint8_t i = 0;
    uint8_t fired = 0;
    uint8_t shots_to_fire = engine->weapon_level;
    uint8_t manual_aim = (aim_input.direction != CENTRE && aim_input.magnitude > AIM_ASSIST_MIN_MAGNITUDE);
    int8_t target_index = manual_aim ? find_enemy_in_aim_sector(engine, aim_input.direction) : find_nearest_enemy(engine);
    SignalEnemy_t* target = 0;
    int16_t target_x = 0;
    int16_t target_y = 0;
    int16_t aim_dx = 0;
    int16_t aim_dy = 0;

    if (manual_aim && target_index < 0 && engine->boss_active) {
        int16_t boss_dx = (int16_t)(engine->boss_x + BOSS_SIZE / 2 - (engine->player_x + PLAYER_SIZE / 2));
        int16_t boss_dy = (int16_t)(engine->boss_y + BOSS_SIZE / 2 - (engine->player_y + PLAYER_SIZE / 2));
        if (direction_matches_vector(aim_input.direction, boss_dx, boss_dy)) {
            target_x = engine->boss_x;
            target_y = engine->boss_y;
        }
    }

    if (!manual_aim && target_index < 0 && !engine->boss_active) {
        return;
    }

    if (shots_to_fire < 1) {
        shots_to_fire = 1;
    }
    if (shots_to_fire > 3) {
        shots_to_fire = 3;
    }

    for (i = 0; i < SIGNAL_MAX_PROJECTILES && fired < shots_to_fire; i++) {
        SignalProjectile_t* shot = &engine->projectiles[i];
        if (shot->active) {
            continue;
        }

        if (target_index >= 0) {
            target = &engine->enemies[(uint8_t)target_index];
            target_x = target->x;
            target_y = target->y;
        } else if (engine->boss_active && (!manual_aim ||
                   direction_matches_vector(aim_input.direction,
                                            (int16_t)(engine->boss_x - engine->player_x),
                                            (int16_t)(engine->boss_y - engine->player_y)))) {
            target_x = engine->boss_x;
            target_y = engine->boss_y;
        } else {
            target_x = engine->player_x;
            target_y = engine->player_y;
        }

        shot->active = 1;
        shot->damage = 1;
        shot->size = player_shot_size(engine);
        shot->x = (int16_t)(engine->player_x + PLAYER_SIZE / 2 - shot->size / 2);
        shot->y = (int16_t)(engine->player_y + PLAYER_SIZE / 2 - shot->size / 2);

        if (manual_aim && target_index < 0 &&
            !(engine->boss_active &&
              direction_matches_vector(aim_input.direction,
                                       (int16_t)(engine->boss_x - engine->player_x),
                                       (int16_t)(engine->boss_y - engine->player_y)))) {
            direction_to_delta(aim_input.direction, 5, &aim_dx, &aim_dy);
            if (aim_dx == 0 && aim_dy == 0) {
                aim_dx = 5;
            }
            shot->dx = aim_dx;
            shot->dy = aim_dy;
            if (shots_to_fire > 1 && fired == 1) {
                shot->x = (int16_t)(shot->x + (aim_dy == 0 ? 0 : 4));
                shot->y = (int16_t)(shot->y + (aim_dx == 0 ? 0 : 4));
            } else if (shots_to_fire > 2 && fired == 2) {
                shot->x = (int16_t)(shot->x - (aim_dy == 0 ? 0 : 4));
                shot->y = (int16_t)(shot->y - (aim_dx == 0 ? 0 : 4));
            }
            fired++;
            continue;
        }

        {
            int16_t dx = target_x - engine->player_x;
            int16_t dy = target_y - engine->player_y;

            if (abs_i16(dx) > abs_i16(dy)) {
                shot->dx = dx > 0 ? 5 : -5;
                shot->dy = dy > 0 ? (int16_t)(2 + fired) : (int16_t)(-2 - fired);
            } else {
                shot->dx = dx > 0 ? (int16_t)(2 + fired) : (int16_t)(-2 - fired);
                shot->dy = dy > 0 ? 5 : -5;
            }
        }

        fired++;
    }

    if (fired > 0) {
        beep(BUZZER_SHOT_FREQ_HZ);
    }
}

static void fire_enemy_shot(PongEngine_t* engine, SignalEnemy_t* enemy)
{
    uint8_t i = 0;

    for (i = 0; i < SIGNAL_MAX_BOSS_SHOTS; i++) {
        SignalProjectile_t* shot = &engine->boss_shots[i];
        if (shot->active) {
            continue;
        }

        int16_t dx = engine->player_x - enemy->x;
        int16_t dy = engine->player_y - enemy->y;

        shot->active = 1;
        shot->damage = 1;
        shot->size = BOSS_SHOT_SIZE;
        shot->x = (int16_t)(enemy->x + ENEMY_SIZE / 2 - BOSS_SHOT_SIZE / 2);
        shot->y = (int16_t)(enemy->y + ENEMY_SIZE / 2 - BOSS_SHOT_SIZE / 2);

        if (abs_i16(dx) > abs_i16(dy)) {
            shot->dx = dx > 0 ? 4 : -4;
            shot->dy = dy > 0 ? 1 : -1;
        } else {
            shot->dx = dx > 0 ? 1 : -1;
            shot->dy = dy > 0 ? 4 : -4;
        }
        beep(BUZZER_HIT_FREQ_HZ);
        return;
    }
}

static void update_enemies(PongEngine_t* engine)
{
    uint8_t i = 0;

    for (i = 0; i < SIGNAL_MAX_ENEMIES; i++) {
        SignalEnemy_t* enemy = &engine->enemies[i];
        int16_t dx = 0;
        int16_t dy = 0;
        int16_t dist = 0;
        if (!enemy->active) {
            continue;
        }

        dx = (int16_t)(engine->player_x - enemy->x);
        dy = (int16_t)(engine->player_y - enemy->y);
        dist = (int16_t)(abs_i16(dx) + abs_i16(dy));

        if (enemy->fire_cooldown_frames > 0) {
            enemy->fire_cooldown_frames--;
        }

        if (enemy->type == ENEMY_TYPE_SNIPER) {
            if (dist < 62) {
                enemy->x += dx > 0 ? (int16_t)-enemy->speed : enemy->speed;
                enemy->y += dy > 0 ? (int16_t)-enemy->speed : enemy->speed;
            } else if (dist > 105) {
                enemy->x += dx > 0 ? enemy->speed : (int16_t)-enemy->speed;
                enemy->y += dy > 0 ? enemy->speed : (int16_t)-enemy->speed;
            } else if (enemy->fire_cooldown_frames == 0) {
                fire_enemy_shot(engine, enemy);
                enemy->fire_cooldown_frames = enemy->elite ? 50 : 70;
            }
        } else {
            if (enemy->x < engine->player_x) {
                enemy->x += enemy->speed;
            } else if (enemy->x > engine->player_x) {
                enemy->x -= enemy->speed;
            }

            if (enemy->y < engine->player_y) {
                enemy->y += enemy->speed;
            } else if (enemy->y > engine->player_y) {
                enemy->y -= enemy->speed;
            }

            if (enemy->type == ENEMY_TYPE_JAMMER && dist < 48 &&
                engine->signal_decode_active && enemy->fire_cooldown_frames == 0) {
                engine->decode_index = 0;
                enemy->fire_cooldown_frames = 55;
                enemy->flash_frames = 8;
                beep(BUZZER_HIT_FREQ_HZ);
            }
        }

        clamp_enemy_position(enemy);
        if (enemy->flash_frames > 0) {
            enemy->flash_frames--;
        }
    }
}

static void fire_boss_shot(PongEngine_t* engine)
{
    uint8_t i = 0;
    uint8_t shots_to_fire = engine->boss_phase >= 2 ? 2 : 1;
    uint8_t fired = 0;

    for (i = 0; i < SIGNAL_MAX_BOSS_SHOTS && fired < shots_to_fire; i++) {
        SignalProjectile_t* shot = &engine->boss_shots[i];
        if (shot->active) {
            continue;
        }

        int16_t dx = engine->player_x - engine->boss_x;
        int16_t dy = engine->player_y - engine->boss_y;

        shot->active = 1;
        shot->damage = 1;
        shot->size = BOSS_SHOT_SIZE;
        shot->x = (int16_t)(engine->boss_x + BOSS_SIZE / 2 - BOSS_SHOT_SIZE / 2);
        shot->y = (int16_t)(engine->boss_y + BOSS_SIZE / 2 - BOSS_SHOT_SIZE / 2);

        if (abs_i16(dx) > abs_i16(dy)) {
            shot->dx = dx > 0 ? 4 : -4;
            shot->dy = dy > 0 ? (int8_t)(2 + fired) : (int8_t)(-2 - fired);
        } else {
            shot->dx = dx > 0 ? (int8_t)(2 + fired) : (int8_t)(-2 - fired);
            shot->dy = dy > 0 ? 4 : -4;
        }
        fired++;
    }

    if (fired > 0) {
        beep(BUZZER_HIT_FREQ_HZ);
    }
}

static void boss_summon_minion(PongEngine_t* engine)
{
    uint8_t i = 0;

    for (i = 0; i < SIGNAL_MAX_ENEMIES; i++) {
        SignalEnemy_t* enemy = &engine->enemies[i];
        int16_t offset_x = (Random_U16(2) == 0) ? -24 : 36;
        int16_t offset_y = (Random_U16(2) == 0) ? -18 : 26;

        if (enemy->active) {
            continue;
        }

        enemy->active = 1;
        enemy->type = engine->boss_tier == 2 ? ENEMY_TYPE_SNIPER : ENEMY_TYPE_CHASER;
        enemy->elite = engine->boss_tier == 2 ? 1 : 0;
        enemy->hp = enemy->elite ? 3 : 1;
        enemy->speed = enemy->elite ? 3 : 2;
        enemy->flash_frames = 10;
        enemy->fire_cooldown_frames = engine->boss_tier == 2 ? 50 : 0;
        enemy->x = (int16_t)(engine->boss_x + offset_x);
        enemy->y = (int16_t)(engine->boss_y + offset_y);
        clamp_enemy_position(enemy);
        spawn_particles(engine, enemy->x + ENEMY_SIZE / 2, enemy->y + ENEMY_SIZE / 2,
                        enemy->elite ? 5 : 2);
        return;
    }
}

static void start_boss_charge(PongEngine_t* engine)
{
    int16_t dx = (int16_t)(engine->player_x - engine->boss_x);
    int16_t dy = (int16_t)(engine->player_y - engine->boss_y);

    engine->boss_charge_dx = 0;
    engine->boss_charge_dy = 0;
    if (abs_i16(dx) >= abs_i16(dy)) {
        engine->boss_charge_dx = dx >= 0 ? 5 : -5;
        engine->boss_charge_dy = dy >= 0 ? 2 : -2;
    } else {
        engine->boss_charge_dx = dx >= 0 ? 2 : -2;
        engine->boss_charge_dy = dy >= 0 ? 5 : -5;
    }
    if (engine->boss_tier == 2) {
        engine->boss_charge_dx = (int8_t)(engine->boss_charge_dx + (engine->boss_charge_dx > 0 ? 1 : -1));
        engine->boss_charge_dy = (int8_t)(engine->boss_charge_dy + (engine->boss_charge_dy > 0 ? 1 : -1));
    }
    engine->boss_charge_frames = BOSS_CHARGE_FRAMES;
    engine->boss_charge_cooldown_frames = engine->boss_tier == 2 ? 70 : 95;
    engine->boss_flash_frames = 8;
    beep(BUZZER_BOSS_FREQ_HZ);
}

static void update_boss(PongEngine_t* engine)
{
    if (!engine->boss_active) {
        return;
    }

    if (engine->boss_phase >= 2 && engine->boss_charge_frames > 0) {
        engine->boss_charge_frames--;
        engine->boss_x = (int16_t)(engine->boss_x + engine->boss_charge_dx);
        engine->boss_y = (int16_t)(engine->boss_y + engine->boss_charge_dy);
    } else {
        engine->boss_move_frames++;
    }

    if (engine->boss_charge_frames == 0 &&
        engine->boss_move_frames >= (engine->boss_tier == 2 || engine->boss_phase >= 2 ? 1 : 2)) {
        engine->boss_move_frames = 0;
        if (engine->boss_x < engine->player_x) {
            engine->boss_x = (int16_t)(engine->boss_x + (engine->boss_phase >= 2 ? 2 : 1));
        } else if (engine->boss_x > engine->player_x) {
            engine->boss_x = (int16_t)(engine->boss_x - (engine->boss_phase >= 2 ? 2 : 1));
        }

        if (engine->boss_y < engine->player_y) {
            engine->boss_y = (int16_t)(engine->boss_y + (engine->boss_phase >= 2 ? 2 : 1));
        } else if (engine->boss_y > engine->player_y) {
            engine->boss_y = (int16_t)(engine->boss_y - (engine->boss_phase >= 2 ? 2 : 1));
        }
    }

    if (engine->boss_x < 0) {
        engine->boss_x = 0;
    }
    if (engine->boss_y < ARENA_TOP) {
        engine->boss_y = ARENA_TOP;
    }
    if (engine->boss_x + BOSS_SIZE > SCREEN_WIDTH) {
        engine->boss_x = SCREEN_WIDTH - BOSS_SIZE;
    }
    if (engine->boss_y + BOSS_SIZE > ARENA_BOTTOM) {
        engine->boss_y = ARENA_BOTTOM - BOSS_SIZE;
    }

    if (engine->boss_fire_cooldown_frames > 0) {
        engine->boss_fire_cooldown_frames--;
    } else {
        fire_boss_shot(engine);
        engine->boss_fire_cooldown_frames = engine->boss_phase >= 2
            ? (engine->boss_tier == 2 ? 24 : 36)
            : (engine->boss_tier == 2 ? 45 : 70);
    }

    if (engine->boss_phase >= 2) {
        if (engine->boss_charge_cooldown_frames > 0) {
            engine->boss_charge_cooldown_frames--;
        } else {
            start_boss_charge(engine);
        }

        if (engine->boss_summon_cooldown_frames > 0) {
            engine->boss_summon_cooldown_frames--;
        } else if (enemy_count(engine) < (engine->boss_tier == 2 ? 5 : 4)) {
            boss_summon_minion(engine);
            engine->boss_summon_cooldown_frames = engine->boss_tier == 2 ? 85 : 120;
        }
    }

    if (engine->boss_flash_frames > 0) {
        engine->boss_flash_frames--;
    }
    if (engine->boss_title_frames > 0) {
        engine->boss_title_frames--;
    }
    if (engine->boss_phase_title_frames > 0) {
        engine->boss_phase_title_frames--;
    }
}

static void update_boss_shots(PongEngine_t* engine)
{
    uint8_t i = 0;

    for (i = 0; i < SIGNAL_MAX_BOSS_SHOTS; i++) {
        SignalProjectile_t* shot = &engine->boss_shots[i];
        if (!shot->active) {
            continue;
        }

        shot->x += shot->dx;
        shot->y += shot->dy;

        if (shot->x < 0 || shot->x > SCREEN_WIDTH || shot->y < ARENA_TOP || shot->y > ARENA_BOTTOM) {
            shot->active = 0;
        }
    }
}

static void update_projectiles(PongEngine_t* engine)
{
    uint8_t i = 0;
    uint8_t j = 0;

    for (i = 0; i < SIGNAL_MAX_PROJECTILES; i++) {
        SignalProjectile_t* shot = &engine->projectiles[i];
        if (!shot->active) {
            continue;
        }

        shot->x += shot->dx;
        shot->y += shot->dy;

        if (shot->x < 0 || shot->x > SCREEN_WIDTH || shot->y < ARENA_TOP || shot->y > ARENA_BOTTOM) {
            shot->active = 0;
            continue;
        }

        for (j = 0; j < SIGNAL_MAX_ENEMIES; j++) {
            SignalEnemy_t* enemy = &engine->enemies[j];
            if (!enemy->active) {
                continue;
            }

            uint8_t shot_size = shot->size != 0 ? shot->size : SHOT_SIZE;
            if (rects_overlap(shot->x, shot->y, shot_size, shot_size, enemy->x, enemy->y, ENEMY_SIZE, ENEMY_SIZE)) {
                shot->active = 0;
                enemy->flash_frames = 5;
                if (enemy->hp > shot->damage) {
                    enemy->hp = (uint8_t)(enemy->hp - shot->damage);
                } else {
                    enemy->hp = 0;
                }
                if (enemy->hp == 0) {
                    int16_t drop_x = enemy->x;
                    int16_t drop_y = enemy->y;
                    uint8_t was_elite = enemy->elite;
                    enemy->active = 0;
                    enemy->elite = 0;
                    award_enemy_kill(engine, was_elite);
                    if (was_elite && engine->elite_kills < 255) {
                        engine->elite_kills++;
                    }
                    engine->hit_effect_frames = 6;
                    if (engine->lives < engine->max_lives && (was_elite || Random_U16(100) < 18)) {
                        spawn_heal_drop(engine, drop_x, drop_y);
                    }
                    spawn_particles(engine, drop_x + ENEMY_SIZE / 2, drop_y + ENEMY_SIZE / 2, was_elite ? 5 : 6);
                    beep(BUZZER_KILL_FREQ_HZ);
                }
                break;
            }
        }

        if (shot->active && engine->boss_active &&
            rects_overlap(shot->x, shot->y, shot->size != 0 ? shot->size : SHOT_SIZE, shot->size != 0 ? shot->size : SHOT_SIZE,
                          engine->boss_x, engine->boss_y, BOSS_SIZE, BOSS_SIZE)) {
            shot->active = 0;
            damage_boss(engine, shot->damage);
        }
    }
}

void PongEngine_Init(PongEngine_t* engine,
                     int16_t paddle_x, int16_t paddle_y,
                     int16_t paddle_width, int16_t paddle_height,
                     int16_t ball_size, float ball_speed)
{
    uint8_t i = 0;
    (void)paddle_x;
    (void)paddle_y;
    (void)paddle_width;
    (void)paddle_height;
    (void)ball_size;
    (void)ball_speed;

    engine->player_size = PLAYER_SIZE;
    engine->player_speed = 3;
    engine->max_lives = MAX_LIVES;
    engine->lives = engine->max_lives;
    engine->score = 0;
    engine->time_left_s = GAME_SECONDS;
    engine->wave = 1;
    engine->first_boss_defeated = 0;
    engine->elite_kills = 0;
    engine->elite_boss_spawned = 0;
    engine->signal_x = 110;
    engine->signal_y = 120;
    engine->heal_active = 0;
    engine->heal_x = 0;
    engine->heal_y = 0;
    engine->chest_active = 0;
    engine->chest_x = 0;
    engine->chest_y = 0;
    engine->victory_animation_frames = 0;
    engine->victory_complete = 0;
    engine->signals_collected = 0;
    engine->total_signals_collected = 0;
    engine->weapon_level = 1;
    engine->fire_rate_level = 0;
    engine->bullet_size_level = 0;
    engine->vitality_level = 0;
    engine->upgrade_choice_active = 0;
    engine->upgrade_choice_index = 0;
    engine->upgrade_choice_message_frames = 0;
    engine->ultimate_charges = 0;
    engine->signal_decode_active = 0;
    engine->decode_index = 0;
    engine->last_decode_dir = CENTRE;
    engine->last_input_dir = CENTRE;
    engine->upgrade_message_frames = 0;
    engine->ultimate_ready_message_frames = 0;
    engine->ultimate_flash_frames = 0;
    engine->boss_x = 0;
    engine->boss_y = 0;
    engine->boss_active = 0;
    engine->boss_spawned = 0;
    engine->boss_tier = 0;
    engine->boss_hp = 0;
    engine->boss_max_hp = FIRST_BOSS_MAX_HP;
    engine->boss_flash_frames = 0;
    engine->boss_move_frames = 0;
    engine->boss_fire_cooldown_frames = 0;
    engine->boss_title_frames = 0;
    engine->boss_warning_frames = 0;
    engine->boss_pending_tier = 0;
    engine->boss_phase = 0;
    engine->boss_phase_title_frames = 0;
    engine->boss_charge_frames = 0;
    engine->boss_charge_cooldown_frames = 0;
    engine->boss_charge_dx = 0;
    engine->boss_charge_dy = 0;
    engine->boss_summon_cooldown_frames = 0;
    engine->ultimate_freeze_frames = 0;
    engine->dash_frames = 0;
    engine->dash_cooldown_frames = 0;
    engine->dash_evade_message_frames = 0;
    engine->dash_dir = E;
    engine->aim_dir = CENTRE;
    engine->aim_indicator_frames = 0;
    engine->charge_frames = 0;
    engine->charge_dir = E;
    engine->trap_cooldown_frames = 0;
    engine->trap_message_frames = 0;
    engine->combo_kills = 0;
    engine->combo_multiplier = 1;
    engine->combo_message_frames = 0;
    engine->combo_timer_frames = 0;
    engine->melee_frames = 0;
    engine->melee_cooldown_frames = 0;
    engine->melee_dir = E;
    engine->fire_cooldown_frames = 20;
    engine->spawn_cooldown_frames = 1;
    engine->invulnerable_frames = 0;
    engine->hit_effect_frames = 0;

    for (i = 0; i < SIGNAL_MAX_ENEMIES; i++) {
        engine->enemies[i].active = 0;
        engine->enemies[i].elite = 0;
        engine->enemies[i].type = ENEMY_TYPE_CHASER;
        engine->enemies[i].fire_cooldown_frames = 0;
    }
    for (i = 0; i < SIGNAL_MAX_PROJECTILES; i++) {
        engine->projectiles[i].active = 0;
        engine->projectiles[i].damage = 1;
        engine->projectiles[i].size = SHOT_SIZE;
    }
    for (i = 0; i < SIGNAL_MAX_BOSS_SHOTS; i++) {
        engine->boss_shots[i].active = 0;
        engine->boss_shots[i].size = BOSS_SHOT_SIZE;
    }
    for (i = 0; i < SIGNAL_MAX_PARTICLES; i++) {
        engine->particles[i].life = 0;
    }
    for (i = 0; i < SIGNAL_MAX_TRAPS; i++) {
        engine->traps[i].active = 0;
        engine->traps[i].armed_frames = 0;
        engine->traps[i].pulse_frames = 0;
    }

    reset_player(engine);
    place_signal(engine);
    game_timer_last_tick = HAL_GetTick();
    buzzer_stop_tick = 0;
    buzzer_off(&buzzer_cfg);
}

void PongEngine_UseUltimate(PongEngine_t* engine)
{
    static const int8_t dirs[ULTIMATE_SHOT_COUNT][2] = {
        { 6,  0}, { 5,  2}, { 4,  4}, { 2,  5},
        { 0,  6}, {-2,  5}, {-4,  4}, {-5,  2},
        {-6,  0}, {-5, -2}, {-4, -4}, {-2, -5},
        { 0, -6}, { 2, -5}, { 4, -4}, { 5, -2}
    };
    uint8_t i = 0;

    if (engine->lives == 0 || engine->ultimate_charges == 0 || engine->signal_decode_active) {
        return;
    }

    engine->ultimate_charges--;
    for (i = 0; i < SIGNAL_MAX_PROJECTILES && i < ULTIMATE_SHOT_COUNT; i++) {
        SignalProjectile_t* shot = &engine->projectiles[i];
        shot->active = 1;
        shot->damage = ULTIMATE_DAMAGE;
        shot->size = player_shot_size(engine);
        shot->x = (int16_t)(engine->player_x + PLAYER_SIZE / 2 - shot->size / 2);
        shot->y = (int16_t)(engine->player_y + PLAYER_SIZE / 2 - shot->size / 2);
        shot->dx = dirs[i][0];
        shot->dy = dirs[i][1];
    }

    engine->ultimate_ready_message_frames = engine->ultimate_charges > 0 ? 90 : 0;
    engine->ultimate_flash_frames = 18;
    engine->ultimate_freeze_frames = ULTIMATE_FREEZE_FRAMES;
    engine->hit_effect_frames = 8;
    beep(BUZZER_UPGRADE_FREQ_HZ);
}

void PongEngine_HandleSecondStickButton(PongEngine_t* engine,
                                        UserInput aim_input,
                                        uint8_t button_down,
                                        uint8_t button_pressed)
{
    if (engine->lives == 0 || engine->victory_animation_frames > 0 || engine->victory_complete) {
        engine->charge_frames = 0;
        return;
    }

    if (engine->upgrade_choice_active) {
        if (aim_input.direction == W || aim_input.direction == NW || aim_input.direction == SW) {
            engine->upgrade_choice_index = 0;
        } else if (aim_input.direction == E || aim_input.direction == NE || aim_input.direction == SE) {
            engine->upgrade_choice_index = 2;
        } else if (aim_input.direction == N || aim_input.direction == S) {
            engine->upgrade_choice_index = 1;
        }
        if (button_pressed) {
            apply_upgrade_choice(engine);
        }
        engine->charge_frames = 0;
        return;
    }

    if (button_pressed) {
        (void)aim_input;
        (void)button_down;
        place_signal_trap(engine);
        engine->charge_frames = 0;
    }
}

void PongEngine_MeleeAttack(PongEngine_t* engine)
{
    int16_t slash_x = engine->player_x - 6;
    int16_t slash_y = engine->player_y - 6;
    int16_t slash_w = PLAYER_SIZE + 12;
    int16_t slash_h = PLAYER_SIZE + 12;
    Direction attack_dir = engine->last_input_dir;
    uint8_t i = 0;
    uint8_t hit = 0;

    if (engine->lives == 0 || engine->signal_decode_active || engine->melee_cooldown_frames > 0) {
        return;
    }

    if (attack_dir == CENTRE) {
        attack_dir = engine->melee_dir;
    }
    if (attack_dir == CENTRE) {
        attack_dir = E;
    }
    engine->melee_dir = attack_dir;
    engine->melee_frames = MELEE_FRAMES;
    engine->melee_cooldown_frames = MELEE_COOLDOWN_FRAMES;

    if (attack_dir == N || attack_dir == NE || attack_dir == NW) {
        slash_y = (int16_t)(engine->player_y - 20);
        slash_h = 24;
    }
    if (attack_dir == S || attack_dir == SE || attack_dir == SW) {
        slash_y = (int16_t)(engine->player_y + PLAYER_SIZE - 4);
        slash_h = 24;
    }
    if (attack_dir == W || attack_dir == NW || attack_dir == SW) {
        slash_x = (int16_t)(engine->player_x - 20);
        slash_w = 24;
    }
    if (attack_dir == E || attack_dir == NE || attack_dir == SE) {
        slash_x = (int16_t)(engine->player_x + PLAYER_SIZE - 4);
        slash_w = 24;
    }

    for (i = 0; i < SIGNAL_MAX_ENEMIES; i++) {
        SignalEnemy_t* enemy = &engine->enemies[i];
        if (!enemy->active) {
            continue;
        }
        if (rects_overlap(slash_x, slash_y, slash_w, slash_h, enemy->x, enemy->y, ENEMY_SIZE, ENEMY_SIZE)) {
            uint8_t was_elite = enemy->elite;
            enemy->flash_frames = 5;
            hit = 1;
            if (enemy->hp > MELEE_DAMAGE) {
                enemy->hp = (uint8_t)(enemy->hp - MELEE_DAMAGE);
            } else {
                int16_t drop_x = enemy->x;
                int16_t drop_y = enemy->y;
                enemy->hp = 0;
                enemy->active = 0;
                enemy->elite = 0;
                award_enemy_kill(engine, was_elite);
                if (was_elite && engine->elite_kills < 255) {
                    engine->elite_kills++;
                }
                if (engine->lives < engine->max_lives && (was_elite || Random_U16(100) < 18)) {
                    spawn_heal_drop(engine, drop_x, drop_y);
                }
                spawn_particles(engine, drop_x + ENEMY_SIZE / 2, drop_y + ENEMY_SIZE / 2, was_elite ? 5 : 6);
            }
        }
    }

    if (engine->boss_active &&
        rects_overlap(slash_x, slash_y, slash_w, slash_h,
                      engine->boss_x, engine->boss_y, BOSS_SIZE, BOSS_SIZE)) {
        damage_boss(engine, MELEE_BOSS_DAMAGE);
        hit = 1;
    }

    beep(hit ? BUZZER_KILL_FREQ_HZ : BUZZER_SHOT_FREQ_HZ);
}

uint8_t PongEngine_UpdateDual(PongEngine_t* engine,
                              UserInput input,
                              UserInput aim_input,
                              uint8_t dash_pressed)
{
    int16_t dx = 0;
    int16_t dy = 0;
    uint8_t i = 0;
    uint8_t aim_active = (aim_input.direction != CENTRE && aim_input.magnitude > AIM_ASSIST_MIN_MAGNITUDE);

    if (engine->lives == 0) {
        update_buzzer();
        return 0;
    }
    if (engine->victory_complete) {
        update_buzzer();
        return engine->lives;
    }
    if (engine->victory_animation_frames > 0) {
        engine->victory_animation_frames--;
        if ((engine->victory_animation_frames % 12U) == 0U) {
            spawn_particles(engine,
                            (int16_t)(engine->player_x + PLAYER_SIZE / 2),
                            (int16_t)(engine->player_y + PLAYER_SIZE / 2),
                            engine->victory_animation_frames > 45 ? 6 : 1);
        }
        if (engine->victory_animation_frames == 0) {
            engine->victory_complete = 1;
            beep(BUZZER_UPGRADE_FREQ_HZ);
        }
        update_particles(engine);
        update_buzzer();
        return engine->lives;
    }

    if (engine->upgrade_choice_active) {
        if (aim_input.direction == W || aim_input.direction == NW || aim_input.direction == SW) {
            engine->upgrade_choice_index = 0;
        } else if (aim_input.direction == E || aim_input.direction == NE || aim_input.direction == SE) {
            engine->upgrade_choice_index = 2;
        } else if (aim_input.direction == N || aim_input.direction == S) {
            engine->upgrade_choice_index = 1;
        }
        engine->aim_dir = aim_input.direction != CENTRE ? aim_input.direction : engine->aim_dir;
        update_particles(engine);
        update_buzzer();
        return engine->lives;
    }

    {
        uint32_t now = HAL_GetTick();
        if ((now - game_timer_last_tick) > 1500U) {
            game_timer_last_tick = now;
        }
        while ((now - game_timer_last_tick) >= 1000U) {
            game_timer_last_tick += 1000U;
            if (engine->time_left_s > 0) {
                engine->time_left_s--;
            }
        }
    }

    engine->wave = (uint8_t)(1 + (engine->score / 6));
    if (engine->wave > 6) {
        engine->wave = 6;
    }
    if (aim_active) {
        engine->aim_dir = aim_input.direction;
        engine->aim_indicator_frames = 10;
    }

    if (engine->signal_decode_active) {
        Direction decode_direction = aim_input.direction;

        if (is_decode_direction(decode_direction) && decode_direction != engine->last_decode_dir) {
            if (decode_direction == engine->decode_sequence[engine->decode_index]) {
                engine->decode_index++;
                beep(BUZZER_SIGNAL_FREQ_HZ);
                if (engine->decode_index >= 3) {
                    finish_signal_decode(engine);
                }
            } else {
                engine->decode_index = 0;
                beep(BUZZER_HIT_FREQ_HZ);
            }
        }
        engine->last_decode_dir = decode_direction;
    } else {
        if (dash_pressed) {
            Direction dash_direction = aim_input.direction != CENTRE ? aim_input.direction : input.direction;
            if (dash_direction == CENTRE) {
                dash_direction = engine->last_input_dir;
            }
            if (dash_direction != CENTRE) {
                start_shadow_dash(engine, dash_direction);
            }
        }

        if (input.direction != CENTRE &&
            engine->last_input_dir == CENTRE &&
            input.magnitude >= SHADOW_DASH_MIN_MAGNITUDE) {
            start_shadow_dash(engine, input.direction);
        }

        if (engine->dash_frames > 0) {
            direction_to_delta(engine->dash_dir, SHADOW_DASH_SPEED, &dx, &dy);
        } else {
            int16_t move_speed = jammer_near_player(engine) ? 2 : engine->player_speed;
            direction_to_delta(input.direction, move_speed, &dx, &dy);
        }

        engine->player_x += dx;
        engine->player_y += dy;
        clamp_player_position(engine);
    }

    if (engine->chest_active) {
        engine->spawn_cooldown_frames = 60;
    } else if (engine->spawn_cooldown_frames > 0) {
        engine->spawn_cooldown_frames--;
    } else {
        if (enemy_count(engine) < (uint8_t)(2 + engine->wave / 2)) {
            spawn_enemy(engine);
        }
        engine->spawn_cooldown_frames = (uint16_t)(80 - (engine->wave * 8));
        if (engine->spawn_cooldown_frames < 28) {
            engine->spawn_cooldown_frames = 28;
        }
    }

    if (!engine->signal_decode_active) {
        if (engine->fire_cooldown_frames > 0) {
            engine->fire_cooldown_frames--;
        } else {
            fire_projectile(engine, aim_input);
            engine->fire_cooldown_frames = (uint16_t)(24 - engine->wave - (engine->weapon_level * 2));
            if (engine->fire_rate_level > 0 && engine->fire_cooldown_frames > engine->fire_rate_level * 3U) {
                engine->fire_cooldown_frames = (uint16_t)(engine->fire_cooldown_frames - engine->fire_rate_level * 3U);
            }
            if (aim_active && engine->fire_cooldown_frames > AIM_FIRE_COOLDOWN_BONUS) {
                engine->fire_cooldown_frames = (uint16_t)(engine->fire_cooldown_frames - AIM_FIRE_COOLDOWN_BONUS);
            }
            if (engine->fire_cooldown_frames < (aim_active ? 5 : 9)) {
                engine->fire_cooldown_frames = aim_active ? 5 : 9;
            }
        }
    }

    if (engine->boss_warning_frames > 0) {
        engine->boss_warning_frames--;
        if (engine->boss_warning_frames == 0 && engine->boss_pending_tier != 0) {
            spawn_boss(engine, engine->boss_pending_tier);
            engine->boss_pending_tier = 0;
        }
    }

    if (engine->ultimate_freeze_frames > 0) {
        engine->ultimate_freeze_frames--;
    } else {
        update_enemies(engine);
        update_boss(engine);
        update_boss_shots(engine);
    }

    if (engine->score >= 10 && !engine->boss_spawned) {
        start_boss_warning(engine, 1);
        engine->boss_spawned = 1;
    }
    if (engine->first_boss_defeated && engine->elite_kills >= 5 &&
        !engine->elite_boss_spawned && !engine->boss_active && engine->boss_warning_frames == 0) {
        start_boss_warning(engine, 2);
        engine->elite_boss_spawned = 1;
    }
    update_projectiles(engine);
    update_signal_traps(engine);
    update_particles(engine);

    if (!engine->signal_decode_active &&
        rects_overlap(engine->player_x, engine->player_y, PLAYER_SIZE, PLAYER_SIZE,
                      engine->signal_x, engine->signal_y, SIGNAL_SIZE, SIGNAL_SIZE)) {
        start_signal_decode(engine);
    }

    if (engine->heal_active &&
        rects_overlap(engine->player_x, engine->player_y, PLAYER_SIZE, PLAYER_SIZE,
                      engine->heal_x, engine->heal_y, HEAL_SIZE, HEAL_SIZE)) {
        engine->heal_active = 0;
        if (engine->lives < engine->max_lives) {
            engine->lives++;
        }
        beep(BUZZER_SIGNAL_FREQ_HZ);
    }
    if (engine->chest_active &&
        rects_overlap(engine->player_x, engine->player_y, PLAYER_SIZE, PLAYER_SIZE,
                      engine->chest_x, engine->chest_y, CHEST_SIZE, CHEST_SIZE)) {
        start_victory_animation(engine);
    }

    if (engine->invulnerable_frames > 0) {
        engine->invulnerable_frames--;
    } else {
        for (i = 0; i < SIGNAL_MAX_ENEMIES; i++) {
            SignalEnemy_t* enemy = &engine->enemies[i];
            if (enemy->active &&
                rects_overlap(engine->player_x, engine->player_y, PLAYER_SIZE, PLAYER_SIZE,
                              enemy->x, enemy->y, ENEMY_SIZE, ENEMY_SIZE)) {
                start_player_hit_feedback(engine);
                knock_enemy_away_from_player(engine, enemy);
                break;
            }
        }

        if (engine->invulnerable_frames == 0 && engine->boss_active &&
            rects_overlap(engine->player_x, engine->player_y, PLAYER_SIZE, PLAYER_SIZE,
                          engine->boss_x, engine->boss_y, BOSS_SIZE, BOSS_SIZE)) {
            start_player_hit_feedback(engine);
            if (engine->player_x < engine->boss_x) {
                engine->player_x = (int16_t)(engine->player_x - 10);
            } else {
                engine->player_x = (int16_t)(engine->player_x + 10);
            }
            if (engine->player_y < engine->boss_y) {
                engine->player_y = (int16_t)(engine->player_y - 10);
            } else {
                engine->player_y = (int16_t)(engine->player_y + 10);
            }
            clamp_player_position(engine);
        }

        for (i = 0; i < SIGNAL_MAX_BOSS_SHOTS && engine->invulnerable_frames == 0; i++) {
            SignalProjectile_t* shot = &engine->boss_shots[i];
            if (shot->active &&
                rects_overlap(engine->player_x, engine->player_y, PLAYER_SIZE, PLAYER_SIZE,
                              shot->x, shot->y, BOSS_SHOT_SIZE, BOSS_SHOT_SIZE)) {
                shot->active = 0;
                start_player_hit_feedback(engine);
            }
        }
    }

    if (engine->hit_effect_frames > 0) {
        engine->hit_effect_frames--;
    }
    if (engine->upgrade_message_frames > 0) {
        engine->upgrade_message_frames--;
    }
    if (engine->upgrade_choice_message_frames > 0) {
        engine->upgrade_choice_message_frames--;
    }
    if (engine->ultimate_ready_message_frames > 0) {
        engine->ultimate_ready_message_frames--;
    }
    if (engine->ultimate_flash_frames > 0) {
        engine->ultimate_flash_frames--;
    }
    if (engine->dash_evade_message_frames > 0) {
        engine->dash_evade_message_frames--;
    }
    if (engine->dash_frames > 0) {
        engine->dash_frames--;
    }
    if (engine->dash_cooldown_frames > 0) {
        engine->dash_cooldown_frames--;
    }
    if (engine->aim_indicator_frames > 0) {
        engine->aim_indicator_frames--;
    }
    if (engine->combo_timer_frames > 0) {
        engine->combo_timer_frames--;
        if (engine->combo_timer_frames == 0) {
            reset_combo(engine);
        }
    }
    if (engine->combo_message_frames > 0) {
        engine->combo_message_frames--;
    }
    if (engine->melee_frames > 0) {
        engine->melee_frames--;
    }
    if (engine->melee_cooldown_frames > 0) {
        engine->melee_cooldown_frames--;
    }
    if (engine->boss_warning_frames > 0) {
        engine->hit_effect_frames = engine->hit_effect_frames > 0 ? engine->hit_effect_frames : 1;
    }

    engine->last_input_dir = input.direction;
    update_buzzer();
    return engine->lives;
}

uint8_t PongEngine_Update(PongEngine_t* engine, UserInput input)
{
    UserInput aim_input;

    aim_input.direction = CENTRE;
    aim_input.magnitude = 0.0f;
    aim_input.angle = -1.0f;

    return PongEngine_UpdateDual(engine, input, aim_input, 0);
}

void PongEngine_Draw(PongEngine_t* engine)
{
    char hud[40];
    char decode_text[12];
    uint8_t i = 0;

    LCD_Draw_Rect(0, ARENA_TOP, SCREEN_WIDTH, SCREEN_HEIGHT - ARENA_TOP, 13, 0);

    for (i = 0; i < 8; i++) {
        uint16_t x = (uint16_t)(i * 32);
        LCD_Draw_Line(x, ARENA_TOP, x, SCREEN_HEIGHT, 9);
        LCD_Draw_Line(0, (uint16_t)(ARENA_TOP + i * 28), SCREEN_WIDTH, (uint16_t)(ARENA_TOP + i * 28), 9);
    }

    if (engine->hit_effect_frames > 0) {
        uint16_t cx = (uint16_t)(engine->player_x + PLAYER_SIZE / 2);
        uint16_t cy = (uint16_t)(engine->player_y + PLAYER_SIZE / 2);
        uint16_t pulse = (uint16_t)(6 + (HIT_FLASH_FRAMES - engine->hit_effect_frames) * 2);
        uint8_t flash_colour = (engine->hit_effect_frames % 4U) < 2U ? 2 : 6;

        LCD_Draw_Rect(0, ARENA_TOP, SCREEN_WIDTH, SCREEN_HEIGHT - ARENA_TOP, 2, 0);
        LCD_Draw_Rect(2, ARENA_TOP + 2, SCREEN_WIDTH - 4, SCREEN_HEIGHT - ARENA_TOP - 4, flash_colour, 0);
        LCD_Draw_Rect(4, ARENA_TOP + 4, SCREEN_WIDTH - 8, SCREEN_HEIGHT - ARENA_TOP - 8, 2, 0);

        LCD_Draw_Circle(cx, cy, pulse, flash_colour, 0);
        LCD_Draw_Circle(cx, cy, (uint16_t)(pulse + 6), 2, 0);
        LCD_Draw_Line(cx, cy, (uint16_t)(cx > 44 ? cx - 44 : 0), cy, flash_colour);
        LCD_Draw_Line(cx, cy, (uint16_t)(cx + 44 < SCREEN_WIDTH ? cx + 44 : SCREEN_WIDTH - 1), cy, flash_colour);
        LCD_Draw_Line(cx, cy, cx, (uint16_t)(cy > 44 ? cy - 44 : ARENA_TOP), flash_colour);
        LCD_Draw_Line(cx, cy, cx, (uint16_t)(cy + 44 < SCREEN_HEIGHT ? cy + 44 : SCREEN_HEIGHT - 1), flash_colour);

        if ((engine->hit_effect_frames % 3U) == 0U) {
            LCD_Draw_Line((uint16_t)(cx > 30 ? cx - 30 : 0),
                          (uint16_t)(cy > 18 ? cy - 18 : ARENA_TOP),
                          (uint16_t)(cx + 30 < SCREEN_WIDTH ? cx + 30 : SCREEN_WIDTH - 1),
                          (uint16_t)(cy + 18 < SCREEN_HEIGHT ? cy + 18 : SCREEN_HEIGHT - 1),
                          1);
            LCD_Draw_Line((uint16_t)(cx > 30 ? cx - 30 : 0),
                          (uint16_t)(cy + 18 < SCREEN_HEIGHT ? cy + 18 : SCREEN_HEIGHT - 1),
                          (uint16_t)(cx + 30 < SCREEN_WIDTH ? cx + 30 : SCREEN_WIDTH - 1),
                          (uint16_t)(cy > 18 ? cy - 18 : ARENA_TOP),
                          1);
        }
    }
    if (engine->ultimate_flash_frames > 0) {
        LCD_Draw_Rect(0, ARENA_TOP, SCREEN_WIDTH, SCREEN_HEIGHT - ARENA_TOP, 6, 0);
        {
            uint16_t cx = (uint16_t)(engine->player_x + PLAYER_SIZE / 2);
            uint16_t cy = (uint16_t)(engine->player_y + PLAYER_SIZE / 2);
            uint16_t radius = (uint16_t)(8 + (18 - engine->ultimate_flash_frames) * 4);
            LCD_Draw_Circle(cx, cy, radius, 6, 0);
            LCD_Draw_Circle(cx, cy, (uint16_t)(radius / 2), 1, 0);
            LCD_Draw_Line(cx, cy, 0, cy, 6);
            LCD_Draw_Line(cx, cy, SCREEN_WIDTH - 1, cy, 6);
            LCD_Draw_Line(cx, cy, cx, ARENA_TOP, 6);
            LCD_Draw_Line(cx, cy, cx, SCREEN_HEIGHT - 1, 6);
        }
    }
    if (engine->boss_warning_frames > 0 && (engine->boss_warning_frames / 6) % 2 == 0) {
        LCD_Draw_Rect(0, ARENA_TOP, SCREEN_WIDTH, SCREEN_HEIGHT - ARENA_TOP, 2, 0);
        LCD_Draw_Rect(4, ARENA_TOP + 4, SCREEN_WIDTH - 8, SCREEN_HEIGHT - ARENA_TOP - 8, 2, 0);
        LCD_printString("ENEMY FIREWALL", 18, 88, 2, 2);
        LCD_printString("DETECTED", 56, 116, 6, 2);
    }
    if (engine->lives <= 2 && (HAL_GetTick() / 180U) % 2U == 0U) {
        LCD_Draw_Rect(0, ARENA_TOP, SCREEN_WIDTH, SCREEN_HEIGHT - ARENA_TOP, 2, 0);
        LCD_Draw_Rect(2, ARENA_TOP + 2, SCREEN_WIDTH - 4, SCREEN_HEIGHT - ARENA_TOP - 4, 2, 0);
    }

    for (i = 0; i < SIGNAL_MAX_PROJECTILES; i++) {
        SignalProjectile_t* shot = &engine->projectiles[i];
        if (shot->active) {
            uint8_t shot_size = shot->size != 0 ? shot->size : SHOT_SIZE;
            if (shot->damage >= 3) {
                LCD_Draw_Circle((uint16_t)(shot->x + shot_size / 2),
                                (uint16_t)(shot->y + shot_size / 2),
                                (uint16_t)(shot->damage + 2), 6, 0);
                LCD_Draw_Rect((uint16_t)shot->x, (uint16_t)shot->y,
                              (uint16_t)(shot_size + 2), (uint16_t)(shot_size + 2), 1, 1);
            } else {
                LCD_Draw_Rect((uint16_t)shot->x, (uint16_t)shot->y, shot_size, shot_size,
                              shot->damage > 1 ? 1 : 6, 1);
            }
        }
    }

    for (i = 0; i < SIGNAL_MAX_BOSS_SHOTS; i++) {
        SignalProjectile_t* shot = &engine->boss_shots[i];
        if (shot->active) {
            LCD_Draw_Rect((uint16_t)shot->x, (uint16_t)shot->y, BOSS_SHOT_SIZE, BOSS_SHOT_SIZE, 2, 1);
        }
    }

    for (i = 0; i < SIGNAL_MAX_TRAPS; i++) {
        SignalTrap_t* trap = &engine->traps[i];
        if (trap->active) {
            uint16_t cx = (uint16_t)(trap->x + SIGNAL_TRAP_SIZE / 2);
            uint16_t cy = (uint16_t)(trap->y + SIGNAL_TRAP_SIZE / 2);
            uint8_t pulse = (uint8_t)((HAL_GetTick() / 120U) % 4U);
            LCD_Draw_Rect((uint16_t)trap->x, (uint16_t)trap->y,
                          SIGNAL_TRAP_SIZE, SIGNAL_TRAP_SIZE, 6, 0);
            LCD_Draw_Circle(cx, cy, (uint16_t)(10 + pulse), 1, 0);
            LCD_Draw_Circle(cx, cy, SIGNAL_TRAP_RADIUS, 6, 0);
        }
        if (trap->pulse_frames > 0) {
            uint16_t cx = (uint16_t)(trap->x + SIGNAL_TRAP_SIZE / 2);
            uint16_t cy = (uint16_t)(trap->y + SIGNAL_TRAP_SIZE / 2);
            LCD_Draw_Circle(cx, cy,
                            (uint16_t)(SIGNAL_TRAP_RADIUS + (SIGNAL_TRAP_PULSE_FRAMES - trap->pulse_frames)),
                            6, 0);
        }
    }

    draw_signal_waveform((uint16_t)engine->signal_x, (uint16_t)engine->signal_y);
    if (engine->heal_active) {
        LCD_Draw_Sprite((uint16_t)engine->heal_x, (uint16_t)engine->heal_y, 8, 8, heal_sprite);
    }
    if (engine->chest_active) {
        uint16_t chest_cx = (uint16_t)(engine->chest_x + CHEST_SIZE / 2);
        uint16_t chest_cy = (uint16_t)(engine->chest_y + CHEST_SIZE / 2);
        uint16_t pulse = (uint16_t)(12 + ((HAL_GetTick() / 120U) % 5U));
        LCD_Draw_Circle(chest_cx, chest_cy, pulse, 6, 0);
        LCD_Draw_Circle(chest_cx, chest_cy, (uint16_t)(pulse + 5), 5, 0);
        LCD_Draw_Sprite_Colour_Scaled((uint16_t)engine->chest_x,
                                      (uint16_t)engine->chest_y,
                                      8, 8, chest_sprite, 6, 2);
        LCD_printString("TIME CHEST", (uint16_t)(engine->chest_x > 20 ? engine->chest_x - 20 : 0),
                        (uint16_t)(engine->chest_y > ARENA_TOP + 12 ? engine->chest_y - 12 : ARENA_TOP),
                        6, 1);
    }

    for (i = 0; i < SIGNAL_MAX_PARTICLES; i++) {
        SignalParticle_t* particle = &engine->particles[i];
        if (particle->life != 0) {
            LCD_Draw_Rect((uint16_t)particle->x, (uint16_t)particle->y, 3, 3, particle->colour, 1);
        }
    }

    for (i = 0; i < SIGNAL_MAX_ENEMIES; i++) {
        SignalEnemy_t* enemy = &engine->enemies[i];
        if (enemy->active) {
            uint16_t ex = (uint16_t)enemy->x;
            uint16_t ey = (uint16_t)enemy->y;
            uint16_t ecx = (uint16_t)(enemy->x + ENEMY_SIZE / 2);
            uint16_t ecy = (uint16_t)(enemy->y + ENEMY_SIZE / 2);
            uint8_t scan_colour = enemy->elite ? 5 : (enemy->type == ENEMY_TYPE_SNIPER ? 6 : 2);
            const uint8_t* sprite = enemy_sprite;

            if (enemy->type == ENEMY_TYPE_SNIPER) {
                sprite = sniper_sprite;
            } else if (enemy->type == ENEMY_TYPE_JAMMER) {
                sprite = jammer_sprite;
                scan_colour = 5;
            }

            LCD_Draw_Circle(ecx, ecy, (uint16_t)(9 + ((HAL_GetTick() / 140U) % 3U)), scan_colour, 0);
            LCD_Draw_Line(ex, ecy, (uint16_t)(ex + ENEMY_SIZE - 1), ecy, scan_colour);
            LCD_Draw_Line(ecx, ey, ecx, (uint16_t)(ey + ENEMY_SIZE - 1), scan_colour);
            if (enemy->flash_frames > 0) {
                LCD_Draw_Rect(ex, ey, ENEMY_SIZE, ENEMY_SIZE, 6, 1);
                LCD_Draw_Rect((uint16_t)(ex > 2 ? ex - 2 : 0),
                              (uint16_t)(ey > ARENA_TOP + 2 ? ey - 2 : ARENA_TOP),
                              ENEMY_SIZE + 4, ENEMY_SIZE + 4, 1, 0);
            } else if (enemy->elite) {
                LCD_Draw_Rect((uint16_t)(ex > 1 ? ex - 1 : 0),
                              (uint16_t)(ey > ARENA_TOP + 1 ? ey - 1 : ARENA_TOP),
                              ENEMY_SIZE + 2, ENEMY_SIZE + 2, 5, 0);
                LCD_Draw_Sprite_Colour_Scaled(ex, ey, 8, 8, sprite, 5, 2);
                LCD_Draw_Circle(ecx, ecy, 4, 6, 1);
            } else {
                if (enemy->type == ENEMY_TYPE_CHASER) {
                    LCD_Draw_Sprite_Scaled(ex, ey, 8, 8, sprite, 2);
                } else {
                    LCD_Draw_Sprite_Colour_Scaled(ex, ey, 8, 8, sprite, scan_colour, 2);
                }
                LCD_Draw_Circle(ecx, ecy, 3, 1, 1);
            }
            if (enemy->elite && enemy->y > ARENA_TOP + 9) {
                uint16_t outline_x = (uint16_t)(enemy->x > 0 ? enemy->x - 1 : enemy->x);
                uint16_t outline_y = (uint16_t)(enemy->y > ARENA_TOP ? enemy->y - 1 : enemy->y);
                LCD_Draw_Rect(outline_x, outline_y, ENEMY_SIZE + 2, ENEMY_SIZE + 2, 5, 0);
                LCD_printString("ELITE", (uint16_t)enemy->x, (uint16_t)(enemy->y - 9), 5, 1);
            }
        }
    }

    if (engine->boss_active) {
        uint16_t hp_w = (uint16_t)((engine->boss_hp * 112U) / engine->boss_max_hp);
        LCD_Draw_Rect(62, 30, 116, 8, 1, 0);
        LCD_Draw_Rect(64, 32, hp_w, 4, engine->boss_tier == 2 ? 5 : 2, 1);
        LCD_printString(engine->boss_tier == 2 ? "ELITE BOSS" : "BOSS",
                        engine->boss_tier == 2 ? 74 : 92, 38,
                        engine->boss_tier == 2 ? 5 : 2, 1);
        if (engine->boss_flash_frames > 0) {
            LCD_Draw_Rect((uint16_t)engine->boss_x, (uint16_t)engine->boss_y, BOSS_SIZE, BOSS_SIZE, 6, 1);
        } else if (engine->boss_tier == 2) {
            uint16_t bx = (uint16_t)engine->boss_x;
            uint16_t by = (uint16_t)engine->boss_y;
            uint16_t bc_x = (uint16_t)(engine->boss_x + BOSS_SIZE / 2);
            uint16_t bc_y = (uint16_t)(engine->boss_y + BOSS_SIZE / 2);
            LCD_Draw_Circle(bc_x, bc_y, (uint16_t)(22 + ((HAL_GetTick() / 120U) % 5U)), 5, 0);
            LCD_Draw_Rect((uint16_t)(bx > 2 ? bx - 2 : 0),
                          (uint16_t)(by > ARENA_TOP + 2 ? by - 2 : ARENA_TOP),
                          BOSS_SIZE + 4, BOSS_SIZE + 4, 5, 0);
            LCD_Draw_Sprite_Colour_Scaled((uint16_t)engine->boss_x, (uint16_t)engine->boss_y, 8, 8, boss_sprite, 5, 4);
            LCD_Draw_Circle(bc_x, bc_y, 7, 6, 1);
        } else {
            uint16_t bc_x = (uint16_t)(engine->boss_x + BOSS_SIZE / 2);
            uint16_t bc_y = (uint16_t)(engine->boss_y + BOSS_SIZE / 2);
            LCD_Draw_Circle(bc_x, bc_y, (uint16_t)(20 + ((HAL_GetTick() / 160U) % 4U)), 2, 0);
            LCD_Draw_Sprite_Scaled((uint16_t)engine->boss_x, (uint16_t)engine->boss_y, 8, 8, boss_sprite, 4);
            LCD_Draw_Circle(bc_x, bc_y, 6, 1, 1);
        }
        if (engine->boss_phase >= 2) {
            uint16_t bc_x = (uint16_t)(engine->boss_x + BOSS_SIZE / 2);
            uint16_t bc_y = (uint16_t)(engine->boss_y + BOSS_SIZE / 2);
            uint8_t phase_colour = engine->boss_tier == 2 ? 5 : 2;
            LCD_Draw_Circle(bc_x, bc_y,
                            (uint16_t)(22 + ((HAL_GetTick() / 100U) % 5U)),
                            phase_colour, 0);
            if (engine->boss_charge_frames > 0) {
                LCD_Draw_Circle(bc_x, bc_y, 27, 6, 0);
                LCD_Draw_Line((uint16_t)(bc_x > 18 ? bc_x - 18 : 0), bc_y,
                              (uint16_t)(bc_x + 18 < SCREEN_WIDTH ? bc_x + 18 : SCREEN_WIDTH - 1), bc_y, 6);
            }
        }
    }

    if (engine->victory_animation_frames == 0 &&
        !(engine->invulnerable_frames > 0 && (engine->invulnerable_frames / 4) % 3 == 1)) {
        int16_t draw_x = engine->player_x;
        int16_t draw_y = engine->player_y;
        Direction visual_weapon_dir = engine->aim_dir != CENTRE ? engine->aim_dir : engine->last_input_dir;
        uint8_t face_left = direction_faces_left(visual_weapon_dir);

        if (engine->hit_effect_frames > 0) {
            draw_x += (engine->hit_effect_frames % 2U) == 0U ? 2 : -2;
            draw_y += (engine->hit_effect_frames % 4U) < 2U ? 1 : -1;
            if (draw_x < 0) { draw_x = 0; }
            if (draw_y < ARENA_TOP) { draw_y = ARENA_TOP; }
            if (draw_x + PLAYER_SIZE > SCREEN_WIDTH) { draw_x = SCREEN_WIDTH - PLAYER_SIZE; }
            if (draw_y + PLAYER_SIZE > ARENA_BOTTOM) { draw_y = ARENA_BOTTOM - PLAYER_SIZE; }
        }

        if (engine->invulnerable_frames == 0) {
            uint16_t aura_x = (uint16_t)(draw_x + PLAYER_SIZE / 2);
            uint16_t aura_y = (uint16_t)(draw_y + PLAYER_SIZE / 2);
            uint8_t aura_colour = ((HAL_GetTick() / 160U) % 2U) == 0U ? 14 : 1;
            LCD_Draw_Circle(aura_x, aura_y, 11, aura_colour, 0);
        }

        if (engine->dash_frames > 0) {
            int16_t trail_dx = 0;
            int16_t trail_dy = 0;
            int16_t ghost_x = draw_x;
            int16_t ghost_y = draw_y;

            direction_to_delta(engine->dash_dir, 5, &trail_dx, &trail_dy);
            ghost_x = (int16_t)(draw_x - trail_dx);
            ghost_y = (int16_t)(draw_y - trail_dy);
            if (ghost_x < 0) { ghost_x = 0; }
            if (ghost_y < ARENA_TOP) { ghost_y = ARENA_TOP; }
            if (ghost_x + PLAYER_SIZE > SCREEN_WIDTH) { ghost_x = SCREEN_WIDTH - PLAYER_SIZE; }
            if (ghost_y + PLAYER_SIZE > ARENA_BOTTOM) { ghost_y = ARENA_BOTTOM - PLAYER_SIZE; }
            LCD_Draw_Sprite_Colour((uint16_t)ghost_x, (uint16_t)ghost_y,
                                   PLAYER_SIZE, PLAYER_SIZE, player_agent_sprite, 14);

            ghost_x = (int16_t)(draw_x - trail_dx * 2);
            ghost_y = (int16_t)(draw_y - trail_dy * 2);
            if (ghost_x < 0) { ghost_x = 0; }
            if (ghost_y < ARENA_TOP) { ghost_y = ARENA_TOP; }
            if (ghost_x + PLAYER_SIZE > SCREEN_WIDTH) { ghost_x = SCREEN_WIDTH - PLAYER_SIZE; }
            if (ghost_y + PLAYER_SIZE > ARENA_BOTTOM) { ghost_y = ARENA_BOTTOM - PLAYER_SIZE; }
            LCD_Draw_Sprite_Colour((uint16_t)ghost_x, (uint16_t)ghost_y,
                                   PLAYER_SIZE, PLAYER_SIZE, player_agent_sprite, 9);
        }

        draw_player_sprite_facing((uint16_t)draw_x, (uint16_t)draw_y,
                                  (engine->invulnerable_frames > 0 && (engine->invulnerable_frames / 4) % 2 == 0)
                                      ? player_agent_hit_sprite
                                      : player_agent_sprite,
                                  face_left);

        {
            uint16_t px = (uint16_t)draw_x;
            uint16_t py = (uint16_t)draw_y;
            draw_agent_gear(px, py, engine->weapon_level, visual_weapon_dir,
                            engine->invulnerable_frames > 0 ? 6 : 0);
        }

        if (engine->aim_indicator_frames > 0 && engine->aim_dir != CENTRE) {
            draw_muzzle_flash((uint16_t)draw_x, (uint16_t)draw_y,
                              engine->aim_dir, engine->weapon_level);
        }

        if (engine->trap_cooldown_frames > 0) {
            uint16_t cd_radius = (uint16_t)(8 + engine->trap_cooldown_frames / 12U);
            if (cd_radius > 14) {
                cd_radius = 14;
            }
            LCD_Draw_Circle((uint16_t)(draw_x + PLAYER_SIZE / 2),
                            (uint16_t)(draw_y + PLAYER_SIZE / 2),
                            cd_radius, 9, 0);
        }
    }

    if (engine->victory_animation_frames > 0) {
        uint8_t progress = (uint8_t)(VICTORY_ANIMATION_FRAMES - engine->victory_animation_frames);
        int16_t start_cx = (int16_t)(engine->player_x + PLAYER_SIZE / 2);
        int16_t start_cy = (int16_t)(engine->player_y + PLAYER_SIZE / 2);
        int16_t target_cx = (int16_t)(SCREEN_WIDTH / 2);
        int16_t target_cy = 132;
        int16_t player_cx = start_cx;
        int16_t player_cy = start_cy;
        int16_t cat_x = 0;
        int16_t cat_y = 0;
        int16_t chest_cx = (int16_t)(engine->chest_x + CHEST_SIZE / 2);
        int16_t chest_cy = (int16_t)(engine->chest_y + CHEST_SIZE / 2);
        uint8_t signal_count = engine->total_signals_collected;
        uint16_t absorb_progress = progress > 22 ? (uint16_t)(progress - 22) : 0U;
        uint16_t ring = (uint16_t)(10 + (progress % 22U));
        uint8_t spin_frame = 0;
        uint8_t i = 0;

        static const int16_t signal_starts[10][2] = {
            { 24,  52}, {206,  48}, { 32, 188}, {202, 196}, {118,  42},
            { 54, 112}, {186, 112}, { 92, 208}, {148, 208}, {120, 132}
        };

        if (progress >= VICTORY_BOARDING_START_FRAME) {
            draw_escape_ship_transition(engine, progress);
            return;
        }

        if (signal_count < 3) {
            signal_count = 3;
        }
        if (signal_count > 10) {
            signal_count = 10;
        }
        if (absorb_progress > 100U) {
            absorb_progress = 100U;
        }
        if (progress < 58) {
            player_cx = (int16_t)(start_cx + ((target_cx - start_cx) * progress) / 58);
            player_cy = (int16_t)(start_cy + ((target_cy - start_cy) * progress) / 58);
            if (progress < 29) {
                player_cy = (int16_t)(player_cy - progress / 2);
            } else {
                player_cy = (int16_t)(player_cy - (58 - progress) / 2);
            }
        } else {
            player_cx = target_cx;
            player_cy = target_cy;
            if (progress < 104) {
                player_cy = (int16_t)(player_cy - 12 + ((progress / 4U) % 3U));
            }
        }

        cat_x = (int16_t)(player_cx - PLAYER_SIZE / 2);
        cat_y = (int16_t)(player_cy - PLAYER_SIZE / 2);
        if (cat_x < 0) { cat_x = 0; }
        if (cat_y < ARENA_TOP) { cat_y = ARENA_TOP; }
        if (cat_x + PLAYER_SIZE > SCREEN_WIDTH) { cat_x = SCREEN_WIDTH - PLAYER_SIZE; }
        if (cat_y + PLAYER_SIZE > ARENA_BOTTOM) { cat_y = ARENA_BOTTOM - PLAYER_SIZE; }

        LCD_Draw_Rect(0, ARENA_TOP, SCREEN_WIDTH, SCREEN_HEIGHT - ARENA_TOP, 13, 0);
        LCD_Draw_Circle((uint16_t)player_cx, (uint16_t)player_cy, ring, 6, 0);
        LCD_Draw_Circle((uint16_t)player_cx, (uint16_t)player_cy, (uint16_t)(ring + 8), 1, 0);

        if (progress < 34) {
            LCD_Draw_Circle((uint16_t)chest_cx, (uint16_t)chest_cy, (uint16_t)(12 + progress / 3U), 6, 0);
            LCD_Draw_Sprite_Colour_Scaled((uint16_t)engine->chest_x,
                                          (uint16_t)engine->chest_y,
                                          8, 8, chest_sprite, 6, 2);
        }

        for (i = 0; i < signal_count; i++) {
            uint16_t delay = (uint16_t)(i * 7U);
            uint16_t local_progress = absorb_progress > delay ? (uint16_t)(absorb_progress - delay) : 0U;
            int16_t start_x = signal_starts[i][0];
            int16_t start_y = signal_starts[i][1];
            int16_t orb_x = start_x;
            int16_t orb_y = start_y;
            uint8_t colour = i % 3U == 0U ? 6 : (i % 3U == 1U ? 1 : 5);

            if (local_progress > 100U) {
                local_progress = 100U;
            }

            orb_x = (int16_t)(start_x + ((player_cx - start_x) * local_progress) / 100);
            orb_y = (int16_t)(start_y + ((player_cy - start_y) * local_progress) / 100);
            orb_x += (int16_t)((i % 2U == 0U ? 1 : -1) * (int16_t)((100U - local_progress) / 12U));
            orb_y += (int16_t)((i % 2U == 0U ? -1 : 1) * (int16_t)((100U - local_progress) / 16U));

            if (orb_x < 0) { orb_x = 0; }
            if (orb_y < ARENA_TOP) { orb_y = ARENA_TOP; }
            if (orb_x > SCREEN_WIDTH - 5) { orb_x = SCREEN_WIDTH - 5; }
            if (orb_y > SCREEN_HEIGHT - 5) { orb_y = SCREEN_HEIGHT - 5; }

            LCD_Draw_Line((uint16_t)orb_x, (uint16_t)orb_y,
                          (uint16_t)player_cx, (uint16_t)player_cy, colour);
            LCD_Draw_Circle((uint16_t)orb_x, (uint16_t)orb_y,
                            local_progress > 86U ? 2 : 4, colour, 1);
        }

        if (progress > 105) {
            uint16_t burst = (uint16_t)(8 + (progress - 105U) * 2U);
            LCD_Draw_Circle((uint16_t)player_cx, (uint16_t)player_cy, burst, 6, 0);
            LCD_Draw_Circle((uint16_t)player_cx, (uint16_t)player_cy, (uint16_t)(burst + 10U), 1, 0);
        }

        if (progress >= 58 && progress < 106) {
            spin_frame = (uint8_t)(((progress - 58U) / 6U) & 3U);
        } else {
            spin_frame = 0;
        }
        if (progress >= 106) {
            LCD_Draw_Line((uint16_t)(player_cx > 16 ? player_cx - 16 : 0),
                          (uint16_t)(player_cy > 14 ? player_cy - 14 : ARENA_TOP),
                          (uint16_t)(player_cx > 6 ? player_cx - 6 : 0),
                          (uint16_t)(player_cy > 4 ? player_cy - 4 : ARENA_TOP), 6);
            LCD_Draw_Line((uint16_t)(player_cx + 16 < SCREEN_WIDTH ? player_cx + 16 : SCREEN_WIDTH - 1),
                          (uint16_t)(player_cy > 14 ? player_cy - 14 : ARENA_TOP),
                          (uint16_t)(player_cx + 6 < SCREEN_WIDTH ? player_cx + 6 : SCREEN_WIDTH - 1),
                          (uint16_t)(player_cy > 4 ? player_cy - 4 : ARENA_TOP), 6);
            LCD_Draw_Circle((uint16_t)player_cx, (uint16_t)player_cy, 15, 1, 0);
        }
        draw_player_sprite_rotated((uint16_t)cat_x, (uint16_t)cat_y,
                                   player_agent_sprite, spin_frame, 1);
        draw_agent_gear((uint16_t)cat_x, (uint16_t)cat_y,
                        engine->weapon_level, E, progress >= 106 ? 6 : 0);
        if (progress >= 106) {
            LCD_printString("VICTORY!", 60, 98, 6, 2);
        }

        LCD_printString(progress < 34 ? "CHEST UNLOCKED" :
                        (progress < 108 ? "SIGNALS ABSORBING" : "SIGNAL SECURED"),
                        progress < 34 ? 32 : (progress < 108 ? 18 : 34),
                        58, progress < 108 ? 6 : 1, 2);
    }

    if (engine->melee_frames > 0) {
        int16_t cx = (int16_t)(engine->player_x + PLAYER_SIZE / 2);
        int16_t cy = (int16_t)(engine->player_y + PLAYER_SIZE / 2);
        int16_t x1 = cx;
        int16_t y1 = cy;
        int16_t x2 = cx;
        int16_t y2 = cy;
        Direction dir = engine->melee_dir;

        if (dir == N || dir == NE || dir == NW) {
            y1 = (int16_t)(cy - 4);
            y2 = (int16_t)(cy - 25);
        }
        if (dir == S || dir == SE || dir == SW) {
            y1 = (int16_t)(cy + 4);
            y2 = (int16_t)(cy + 25);
        }
        if (dir == W || dir == NW || dir == SW) {
            x1 = (int16_t)(cx - 4);
            x2 = (int16_t)(cx - 25);
        }
        if (dir == E || dir == NE || dir == SE) {
            x1 = (int16_t)(cx + 4);
            x2 = (int16_t)(cx + 25);
        }

        if (x1 < 0) { x1 = 0; }
        if (y1 < ARENA_TOP) { y1 = ARENA_TOP; }
        if (x2 < 0) { x2 = 0; }
        if (y2 < ARENA_TOP) { y2 = ARENA_TOP; }
        if (x1 >= SCREEN_WIDTH) { x1 = SCREEN_WIDTH - 1; }
        if (x2 >= SCREEN_WIDTH) { x2 = SCREEN_WIDTH - 1; }
        if (y1 >= SCREEN_HEIGHT) { y1 = SCREEN_HEIGHT - 1; }
        if (y2 >= SCREEN_HEIGHT) { y2 = SCREEN_HEIGHT - 1; }

        LCD_Draw_Line((uint16_t)x1, (uint16_t)y1, (uint16_t)x2, (uint16_t)y2, 1);
        LCD_Draw_Line((uint16_t)(x1 > 1 ? x1 - 1 : x1), (uint16_t)y1,
                      (uint16_t)(x2 > 1 ? x2 - 1 : x2), (uint16_t)y2, 6);
    }

    if (engine->upgrade_message_frames > 0) {
        int16_t text_x = engine->player_x - 28;
        int16_t text_y = engine->player_y - 12;
        if (text_x < 0) {
            text_x = 0;
        }
        if (text_x > SCREEN_WIDTH - 118) {
            text_x = SCREEN_WIDTH - 118;
        }
        if (text_y < ARENA_TOP) {
            text_y = ARENA_TOP;
        }
        LCD_printString("GADGET UPGRADED", (uint16_t)text_x, (uint16_t)text_y, 6, 1);
    }
    if (engine->upgrade_choice_message_frames > 0) {
        LCD_printString(engine->upgrade_choice_index == 0 ? "FIRE RATE UP" :
                        (engine->upgrade_choice_index == 1 ? "BIG SHOT UP" : "VITALITY UP"),
                        58, 64, 6, 1);
    }
    if (engine->trap_message_frames > 0) {
        LCD_printString(engine->trap_cooldown_frames > 0 ? "TRAP SET" : "TRAP!",
                        86, 78, 6, 2);
    }
    if (engine->dash_evade_message_frames > 0) {
        LCD_printString("EVADE!", 82, 64, 14, 2);
    }

    {
        uint16_t hp_w = (uint16_t)((engine->lives * 50U) / engine->max_lives);
        uint8_t hp_colour = engine->lives <= 2 ? 2 : (engine->lives <= 4 ? 6 : 3);
        LCD_printString("HP", 6, 4, hp_colour, 1);
        LCD_Draw_Rect(24, 6, 54, 8, 1, 0);
        LCD_Draw_Rect(26, 8, hp_w, 4, hp_colour, 1);
    }
    snprintf(hud, sizeof(hud), "S%u W%u F%u B%u", engine->score,
             engine->weapon_level, engine->fire_rate_level, engine->bullet_size_level);
    LCD_printString(hud, 84, 4, 1, 1);
    if (engine->signal_decode_active) {
        snprintf(decode_text, sizeof(decode_text), "%c %c %c",
                 direction_to_symbol(engine->decode_sequence[0]),
                 direction_to_symbol(engine->decode_sequence[1]),
                 direction_to_symbol(engine->decode_sequence[2]));
        LCD_printString("DECODE", 126, 2, 6, 1);
        LCD_printString(decode_text, 126, 11, 6, 1);
        snprintf(decode_text, sizeof(decode_text), "STEP:%u", (unsigned)(engine->decode_index + 1));
        LCD_printString(decode_text, 126, 20, 6, 1);
        LCD_printString("R-STICK", 188, 20, 1, 1);
    } else if (engine->first_boss_defeated && !engine->elite_boss_spawned) {
        snprintf(decode_text, sizeof(decode_text), "ELITE %u/5", engine->elite_kills);
        LCD_printString(decode_text, 132, 4, 5, 1);
    } else {
        snprintf(decode_text, sizeof(decode_text), engine->weapon_level < 3 ? "SIG->UP" : "SIG->MOD");
        LCD_printString(decode_text, 132, 4, 6, 1);
    }

    if (engine->upgrade_choice_active) {
        draw_upgrade_choice(engine);
    }

    if (engine->boss_title_frames > 0) {
        LCD_printString(engine->boss_tier == 2 ? "ELITE" : "BOSS",
                        engine->boss_tier == 2 ? 54 : 80, 104, 2, 4);
    }
    if (engine->boss_phase_title_frames > 0 && (engine->boss_phase_title_frames / 5U) % 2U == 0U) {
        LCD_printString("PHASE 2", 54, 92, 6, 3);
        LCD_printString("FIREWALL RAGE", 40, 122, 2, 2);
    }
}

uint8_t PongEngine_GetLives(PongEngine_t* engine)
{
    return engine->lives;
}

uint16_t PongEngine_GetScore(PongEngine_t* engine)
{
    return engine->score;
}

uint8_t PongEngine_IsVictoryComplete(PongEngine_t* engine)
{
    return engine->victory_complete;
}
